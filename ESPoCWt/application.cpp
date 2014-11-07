#include "application.h"

#include <boost/algorithm/string/replace.hpp>
#include <Wt/Utils>
#include <Wt/WAnchor>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WImage>
#include <Wt/WStandardItem>
#include <Wt/WStringListModel>
#include <Wt/WTabWidget>
#include <Wt/WVBoxLayout>

#define SUGGESTION_COUNT	(20)
#define LANGUAGE            "nor"

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__


ESPoCApplication::ESPoCApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_signal(this, "search")
{
    messageResourceBundle().use(appRoot() + "strings");

	m_es = new ElasticSearch("localhost:9200");

	setTitle("ElasticSearch Proof-of-Concept Wt application");

	Wt::WTabWidget* tab_widget = new Wt::WTabWidget(root());

    //Search
	Wt::WContainerWidget* search_tab = new Wt::WContainerWidget();
    Wt::WVBoxLayout* search_vbox = new Wt::WVBoxLayout();
    search_tab->setLayout(search_vbox);

    m_search_edit = new Wt::WLineEdit();

	m_search_suggestion = CreateSuggestionPopup(root());
	m_search_suggestion->forEdit(m_search_edit);
	m_search_suggestion_model = new Wt::WStandardItemModel(search_vbox);
	m_search_suggestion->setModel(m_search_suggestion_model);
	m_search_suggestion->setFilterLength(2);
    m_search_suggestion->filterModel().connect(this, &ESPoCApplication::FilterSuggestion);
	m_search_suggestion_model->itemChanged().connect(this, &ESPoCApplication::SuggestionChanged);
    m_search_signal.connect(this, &ESPoCApplication::Search);
	
    m_nor_term_panel = new Wt::WPanel();
    m_nor_term_panel->setCollapsible(true);
    Wt::WContainerWidget* nor_term_container = new Wt::WContainerWidget();
    m_nor_term_panel_layout = new Wt::WVBoxLayout();
    nor_term_container->setLayout(m_nor_term_panel_layout);
    m_nor_term_panel->setCentralWidget(nor_term_container);

    Wt::WImage* nor_flag = new Wt::WImage("images/nor.png");
    m_nor_term_panel->titleBarWidget()->insertWidget(0, nor_flag);
    nor_flag->setHeight(Wt::WLength(1.0, Wt::WLength::FontEm));
    nor_flag->setMargin(Wt::WLength(3.0, Wt::WLength::Pixel));

    m_eng_term_panel = new Wt::WPanel();
    m_eng_term_panel->setCollapsible(true);
    Wt::WContainerWidget* eng_term_container = new Wt::WContainerWidget();
    m_eng_term_panel_layout = new Wt::WVBoxLayout();
    eng_term_container->setLayout(m_eng_term_panel_layout);
    m_eng_term_panel->setCentralWidget(eng_term_container);

    Wt::WImage* eng_flag = new Wt::WImage("images/eng.png");
    m_eng_term_panel->titleBarWidget()->insertWidget(0, eng_flag);
    eng_flag->setHeight(Wt::WLength(1.0, Wt::WLength::FontEm));
    eng_flag->setMargin(Wt::WLength(3.0, Wt::WLength::Pixel));

    m_description_text = new Wt::WText();
 
    m_mesh_id_text = new Wt::WText();

    m_links_layout = new Wt::WGridLayout();

    search_vbox->addWidget(m_search_edit);
    search_vbox->addWidget(new Wt::WText(Wt::WString::tr("PreferredNorwegianTerm")));
    search_vbox->addWidget(m_nor_term_panel);
    search_vbox->addWidget(new Wt::WText(Wt::WString::tr("PreferredEnglishTerm")));
    search_vbox->addWidget(m_eng_term_panel);
    search_vbox->addWidget(new Wt::WText(Wt::WString::tr("Description")));
    search_vbox->addWidget(m_description_text);
    search_vbox->addWidget(new Wt::WText(Wt::WString::tr("MeSH_ID")));
    search_vbox->addWidget(m_mesh_id_text);
    search_vbox->addWidget(new Wt::WText(Wt::WString::tr("Links")));
    search_vbox->addLayout(m_links_layout);
 
	tab_widget->addTab(search_tab, Wt::WString::tr("Search"));

    //Hierarchy
    Wt::WContainerWidget* hierarchy_tab = new Wt::WContainerWidget();
    Wt::WVBoxLayout* hierarchy_vbox = new Wt::WVBoxLayout();
    hierarchy_tab->setLayout(hierarchy_vbox);

    m_hierarchy_model = new Wt::WStandardItemModel(hierarchy_vbox);
    m_hierarchy_tree_view = new Wt::WTreeView();
    m_hierarchy_tree_view->setModel(m_hierarchy_model);
    m_hierarchy_tree_view->setSelectionMode(Wt::SingleSelection);

    hierarchy_vbox->addWidget(m_hierarchy_tree_view);

    tab_widget->addTab(hierarchy_tab, Wt::WString::tr("Hierarchy"));
    tab_widget->currentChanged().connect(this, &ESPoCApplication::TabChanged);


    ClearLayout();
    m_search_edit->setFocus();
}

ESPoCApplication::~ESPoCApplication()
{
	delete m_es;
}

void ESPoCApplication::FindIndirectHit(const Json::Object& /*source_object*/, std::string& indirect_hit_str)
{
    indirect_hit_str = "ToDo";
}

