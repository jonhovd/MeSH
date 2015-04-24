#include "application.h"

#include <boost/locale/conversion.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <Wt/Utils>
#include <Wt/WAnchor>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WDate>
#include <Wt/WHBoxLayout>
#include <Wt/WImage>
#include <Wt/WStandardItem>
#include <Wt/WStringListModel>
#include <Wt/WVBoxLayout>

//#define LOG_SUPPORT
#ifdef LOG_SUPPORT
# include <boost/thread/mutex.hpp>
boost::mutex g_log_mutex;
# define LOG(x) {boost::mutex::scoped_lock lock(g_log_mutex);fprintf(stdout,x);fflush(stdout);}
#else
# define LOG(x)
#endif

#define SUGGESTION_COUNT    (20)
#define LANGUAGE            "nor"

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

#define EQUAL                           (0)

#define SUGGESTIONLIST_ITEM_ID_ROLE     (Wt::UserRole)
#define HIERARCHY_ITEM_TREE_NUMBER_ROLE (Wt::UserRole+1)
#define HIERARCHY_ITEM_ID_ROLE          (Wt::UserRole+2)


ESPoCApplication::ESPoCApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_signal(this, "search"),
  m_has_populated_hierarchy_model(false),
  m_hierarchy_popup_menu(NULL)
{
    LOG("LOG: start constructor\n");
    messageResourceBundle().use(appRoot() + "strings");
    useStyleSheet(Wt::WLink("MeSH.css"));

    m_es = new ElasticSearch("localhost:9200");

    setTitle(Wt::WString::tr("AppName"));

    WApplication::instance()->internalPathChanged().connect(this, &ESPoCApplication::onInternalPathChange);

    m_infobox = new Wt::WMessageBox(Wt::WString("Hjelp"), Wt::WString::tr("InfoText"), Wt::Icon::NoIcon, Wt::StandardButton::NoButton, root());
    m_infobox->setModal(false);
    m_infobox->setClosable(true);
    m_infobox->setResizable(false);
    m_infobox->setDeleteWhenHidden(false);
    m_infobox->setMaximumSize(Wt::WLength(640, Wt::WLength::Pixel), Wt::WLength::Auto);
    m_infobox->buttonClicked().connect(this, &ESPoCApplication::InfoboxButtonClicked);
    m_infobox_visible = false;
    
    //Header
    Wt::WContainerWidget* header_widget = new Wt::WContainerWidget();
    Wt::WHBoxLayout* header_hbox = new Wt::WHBoxLayout();
    header_hbox->setContentsMargins(0, 0, 0, 0);
    header_widget->setLayout(header_hbox);

    Wt::WImage* logo = new Wt::WImage("images/logo.png");
    header_hbox->addWidget(logo, 0, Wt::AlignLeft|Wt::AlignTop);

    Wt::WContainerWidget* appname_widget = new Wt::WContainerWidget();
    Wt::WVBoxLayout* appname_vbox = new Wt::WVBoxLayout();
    appname_vbox->setContentsMargins(0, 0, 0, 0);
    appname_widget->setLayout(appname_vbox);
    Wt::WText* appname_text = new Wt::WText(Wt::WString::tr("AppName"));
    appname_text->setStyleClass("mesh-appname");
    appname_vbox->addWidget(appname_text, 0, Wt::AlignCenter|Wt::AlignMiddle);
    Wt::WText* appversion_text = new Wt::WText(Wt::WString::tr("AppVersion"));
    appversion_text->setStyleClass("mesh-appversion");
    appname_vbox->addWidget(appversion_text, 0, Wt::AlignCenter);
    header_hbox->addWidget(appname_widget, 1, Wt::AlignCenter|Wt::AlignTop);

    Wt::WContainerWidget* applinks_widget = new Wt::WContainerWidget();
    Wt::WVBoxLayout* applinks_vbox = new Wt::WVBoxLayout();
    applinks_vbox->setContentsMargins(0, 0, 0, 0);
    applinks_widget->setLayout(applinks_vbox);
    Wt::WAnchor* appabout_anchor = new Wt::WAnchor(Wt::WLink(Wt::WString::tr("AppAboutUrl").toUTF8()), Wt::WString::tr("AppAbout"));
    appabout_anchor->setTarget(Wt::TargetNewWindow);
    applinks_vbox->addWidget(appabout_anchor);
    Wt::WAnchor* appquestion_anchor = new Wt::WAnchor(Wt::WLink(Wt::WString::tr("AppSendQuestionUrl").toUTF8()), Wt::WString::tr("AppSendQuestion"));
    applinks_vbox->addWidget(appquestion_anchor);
    Wt::WAnchor* appstatistics_anchor = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, Wt::WString::tr("AppStatisticsInternalPath").toUTF8()), Wt::WString::tr("AppStatistics"));
    applinks_vbox->addWidget(appstatistics_anchor);
    Wt::WAnchor* apphelp_anchor = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, Wt::WString::tr("AppHelpInternalPath").toUTF8()), Wt::WString::tr("AppHelp"));
    applinks_vbox->addWidget(apphelp_anchor);
    header_hbox->addWidget(applinks_widget, 0, Wt::AlignRight|Wt::AlignTop);

    root()->addWidget(header_widget);

    //Tabs
    m_tab_widget = new Wt::WTabWidget(root());

    //Search-tab
    m_tab_widget->addTab(CreateSearchTab(), Wt::WString::tr("Search"));

    //Hierarchy-tab
    Wt::WContainerWidget* hierarchy_tab = new Wt::WContainerWidget();
    Wt::WVBoxLayout* hierarchy_vbox = new Wt::WVBoxLayout();
    hierarchy_vbox->setContentsMargins(0, 9, 0, 0);
    hierarchy_tab->setLayout(hierarchy_vbox);

    m_hierarchy_model = new Wt::WStandardItemModel(hierarchy_vbox);
    m_hierarchy_model->setSortRole(HIERARCHY_ITEM_TREE_NUMBER_ROLE);
    m_hierarchy_tree_view = new Wt::WTreeView();
    m_hierarchy_tree_view->setModel(m_hierarchy_model);
    m_hierarchy_tree_view->setSelectionMode(Wt::SingleSelection);

    hierarchy_vbox->addWidget(m_hierarchy_tree_view);
    m_hierarchy_tree_view->expanded().connect(this, &ESPoCApplication::TreeItemExpanded);
    m_hierarchy_tree_view->clicked().connect(this, &ESPoCApplication::TreeItemClicked);

    m_tab_widget->addTab(hierarchy_tab, Wt::WString::tr("Hierarchy"));
    m_tab_widget->currentChanged().connect(this, &ESPoCApplication::TabChanged);

    //Statistics-tab
    m_statistics_tab = CreateStatisticsTab();
    m_tab_widget->addTab(m_statistics_tab, Wt::WString::tr("Statistics"));
    m_tab_widget->setTabHidden(TAB_INDEX_STATISTICS, true);

    ClearLayout();

    TabChanged(TAB_INDEX_SEARCH);

    ShowOrHideInfobox();
    LOG("LOG: end constructor\n");
}

ESPoCApplication::~ESPoCApplication()
{
    delete m_infobox;
    delete m_hierarchy_popup_menu;
    delete m_es;
}

Wt::WContainerWidget* ESPoCApplication::CreateSearchTab()
{
    Wt::WContainerWidget* search_tab = new Wt::WContainerWidget();
    Wt::WVBoxLayout* search_vbox = new Wt::WVBoxLayout();
    search_vbox->setContentsMargins(0, 9, 0, 0);
    search_tab->setLayout(search_vbox);

    m_search_edit = new Wt::WLineEdit();
    m_search_edit->focussed().connect(this, &ESPoCApplication::SearchEditFocussed);
    m_search_suggestion = CreateSuggestionPopup(root());
    m_search_suggestion->forEdit(m_search_edit);
    m_search_suggestion_model = new Wt::WStandardItemModel(search_vbox);
    m_search_suggestion->setModel(m_search_suggestion_model);
    m_search_suggestion->setFilterLength(2);
    m_search_suggestion->filterModel().connect(this, &ESPoCApplication::FilterSuggestion);
    m_search_suggestion_model->itemChanged().connect(this, &ESPoCApplication::SuggestionChanged);
    m_search_signal.connect(this, &ESPoCApplication::Search);
    search_vbox->addWidget(m_search_edit);

    m_result_container = new Wt::WContainerWidget();
    Wt::WVBoxLayout* result_vbox = new Wt::WVBoxLayout();
    result_vbox->setContentsMargins(0, 0, 0, 0);
    m_result_container->setLayout(result_vbox);

    Wt::WCssDecorationStyle panel_style;
    panel_style.setBackgroundColor(Wt::WColor(232, 232, 232));
    panel_style.setForegroundColor(Wt::WColor(Wt::GlobalColor::black));
    Wt::WFont panel_font;
    panel_font.setSize(Wt::WFont::Size::Large);
    panel_font.setWeight(Wt::WFont::Weight::Bold);
    panel_style.setFont(panel_font);
    
    m_nor_term_panel = new Wt::WPanel();
    m_nor_term_panel->setCollapsible(true);
    Wt::WContainerWidget* nor_term_container = new Wt::WContainerWidget();
    m_nor_term_panel_layout = new Wt::WVBoxLayout();
    m_nor_term_panel_layout->setContentsMargins(0, 0, 0, 0);
    nor_term_container->setLayout(m_nor_term_panel_layout);
    m_nor_term_panel->setCentralWidget(nor_term_container);

    Wt::WImage* nor_flag = new Wt::WImage("images/nor.png");
    m_nor_term_panel->titleBarWidget()->insertWidget(0, nor_flag);
    nor_flag->setHeight(Wt::WLength(1.0, Wt::WLength::FontEm));
    nor_flag->setMargin(Wt::WLength(3.0, Wt::WLength::Pixel));
    m_nor_term_panel->titleBarWidget()->setDecorationStyle(panel_style);

    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("PreferredNorwegianTerm")));
    result_vbox->addWidget(m_nor_term_panel);

    m_nor_description_text = new Wt::WText();
    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("NorwegianDescription")));
    result_vbox->addWidget(m_nor_description_text);

    m_eng_term_panel = new Wt::WPanel();
    m_eng_term_panel->setCollapsible(true);
    Wt::WContainerWidget* eng_term_container = new Wt::WContainerWidget();
    m_eng_term_panel_layout = new Wt::WVBoxLayout();
    m_eng_term_panel_layout->setContentsMargins(0, 0, 0, 0);
    eng_term_container->setLayout(m_eng_term_panel_layout);
    m_eng_term_panel->setCentralWidget(eng_term_container);

    Wt::WImage* eng_flag = new Wt::WImage("images/eng.png");
    m_eng_term_panel->titleBarWidget()->insertWidget(0, eng_flag);
    eng_flag->setHeight(Wt::WLength(1.0, Wt::WLength::FontEm));
    eng_flag->setMargin(Wt::WLength(3.0, Wt::WLength::Pixel));
    m_eng_term_panel->titleBarWidget()->setDecorationStyle(panel_style);

    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("PreferredEnglishTerm")));
    result_vbox->addWidget(m_eng_term_panel);

    m_eng_description_text = new Wt::WText();
    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("EnglishDescription")));
    result_vbox->addWidget(m_eng_description_text);
 
    Wt::WContainerWidget* mesh_id_container = new Wt::WContainerWidget();
    Wt::WHBoxLayout* mesh_id_hbox = new Wt::WHBoxLayout();
    mesh_id_hbox->setContentsMargins(0, 0, 0, 0);
    mesh_id_container->setLayout(mesh_id_hbox);
    m_mesh_id_text = new Wt::WText();
    mesh_id_hbox->addWidget(new Wt::WText(Wt::WString::tr("MeSH_ID")), 0, Wt::AlignLeft);
    mesh_id_hbox->addWidget(m_mesh_id_text, 1, Wt::AlignLeft);
    result_vbox->addWidget(mesh_id_container);

    m_links_layout = new Wt::WGridLayout();
    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("Links")));
    result_vbox->addLayout(m_links_layout);
    
    search_vbox->addWidget(m_result_container);

    return search_tab;
}

Wt::WContainerWidget* ESPoCApplication::CreateStatisticsTab()
{
    Wt::WContainerWidget* statistics_tab = new Wt::WContainerWidget();
    m_statistics_layout = new Wt::WGridLayout();
    m_statistics_layout->setContentsMargins(0, 9, 0, 0);
    statistics_tab->setLayout(m_statistics_layout);

    return statistics_tab;
}