void ESPoCApplication::FilterSuggestion(const Wt::WString& filter)
{
    if (!m_layout_is_cleared)
    {
        ClearLayout();
    }

	m_search_suggestion_model->clear();

    std::string filter_str = filter.toUTF8();
    std::string cleaned_filter_str;
    CleanFilterString(filter_str, cleaned_filter_str);
    
	Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(SUGGESTION_COUNT+1 /* +1 to see if we got more than SUGGESTION_COUNT hits */).arg(filter).arg(cleaned_filter_str);

	Json::Object search_result;
	long result_size = ESSearch("mesh", LANGUAGE, query.toUTF8(), search_result);

	int row = 0;
	if (0 == result_size)
	{
            Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::tr("NoHits"));
            item->setData(boost::any(), Wt::UserRole);
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

            const std::string name_value_str = name_value.getString();
            std::string indirect_hit_str;
            Wt::WStandardItem* item;
            if (std::string::npos == name_value_str.find(filter_str))
            {
                FindIndirectHit(source_object, indirect_hit_str);
            }
            
            if (!indirect_hit_str.empty())
            {
                item = new Wt::WStandardItem(Wt::WString::tr("IndirectHit").arg(name_value.getString()).arg(indirect_hit_str));
            }
            else
            {
                item = new Wt::WStandardItem(Wt::WString::fromUTF8(name_value.getString()));
            }
			item->setData(boost::any(id_value.getString()), Wt::UserRole);
			m_search_suggestion_model->setItem(row, 0, item);

			++iterator;
		}

        m_search_suggestion_model->sort(0);

        if (hits_array.size() > SUGGESTION_COUNT)
        {
            Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::tr("MoreHits"));
            item->setData(boost::any(), Wt::UserRole);
            m_search_suggestion_model->setItem(row++, 0, item);
        }
	}

	m_search_suggestion_model->setData(--row, 0, std::string("Wt-more-data"), Wt::StyleClassRole);
}

void ESPoCApplication::SuggestionChanged(Wt::WStandardItem* item)
{
	if (item)
	{
		item = NULL;
	}
}

void ESPoCApplication::Search(const Wt::WString& mesh_id)
{
    Wt::WString preferred_eng_term;

    m_mesh_id_text->setText(mesh_id);
    m_mesh_id_text->show();

    Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id.toUTF8());

    Json::Object search_result;
    long result_size = ESSearch("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        return;
    }
    
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
        bool preferred_concept = (0 == preferred_concept_value.getString().compare("yes"));
        if (preferred_concept && concept_object.member("description"))
        {
            const Json::Value description_value = concept_object.getValue("description");
            std::string description_str = description_value.getString();
            boost::replace_all(description_str, "\\n", "\n");
            
            m_description_text->setTextFormat(Wt::PlainText);
            m_description_text->setText(Wt::WString::fromUTF8(description_str));
            m_description_text->show();
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
                is_norwegian = (0 == language_value.getString().compare("nor"));
            }

            const Json::Value term_text_value = term_object.getValue("text");
            const Wt::WString term_text_str = Wt::WString::fromUTF8(term_text_value.getString());
            
            const Json::Value preferred_term_value = term_object.getValue("preferred");
            bool preferred_term = (0 == preferred_term_value.getString().compare("yes"));
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
    m_nor_term_panel->show();
    m_nor_term_panel->expand();
    m_eng_term_panel->show();
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
    
    m_layout_is_cleared = false;
}

void ESPoCApplication::TabChanged(int active_tab_index)
{
    if (1 == active_tab_index)
    {
        PopulateHierarchy();
    }
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

void ESPoCApplication::PopulateHierarchy()
{
    Wt::WString query = Wt::WString::tr("HierarchyTopNodesQuery");

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

    int row = 0;
    Json::Array::const_iterator iterator = hits_array.begin();
    for (; iterator!=hits_array.end(); ++iterator)
    {
        const Json::Value hit_value = *iterator;
        const Json::Object hit_value_object = hit_value.getObject();
        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value id_value = source_object.getValue("id");
        if (id_value.getString().empty())
            continue;
        
        std::string tree_value;
        if (source_object.member("tree_numbers"))
        {
            tree_value = source_object.getValue("tree_numbers").getArray().first().getString();
        }
        
        const Json::Value name_value = source_object.getValue("name");
        
        std::stringstream node_text;
        node_text << name_value.getString();
        if (!tree_value.empty())
        {
            node_text << " [" << tree_value << "]";
        }

        Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::fromUTF8(node_text.str()));
        
        if (source_object.member("child_tree_numbers"))
        {
            Wt::WStandardItem* child_item = new Wt::WStandardItem(Wt::WString("ToDo"));
            item->setChild(0, 0, child_item);
        }
        
        item->setData(boost::any(id_value.getString()), Wt::UserRole);
        m_hierarchy_model->setItem(row++, 0, item);
    }
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
    m_nor_term_panel->setTitle("");
    m_nor_term_panel->expand();
    m_nor_term_panel->hide();
    m_nor_term_panel_layout->clear();
    
    m_eng_term_panel->setTitle("");
    m_eng_term_panel->collapse();
    m_eng_term_panel->hide();
    m_eng_term_panel_layout->clear();

    m_description_text->setText("");
    m_description_text->hide();

    m_mesh_id_text->setText("");
    m_mesh_id_text->hide();

    m_links_layout->clear();

    m_layout_is_cleared = true;
}

void ESPoCApplication::CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str, bool add_wildcard) const
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
    
    if (add_wildcard && 0<filter_length && ' '!=cleaned_filter_str[filter_length-1])
    {
        cleaned_filter_str.append("*");
    }
}