void ESPoCApplication::FindIndirectHit(const std::string& haystack, const std::string& needles, double& best_hit_factor, std::string& indirect_hit_str)
{
    LOG("LOG: start FindIndirectHit\n");
    const std::string lowercase_haystack = boost::locale::to_lower(haystack);
    size_t haystack_length = haystack.length();
    if (0 == haystack_length)
    {
        LOG("LOG: end FindIndirectHit\n");
        return;
    }

    uint8_t* match_mask = new uint8_t[haystack_length];
    size_t i;
    for (i=0; i<haystack_length; i++)
        match_mask[i] = 0;

    std::vector<std::string> needle_vector;
    const std::string lowercase_needles = boost::locale::to_lower(needles);
    boost::split(needle_vector, lowercase_needles, ::isspace);

    std::string needle;
    size_t needle_length;
    size_t search_pos;
    size_t found_pos;
    std::vector<std::string>::iterator it = needle_vector.begin();
    for ( ; it != needle_vector.end(); ++it)
    {
        needle = *it;
        needle_length = needle.length();
        if (0 == needle_length)
            continue;

        search_pos = 0;
        while (std::string::npos != (found_pos = lowercase_haystack.find(needle, search_pos)))
        {
            for (i=0; i<needle_length && (found_pos+i < haystack_length); i++)
            {
                match_mask[found_pos+i] = 1;
            }

            search_pos = found_pos + needle_length;
        }
    }
    
    size_t found_matches = 0;
    for (i=0; i<haystack_length; i++)
    {
        if (0 != match_mask[i])
        {
            found_matches++;
        }
    }
    delete[] match_mask;

    double hit_factor = (double)found_matches/(double)haystack_length;
    if (hit_factor > best_hit_factor)
    {
        indirect_hit_str = haystack;
        best_hit_factor = hit_factor;
    }
    LOG("LOG: end FindIndirectHit\n");
}

void ESPoCApplication::FindIndirectHit(const Json::Object& source_object, const std::string& cleaned_filter_str, std::string& indirect_hit_str)
{
    LOG("LOG: start FindIndirectHit2\n");
    indirect_hit_str.clear();
    double best_hit_factor = 0.0;

    const Json::Value concepts_value = source_object.getValue("concepts");
    const Json::Array concepts_array = concepts_value.getArray();

    Json::Array::const_iterator concept_iterator = concepts_array.begin();
    for (; concept_iterator!=concepts_array.end(); ++concept_iterator)
    {
        const Json::Value concept_value = *concept_iterator;
        const Json::Object concept_object = concept_value.getObject();
#if 0
        if (concept_object.member("description"))
        {
            const Json::Value description_value = concept_object.getValue("description");
            std::string description_str = description_value.getString();
            boost::replace_all(description_str, "\\n", "\n");
            FindIndirectHit(description_str, cleaned_filter_str, best_hit_factor, indirect_hit_str);
        }
        if (concept_object.member("english_description"))
        {
            const Json::Value description_value = concept_object.getValue("english_description");
            std::string description_str = description_value.getString();
            boost::replace_all(description_str, "\\n", "\n");
            FindIndirectHit(description_str, cleaned_filter_str, best_hit_factor, indirect_hit_str);
        }
#endif
        const Json::Value terms_value = concept_object.getValue("terms");
        const Json::Array terms_array = terms_value.getArray();
        Json::Array::const_iterator term_iterator = terms_array.begin();
        for (; term_iterator!=terms_array.end(); ++term_iterator)
        {
            const Json::Value term_value = *term_iterator;
            const Json::Object term_object = term_value.getObject();
            
            const Json::Value term_text_value = term_object.getValue("text");
            FindIndirectHit(term_text_value.getString(), cleaned_filter_str, best_hit_factor, indirect_hit_str);
        }
    }
    LOG("LOG: end FindIndirectHit2\n");
}

void ESPoCApplication::FilterSuggestion(const Wt::WString& filter)
{
    LOG("LOG: start FilterSuggestion\n");
    if (!m_layout_is_cleared)
    {
        ClearLayout();
    }
    
    if (m_infobox_visible)
    {
        ShowOrHideInfobox();
    }

	m_search_suggestion_model->clear();

    std::string filter_str = filter.toUTF8();
    const std::string lowercase_filter_str = boost::locale::to_lower(filter_str);
    std::string cleaned_filter_str;
    CleanFilterString(filter_str, cleaned_filter_str);
    std::string wildcard_filter_str;
    AddWildcard(cleaned_filter_str, wildcard_filter_str);
    
    Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(SUGGESTION_COUNT+1 /* +1 to see if we got more than SUGGESTION_COUNT hits */).arg(filter).arg(wildcard_filter_str);

    Json::Object search_result;
    long result_size = ESSearch("mesh", LANGUAGE, query.toUTF8(), search_result);

    int row = 0;
    if (0 == result_size)
    {
            Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::tr("NoHits"));
            item->setData(boost::any(), SUGGESTIONLIST_ITEM_ID_ROLE);
            m_search_suggestion_model->setItem(row++, 0, item);
    }
    else
    {
        const Json::Value value = search_result.getValue("hits");
        const Json::Object value_object = value.getObject();
        const Json::Value hits_value = value_object.getValue("hits");
        const Json::Array hits_array = hits_value.getArray();

        Json::Array::const_iterator iterator = hits_array.begin();
        for (row=0; row<SUGGESTION_COUNT && iterator!=hits_array.end(); row++)
        {
            const Json::Value hit_value = *iterator;
            const Json::Object hit_value_object = hit_value.getObject();
            const Json::Value source_value = hit_value_object.getValue("_source");
            const Json::Object source_object = source_value.getObject();
            
            const Json::Value id_value = source_object.getValue("id");
            const Json::Value name_value = source_object.getValue("name");

            const std::string lowercase_name_value_str = boost::locale::to_lower(name_value.getString());
            std::string indirect_hit_str;
            Wt::WStandardItem* item;
            if (std::string::npos == lowercase_name_value_str.find(lowercase_filter_str))
            {
                FindIndirectHit(source_object, cleaned_filter_str, indirect_hit_str);
            }
            
            if (!indirect_hit_str.empty())
            {
                item = new Wt::WStandardItem(Wt::WString::tr("IndirectHit").arg(name_value.getString()).arg(indirect_hit_str));
            }
            else
            {
                item = new Wt::WStandardItem(Wt::WString::fromUTF8(name_value.getString()));
            }
            item->setData(boost::any(id_value.getString()), SUGGESTIONLIST_ITEM_ID_ROLE);
            m_search_suggestion_model->setItem(row, 0, item);

            ++iterator;
        }

        m_search_suggestion_model->sort(0);

        if (hits_array.size() > SUGGESTION_COUNT)
        {
            Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::tr("MoreHits").arg(SUGGESTION_COUNT));
            item->setData(boost::any(), SUGGESTIONLIST_ITEM_ID_ROLE);
            m_search_suggestion_model->setItem(row++, 0, item);
        }
    }

    m_search_suggestion_model->setData(--row, 0, std::string("Wt-more-data"), Wt::StyleClassRole);
    LOG("LOG: end FilterSuggestion\n");
}

void ESPoCApplication::SuggestionChanged(Wt::WStandardItem* item)
{
    LOG("LOG: start SuggestionChanged\n");
	if (item)
	{
		item = NULL;
	}
    LOG("LOG: end SuggestionChanged\n");
}

void ESPoCApplication::Search(const Wt::WString& mesh_id)
{
    LOG("LOG: start Search\n");
    Wt::WString preferred_eng_term;

    m_mesh_id_text->setText(mesh_id);

    Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id.toUTF8());

    Json::Object search_result;
    long result_size = ESSearch("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        LOG("LOG: end Search\n");
        return;
    }
    
    LogSearch(mesh_id.toUTF8());

    Wt::WStringListModel* non_preferred_nor_terms = new Wt::WStringListModel();
    Wt::WStringListModel* non_preferred_eng_terms = new Wt::WStringListModel();

    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();
    const Json::Value hit_value = hits_array.first();
    const Json::Object hit_value_object = hit_value.getObject();
    const Json::Value source_value = hit_value_object.getValue("_source");
    const Json::Object source_object = source_value.getObject();
    const Json::Value concepts_value = source_object.getValue("concepts");
    const Json::Array concepts_array = concepts_value.getArray();

    Json::Array::const_iterator concept_iterator = concepts_array.begin();
    for (; concept_iterator!=concepts_array.end(); ++concept_iterator)
    {
        const Json::Value concept_value = *concept_iterator;
        const Json::Object concept_object = concept_value.getObject();
        const Json::Value preferred_concept_value = concept_object.getValue("preferred");
        bool preferred_concept = (EQUAL == preferred_concept_value.getString().compare("yes"));
        if (preferred_concept && concept_object.member("description"))
        {
            const Json::Value description_value = concept_object.getValue("description");
            std::string description_str = description_value.getString();
            boost::replace_all(description_str, "\\n", "\n");
            
            m_nor_description_text->setTextFormat(Wt::PlainText);
            m_nor_description_text->setText(Wt::WString::fromUTF8(description_str));
            m_nor_description_text->show();
        }
        if (preferred_concept && concept_object.member("english_description"))
        {
            const Json::Value description_value = concept_object.getValue("english_description");
            std::string description_str = description_value.getString();
            boost::replace_all(description_str, "\\n", "\n");
            
            m_eng_description_text->setTextFormat(Wt::PlainText);
            m_eng_description_text->setText(Wt::WString::fromUTF8(description_str));
            m_eng_description_text->show();
        }

        const Json::Value terms_value = concept_object.getValue("terms");
        const Json::Array terms_array = terms_value.getArray();
        Json::Array::const_iterator term_iterator = terms_array.begin();
        for (; term_iterator!=terms_array.end(); ++term_iterator)
        {
            const Json::Value term_value = *term_iterator;
            const Json::Object term_object = term_value.getObject();
            
            bool is_norwegian = false;
            if (term_object.member("language"))
            {
                const Json::Value language_value = term_object.getValue("language");
                is_norwegian = (EQUAL == language_value.getString().compare("nor"));
            }

            const Json::Value term_text_value = term_object.getValue("text");
            const Wt::WString term_text_str = Wt::WString::fromUTF8(term_text_value.getString());
            
            const Json::Value preferred_term_value = term_object.getValue("preferred");
            bool preferred_term = (EQUAL == preferred_term_value.getString().compare("yes"));
            if (preferred_concept && preferred_term)
            {
                Wt::WPanel* term_panel = (is_norwegian ? m_nor_term_panel : m_eng_term_panel);
                term_panel->setTitle(term_text_str);
                if (!is_norwegian && preferred_eng_term.empty())
                {
                    preferred_eng_term = term_text_str;
                }
            }
            else
            {
                Wt::WStringListModel* term_list = (is_norwegian ? non_preferred_nor_terms :  non_preferred_eng_terms);
                term_list->addString(term_text_str);
            }
        }
    }

    m_nor_term_panel_layout->addWidget(new Wt::WText(Wt::WString::tr("NonPreferredNorwegianTerms")));
    m_eng_term_panel_layout->addWidget(new Wt::WText(Wt::WString::tr("NonPreferredEnglishTerms")));

    int i;
    for (i=0; i<2; i++)
    {
        const std::vector<Wt::WString>* non_preferred_term_list;
        Wt::WLayout* non_preferred_term_layout = NULL;

        switch(i)
        {
            case 0:
                non_preferred_term_list = &non_preferred_nor_terms->stringList();
                non_preferred_term_layout = m_nor_term_panel_layout;
                break;
                
            case 1:
                non_preferred_term_list = &non_preferred_eng_terms->stringList();
                non_preferred_term_layout = m_eng_term_panel_layout;
                break;
                
            default: break;
        }
        
        if (!non_preferred_term_layout)
            continue;
        
        std::vector<Wt::WString>::const_iterator non_preferred_term_iterator = non_preferred_term_list->begin();
        for ( ; non_preferred_term_iterator!=non_preferred_term_list->end(); ++non_preferred_term_iterator)
        {
            Wt::WString non_preferred_term_string = *non_preferred_term_iterator;
            Wt::WText* non_preferred_term_text = new Wt::WText(non_preferred_term_string, Wt::PlainText);
            non_preferred_term_layout->addWidget(non_preferred_term_text);
        }
    }
    m_result_container->show();
    m_nor_term_panel->expand();
    m_eng_term_panel->collapse();

    std::string url_encoded_term = Wt::Utils::urlEncode(preferred_eng_term.toUTF8());
    std::string url_encoded_filtertext = Wt::Utils::urlEncode(m_search_edit->text().toUTF8());
    int link_category_index = 0;
    int link_index;
    while (true)
    {
        link_category_index++;
        Wt::WString link_category_key = Wt::WString::tr("LinkCategoryFormat").arg(link_category_index);
        Wt::WString link_category_text = Wt::WString::tr(link_category_key.toUTF8());
        if ('?' == link_category_text.toUTF8().at(0))
            break;

        m_links_layout->addWidget(new Wt::WText(link_category_text), 0, link_category_index-1);

        Wt::WContainerWidget* container = new Wt::WContainerWidget();
        container->setContentAlignment(Wt::AlignJustify);
        link_index = 0;
        while (true)
        {
            link_index++;
            Wt::WString link_text_key = Wt::WString::tr("LinkTextFormat").arg(link_category_index).arg(link_index);
            Wt::WString link_url_key = Wt::WString::tr("LinkUrlFormat").arg(link_category_index).arg(link_index);
            Wt::WString link_text = Wt::WString::tr(link_text_key.toUTF8());
            if ('?' == link_text.toUTF8().at(0))
                break;
            
            Wt::WString link_url = Wt::WString::tr(link_url_key.toUTF8()).arg(mesh_id).arg(url_encoded_term).arg(url_encoded_filtertext);
            std::string link_str = link_url.toUTF8();
            boost::replace_all(link_str, "&amp;", "&");
            Wt::WAnchor* anchor = new Wt::WAnchor(Wt::WLink(link_str), link_text);
            anchor->setTarget(Wt::TargetNewWindow);
            anchor->setPadding(Wt::WLength(2.5, Wt::WLength::FontEm), Wt::Right);
            container->addWidget(anchor);
        }
        m_links_layout->addWidget(container, 1, link_category_index-1);
    }
    
    delete non_preferred_nor_terms;
    delete non_preferred_eng_terms;
    
    //Collapse/expand hierarchy
    CollapseHierarchy();
    if (source_object.member("tree_numbers"))
    {
        const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
        const Json::Array tree_numbers_array = tree_numbers_value.getArray();
        Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
        for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
        {
            const Json::Value tree_number_value = *tree_numbers_iterator;
            ExpandToTreeNumber(tree_number_value.getString());
        }
    }
            
    m_layout_is_cleared = false;
    LOG("LOG: end Search\n");
}

void ESPoCApplication::CollapseHierarchy()
{
    LOG("LOG: start CollapseHierarchy\n");
    PopulateHierarchy(); //Just in case it isn't populated yet

    int row = 0;
    Wt::WModelIndex index;
    while (true)
    {
        index = m_hierarchy_model->index(row, 0);
        if (!index.isValid())
            break;

        m_hierarchy_tree_view->collapse(index);
        row++;
    }
    LOG("LOG: end CollapseHierarchy\n");
}

void ESPoCApplication::ExpandToTreeNumber(const std::string& tree_number_string)
{
    LOG("LOG: start ExpandToTreeNumber\n");
    Wt::WModelIndex model_index;
    ExpandTreeNumberRecursive(tree_number_string, model_index);
    LOG("LOG: end ExpandToTreeNumber\n");
}

void ESPoCApplication::ExpandTreeNumberRecursive(const std::string& current_tree_number_string, Wt::WModelIndex& model_index)
{
    std::string parent_tree_number_string;
    GetParentTreeNumber(current_tree_number_string, parent_tree_number_string);
    bool top_level = parent_tree_number_string.empty();
    if (!top_level) //We're not at top-level yet
    {
        ExpandTreeNumberRecursive(parent_tree_number_string, model_index); //Recurse, depth-first
    }

    if (FindChildModelIndex(current_tree_number_string, top_level, model_index))
    {
        TreeItemExpanded(model_index);
        m_hierarchy_tree_view->expand(model_index);
    }
}

bool ESPoCApplication::FindChildModelIndex(const std::string& tree_number_string, bool top_level, Wt::WModelIndex& index)
{
    LOG("LOG: start FindChildModelIndex\n");
    int row = 0;
    Wt::WModelIndex child_index;
    Wt::WStandardItem* standard_item;
    std::string item_tree_number_string;
    while (true)
    {
        child_index = top_level ? m_hierarchy_model->index(row, 0) : index.child(row, 0);
        if (!child_index.isValid())
        {
            LOG("LOG: end FindChildModelIndex\n");
            return false;
        }

        standard_item = m_hierarchy_model->itemFromIndex(child_index);
        if (!standard_item)
        {
            LOG("LOG: end FindChildModelIndex\n");
            return false;
        }

        item_tree_number_string = boost::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE));
        if (EQUAL == tree_number_string.compare(item_tree_number_string))
        {
            index = child_index;
            LOG("LOG: end FindChildModelIndex\n");
            return true;
        }

        row++;
    }
}

void ESPoCApplication::SearchEditFocussed()
{
    LOG("LOG: start SearchEditFocussed\n");
    m_search_edit->setText("");
    LOG("LOG: end SearchEditFocussed\n");
}

void ESPoCApplication::TabChanged(int active_tab_index)
{
    LOG("LOG: start TabChanged\n");
    switch(active_tab_index)
    {
        case TAB_INDEX_HIERARCHY: PopulateHierarchy(); break;

        case TAB_INDEX_STATISTICS: PopulateStatistics(); break;

        case TAB_INDEX_SEARCH: //Fallthrough
        default: m_search_edit->setFocus(); break;
    }
    LOG("LOG: end TabChanged\n");
}

void ESPoCApplication::TreeItemExpanded(const Wt::WModelIndex& index)
{
    LOG("LOG: start TreeItemExpanded\n");
    if (!index.isValid())
    {
        LOG("LOG: end TreeItemExpanded\n");
        return;
    }

    Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(index);
    if (!standard_item || !standard_item->hasChildren())
    {
        LOG("LOG: end TreeItemExpanded\n");
        return;
    }

    Wt::WStandardItem* possible_placeholder = standard_item->child(0, 0);
    if (!possible_placeholder || //We don't have a children placeholder. This item should not be populated by children 
        !possible_placeholder->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE).empty()) //This is a real child, not a placeholder. No need to populate children one more time.
    {
        LOG("LOG: end TreeItemExpanded\n");
        return;
    }

    //Remove placeholder
    possible_placeholder = standard_item->takeChild(0, 0);
    delete possible_placeholder;

    std::string parent_tree_number_string = boost::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE));
    //Fetch all children from ElasticSearch
    Wt::WString query = Wt::WString::tr("HierarchyChildrenQuery").arg(parent_tree_number_string);

    Json::Object search_result;
    long result_size = ESSearch("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        LOG("LOG: end TreeItemExpanded\n");
        return;
    }

    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    bool added_items = false;
    int row = 0;
    Json::Array::const_iterator iterator = hits_array.begin();
    for (; iterator!=hits_array.end(); ++iterator)
    {
        const Json::Value hit_value = *iterator;
        const Json::Object hit_value_object = hit_value.getObject();
        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value id_value = source_object.getValue("id");
        std::string id_value_string = id_value.getString();
        if (id_value_string.empty())
            continue;
        
        std::string tree_number_value_string;
        std::string possible_parent_tree_number_string;
        const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
        const Json::Array tree_numbers_array = tree_numbers_value.getArray();
        Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
        for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
        {
            const Json::Value tree_number_value = *tree_numbers_iterator;
            tree_number_value_string = tree_number_value.getString();
            GetParentTreeNumber(tree_number_value_string, possible_parent_tree_number_string);
            if (EQUAL == parent_tree_number_string.compare(possible_parent_tree_number_string)) //This three_number matches the parent, add it as a child
            {
                const Json::Value name_value = source_object.getValue("name");
                
                std::stringstream node_text;
                node_text << name_value.getString();
                if (!tree_number_value_string.empty())
                {
                    node_text << " [" << tree_number_value_string << "]";
                }

                Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::fromUTF8(node_text.str()));
                AddChildPlaceholderIfNeeded(source_object, tree_number_value_string, item);
                item->setData(boost::any(tree_number_value_string), HIERARCHY_ITEM_TREE_NUMBER_ROLE);
                item->setData(boost::any(id_value_string), HIERARCHY_ITEM_ID_ROLE);
                standard_item->setChild(row++, 0, item);
                added_items = true;
            }
        }
    }
    
    if (added_items)
    {
        m_hierarchy_model->sort(0);
    }
    LOG("LOG: end TreeItemExpanded\n");
}

void ESPoCApplication::TreeItemClicked(const Wt::WModelIndex& index, const Wt::WMouseEvent& mouse)
{
    LOG("LOG: start TreeItemClicked\n");
    if (!index.isValid())
    {
        LOG("LOG: end TreeItemClicked\n");
        return;
    }
    
    Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(index);
    if (!standard_item)
    {
        LOG("LOG: end TreeItemClicked\n");
        return;
    }
    
    if (m_hierarchy_popup_menu)
    {
        delete m_hierarchy_popup_menu;
    }

    m_hierarchy_popup_menu = new Wt::WPopupMenu;
    m_hierarchy_popup_menu->setAutoHide(true, 1000);

    m_popup_menu_id_string = boost::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_ID_ROLE));

    Wt::WString soek = Wt::WString::tr("SearchFromHierarchy").arg(standard_item->text().toUTF8());
    m_hierarchy_popup_menu->addItem(soek)->triggered().connect(this, &ESPoCApplication::PopupMenuTriggered);

    m_hierarchy_popup_menu->popup(mouse);
    LOG("LOG: end TreeItemClicked\n");
}

void ESPoCApplication::PopupMenuTriggered(Wt::WMenuItem* item)
{
    LOG("LOG: start PopupMenuTriggered\n");
    if (item && !m_popup_menu_id_string.empty())
    {
        ClearLayout();
        m_tab_widget->setCurrentIndex(TAB_INDEX_SEARCH);
        Search(m_popup_menu_id_string);
    }
    LOG("LOG: end PopupMenuTriggered\n");
}

void ESPoCApplication::InfoboxButtonClicked()
{
    LOG("LOG: start InfoboxButtonClicked\n");
    ShowOrHideInfobox();
    LOG("LOG: end InfoboxButtonClicked\n");
}

long ESPoCApplication::ESSearch(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result)
{
	try
	{
		return m_es->search(index, type, query, search_result);
	}
    catch(...)
	{
		return 0L;
    }
}

void ESPoCApplication::LogSearch(const std::string& search_string)
{
    //Update search text statistics
    int count = 0;
    Json::Object text_search_result;
    if (m_es->getDocument("statistics", "text", search_string.c_str(), text_search_result))
    {
        const Json::Value source_value = text_search_result.getValue("_source");
        const Json::Object source_object = source_value.getObject();
        if (source_object.member("count"))
        {
            const Json::Value count_value = source_object.getValue("count");
            count = count_value.getInt();
        }
    }
    
    Json::Object textstat_json;
    textstat_json.addMemberByKey("count", ++count);
    m_es->upsert("statistics", "text", search_string, textstat_json);


    //Update search day statistics
    Wt::WDate today = Wt::WDate::currentServerDate();
    const std::string today_string = today.toString("yyyy-MM-dd").toUTF8();
    count = 0;
    Json::Object day_search_result;
    if (m_es->getDocument("statistics", "day", today_string.c_str(), day_search_result))
    {
        const Json::Value source_value = day_search_result.getValue("_source");
        const Json::Object source_object = source_value.getObject();
        if (source_object.member("count"))
        {
            const Json::Value count_value = source_object.getValue("count");
            count = count_value.getInt();
        }
    }
    
    Json::Object daystat_json;
    daystat_json.addMemberByKey("count", ++count);
    m_es->upsert("statistics", "day", today_string, daystat_json);
}

void ESPoCApplication::PopulateHierarchy()
{
    LOG("LOG: start PopulateHierarchy\n");
    if (m_has_populated_hierarchy_model)
    {
        LOG("LOG: end PopulateHierarchy\n");
        return;
    }

    Wt::WString query = Wt::WString::tr("HierarchyTopNodesQuery");

    Json::Object search_result;
    long result_size = ESSearch("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        LOG("LOG: end PopulateHierarchy\n");
        return;
    }
    
    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    int row = 0;
    Json::Array::const_iterator iterator = hits_array.begin();
    for (; iterator!=hits_array.end(); ++iterator)
    {
        const Json::Value hit_value = *iterator;
        const Json::Object hit_value_object = hit_value.getObject();
        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value id_value = source_object.getValue("id");
        std::string id_value_string = id_value.getString();
        if (id_value_string.empty())
            continue;
        
        std::string tree_number_value_string;
        if (source_object.member("tree_numbers"))
        {
            const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
            const Json::Array tree_numbers_array = tree_numbers_value.getArray();
            Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
            for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
            {
                const Json::Value tree_number_value = *tree_numbers_iterator;
                tree_number_value_string = tree_number_value.getString();
                if (std::string::npos != tree_number_value_string.find('.'))
                    continue; //skip child tree_numbers for meshes that also have top-level tree_numbers
                    
                //We have a top-level tree_number!
                const Json::Value name_value = source_object.getValue("name");
                
                std::stringstream node_text;
                node_text << name_value.getString();
                if (!tree_number_value_string.empty())
                {
                    node_text << " [" << tree_number_value_string << "]";
                }

                Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::fromUTF8(node_text.str()));
                AddChildPlaceholderIfNeeded(source_object, tree_number_value_string, item);
                item->setData(boost::any(tree_number_value_string), HIERARCHY_ITEM_TREE_NUMBER_ROLE);
                item->setData(boost::any(id_value_string), HIERARCHY_ITEM_ID_ROLE);
                m_hierarchy_model->setItem(row++, 0, item);
            }
        }
    }
    m_hierarchy_model->sort(0);
    m_has_populated_hierarchy_model = true;
    LOG("LOG: end PopulateHierarchy\n");
}

void ESPoCApplication::PopulateStatistics()
{
    LOG("LOG: start PopulateStatistics\n");

    m_statistics_layout->clear();
    int i;
    for (i=0; i<8; i++)
    {
        m_statistics_layout->setColumnStretch(i, 0);
    }
    for (i=0; i<=8; i+=4)
    {
        m_statistics_layout->setColumnStretch(i, 1);
        m_statistics_layout->addWidget(new Wt::WText(""), 0, i);
    }

    PopulateDayStatistics();
    PopulateTextStatistics();


    LOG("LOG: end PopulateStatistics\n");
}

void ESPoCApplication::PopulateDayStatistics()
{
    LOG("LOG: start PopulateDayStatistics\n");

    m_statistics_layout->addWidget(new Wt::WText(Wt::WString::tr("StatisticsPerDay")), 0, 1);

    Wt::WString query = Wt::WString::tr("StatisticsDay");

    Json::Object search_result;
    long result_size = ESSearch("statistics", "day", query.toUTF8(), search_result);
    if (0 == result_size)
    {
        LOG("LOG: end PopulateDayStatistics\n");
        return;
    }
    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    int row = 0;
    Json::Array::const_iterator iterator = hits_array.begin();
    for (; iterator!=hits_array.end(); ++iterator)
    {
        const Json::Value hit_value = *iterator;
        const Json::Object hit_value_object = hit_value.getObject();

        const Json::Value day_value = hit_value_object.getValue("_id");
        std::string day_value_string = day_value.getString();

        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value count_value = source_object.getValue("count");
        int count_value_int = count_value.getInt();

        m_statistics_layout->addWidget(new Wt::WText(day_value_string), row, 2);
        m_statistics_layout->addWidget(new Wt::WText(Wt::WString("{1}").arg(count_value_int)), row, 3);
        row++;
    }

    LOG("LOG: end PopulateDayStatistics\n");
}

void ESPoCApplication::PopulateTextStatistics()
{
    LOG("LOG: start PopulateTextStatistics\n");

    m_statistics_layout->addWidget(new Wt::WText(Wt::WString::tr("StatisticsPerMeSH")), 0, 5);

    Wt::WString query = Wt::WString::tr("StatisticsText");

    Json::Object search_result;
    long result_size = ESSearch("statistics", "text", query.toUTF8(), search_result);
    if (0 == result_size)
    {
        LOG("LOG: end PopulateTextStatistics\n");
        return;
    }
    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    int row = 0;
    Json::Array::const_iterator iterator = hits_array.begin();
    for (; iterator!=hits_array.end(); ++iterator)
    {
        const Json::Value hit_value = *iterator;
        const Json::Object hit_value_object = hit_value.getObject();

        const Json::Value mesh_value = hit_value_object.getValue("_id");
        std::string mesh_value_string = mesh_value.getString();

        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value count_value = source_object.getValue("count");
        int count_value_int = count_value.getInt();

        std::string name;
        MeSHToName(mesh_value_string, name);
        m_statistics_layout->addWidget(new Wt::WText(name), row, 6);
        m_statistics_layout->addWidget(new Wt::WText(Wt::WString("{1}").arg(count_value_int)), row, 7);
        row++;
    }

    LOG("LOG: end PopulateTextStatistics\n");
}

Wt::WSuggestionPopup* ESPoCApplication::CreateSuggestionPopup(Wt::WContainerWidget* parent)
{
    std::string matcherJS = INLINE_JAVASCRIPT(
        function (edit) {
            var value = edit.value;
            return function(suggestion) {
                if (!suggestion)
                    return value;

                var match = suggestion.toLowerCase().indexOf(value.toLowerCase());
                if (-1 == match) {
                    return {match: true, suggestion: suggestion};
                } else {
                    var s = suggestion.substr(0, match)+"<b>"+suggestion.substr(match, value.length)+"</b>"+suggestion.substr(match+value.length);
                    return { match : true, suggestion: s};
                }
            }
        }
    );

    std::string replacerJS = INLINE_JAVASCRIPT(
        function(edit, suggestionText, suggestionValue) {
            var text = suggestionText;
            var match = text.indexOf("<b>");
            if (-1 != match) {
                text = text.substr(0, match)+text.substr(match+3);
            }
            match = text.indexOf("</b>");
            if (-1 != match) {
                text = text.substr(0, match)+text.substr(match+4);
            }
            edit.value = text;

            Wt.emit(Wt, 'search', suggestionValue);
        }
     );

    return new Wt::WSuggestionPopup(matcherJS, replacerJS, parent);
}

void ESPoCApplication::ClearLayout()
{
    LOG("LOG: start ClearLayout\n");
    m_nor_term_panel->setTitle("");
    m_nor_term_panel->expand();
    m_nor_term_panel_layout->clear();
    
    m_nor_description_text->setText("");
    m_nor_description_text->hide();

    m_eng_term_panel->setTitle("");
    m_eng_term_panel->collapse();
    m_eng_term_panel_layout->clear();

    m_eng_description_text->setText("");
    m_eng_description_text->hide();

    m_mesh_id_text->setText("");

    m_links_layout->clear();

    m_result_container->hide();
    
    m_layout_is_cleared = true;
    LOG("LOG: end ClearLayout\n");
}

void ESPoCApplication::CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str) const
{
    cleaned_filter_str = filter_str;
    size_t filter_length = cleaned_filter_str.length();
    size_t i;
    for (i=0; i<filter_length; i++)
    {
        if (ispunct(cleaned_filter_str[i]))
        {
            cleaned_filter_str[i] = ' ';
        }
    }
}

void ESPoCApplication::AddWildcard(const std::string filter_str, std::string& wildcard_filter_str) const
{
    wildcard_filter_str = filter_str;
    size_t filter_length = wildcard_filter_str.length();
    if (0<filter_length && ' '!=wildcard_filter_str[filter_length-1])
    {
        wildcard_filter_str.append("*");
    }
}

void ESPoCApplication::GetParentTreeNumber(const std::string& child_tree_number, std::string& parent_tree_number)
{
    size_t substring_length = child_tree_number.find_last_of('.');
    if (std::string::npos==substring_length && 1<child_tree_number.length()) //If length==1, we have a forced topnode. We know they do not have a parent
    {
        substring_length = 1; //If we are forcing topnodes, "D" is the valid parent for "D01" (without dot..)
    }
    parent_tree_number = (std::string::npos==substring_length) ? "" : child_tree_number.substr(0, substring_length);
}

bool ESPoCApplication::AddChildPlaceholderIfNeeded(const Json::Object& source_object, const std::string& current_tree_number_string, Wt::WStandardItem* current_item)
{
    LOG("LOG: start AddChildPlaceholderIfNeeded\n");
    bool added_placeholder = false;
    //Check if we have a matching child in the child_tree_numbers array
    if (source_object.member("child_tree_numbers"))
    {
        std::string possible_parent_tree_number_string;

        const Json::Value child_tree_numbers_value = source_object.getValue("child_tree_numbers");
        const Json::Array child_tree_numbers_array = child_tree_numbers_value.getArray();
        Json::Array::const_iterator child_iterator = child_tree_numbers_array.begin();
        for (; child_iterator!=child_tree_numbers_array.end(); ++child_iterator)
        {
            const Json::Value child_tree_number_value = *child_iterator;
            GetParentTreeNumber(child_tree_number_value.getString(), possible_parent_tree_number_string);
            if (EQUAL == current_tree_number_string.compare(possible_parent_tree_number_string))
            {
                Wt::WStandardItem* child_item = new Wt::WStandardItem(Wt::WString("")); //Placeholder, adds the [+]-icon
                current_item->setChild(0, 0, child_item);
                added_placeholder = true;
            }
        }
    }
    LOG("LOG: almost end AddChildPlaceholderIfNeeded\n");
    return added_placeholder;
}

void ESPoCApplication::ShowOrHideInfobox()
{
    m_infobox_visible = !m_infobox_visible;
    m_infobox->setHidden(!m_infobox_visible);
    if (TAB_INDEX_SEARCH >= m_tab_widget->currentIndex())
    {
        m_search_edit->setFocus();
    }
}

void ESPoCApplication::MeSHToName(const std::string& mesh_id, std::string& name)
{
    name = mesh_id;

    Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id);
    Json::Object search_result;
    long result_size = ESSearch("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        return;
    }
    
    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();
    const Json::Value hit_value = hits_array.first();
    const Json::Object hit_value_object = hit_value.getObject();
    const Json::Value source_value = hit_value_object.getValue("_source");
    const Json::Object source_object = source_value.getObject();
    const Json::Value concepts_value = source_object.getValue("concepts");
    const Json::Array concepts_array = concepts_value.getArray();

    Json::Array::const_iterator concept_iterator = concepts_array.begin();
    for (; concept_iterator!=concepts_array.end(); ++concept_iterator)
    {
        const Json::Value concept_value = *concept_iterator;
        const Json::Object concept_object = concept_value.getObject();
        const Json::Value preferred_concept_value = concept_object.getValue("preferred");
        bool preferred_concept = (EQUAL == preferred_concept_value.getString().compare("yes"));
        if (!preferred_concept)
        {
            continue;
        }

        const Json::Value terms_value = concept_object.getValue("terms");
        const Json::Array terms_array = terms_value.getArray();
        Json::Array::const_iterator term_iterator = terms_array.begin();
        for (; term_iterator!=terms_array.end(); ++term_iterator)
        {
            const Json::Value term_value = *term_iterator;
            const Json::Object term_object = term_value.getObject();
            
            const Json::Value preferred_term_value = term_object.getValue("preferred");
            bool preferred_term = (EQUAL == preferred_term_value.getString().compare("yes"));
            if (!preferred_term)
            {
                continue;
            }

            const Json::Value term_text_value = term_object.getValue("text");
            name = term_text_value.getString();
            
            bool is_norwegian = false;
            if (term_object.member("language"))
            {
                const Json::Value language_value = term_object.getValue("language");
                is_norwegian = (EQUAL == language_value.getString().compare("nor"));
            }
            if (is_norwegian)
            {
                return;
            }
        }
    }
}

void ESPoCApplication::onInternalPathChange(const std::string& url)
{
    LOG("LOG: start onInternalPathChange\n");
    if (EQUAL == url.compare(Wt::WString::tr("AppHelpInternalPath").toUTF8())) {
        ShowOrHideInfobox();
    }
    else if (EQUAL == url.compare(Wt::WString::tr("AppStatisticsInternalPath").toUTF8()))
    {
        m_tab_widget->setTabHidden(TAB_INDEX_STATISTICS, false);
    }
    WApplication::instance()->setInternalPath("/");
    LOG("LOG: end onInternalPathChange\n");
}
