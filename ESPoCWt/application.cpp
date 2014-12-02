#include "application.h"

#include <boost/algorithm/string/replace.hpp>
#include <Wt/Utils>
#include <Wt/WAnchor>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WImage>
#include <Wt/WStandardItem>
#include <Wt/WStringListModel>
#include <Wt/WVBoxLayout>

#define SUGGESTION_COUNT	(20)
#define LANGUAGE            "nor"

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

#define EQUAL                           (0)

#define TAB_INDEX_SEARCH                (0)
#define TAB_INDEX_HIERARCHY             (1)

#define SUGGESTIONLIST_ITEM_ID_ROLE     (Wt::UserRole)
#define HIERARCHY_ITEM_TREE_NUMBER_ROLE (Wt::UserRole+1)
#define HIERARCHY_ITEM_ID_ROLE          (Wt::UserRole+2)


ESPoCApplication::ESPoCApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_signal(this, "search"),
  m_has_populated_hierarchy_model(false),
  m_hierarchy_popup_menu(NULL)
{
    messageResourceBundle().use(appRoot() + "strings");
    useStyleSheet(Wt::WLink("MeSH.css"));

	m_es = new ElasticSearch("localhost:9200");

	setTitle(Wt::WString::tr("AppName"));

    Wt::WImage* logo = new Wt::WImage("images/logo.png");
    root()->addWidget(logo);

    Wt::WText* header = new Wt::WText(Wt::WString::tr("AppName"));
    header->setStyleClass("mesh-header");
    header->setPadding(Wt::WLength(3.0, Wt::WLength::FontEm), Wt::Left);
    root()->addWidget(header);

    m_tab_widget = new Wt::WTabWidget(root());

    m_tab_widget->addTab(CreateSearchTab(), Wt::WString::tr("Search"));

    //Hierarchy
    Wt::WContainerWidget* hierarchy_tab = new Wt::WContainerWidget();
    Wt::WVBoxLayout* hierarchy_vbox = new Wt::WVBoxLayout();
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


    ClearLayout();

    TabChanged(TAB_INDEX_SEARCH);
}

ESPoCApplication::~ESPoCApplication()
{
    delete m_hierarchy_popup_menu;
    delete m_es;
}

Wt::WContainerWidget* ESPoCApplication::CreateSearchTab()
{
    Wt::WContainerWidget* search_tab = new Wt::WContainerWidget();
    Wt::WVBoxLayout* search_vbox = new Wt::WVBoxLayout();
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
 
    m_mesh_id_text = new Wt::WText();
    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("MeSH_ID")));
    result_vbox->addWidget(m_mesh_id_text);

    m_links_layout = new Wt::WGridLayout();
    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("Links")));
    result_vbox->addLayout(m_links_layout);
    
    search_vbox->addWidget(m_result_container);

    return search_tab;
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
			item->setData(boost::any(id_value.getString()), SUGGESTIONLIST_ITEM_ID_ROLE);
			m_search_suggestion_model->setItem(row, 0, item);

			++iterator;
		}

        m_search_suggestion_model->sort(0);

        if (hits_array.size() > SUGGESTION_COUNT)
        {
            Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::tr("MoreHits"));
            item->setData(boost::any(), SUGGESTIONLIST_ITEM_ID_ROLE);
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
}

void ESPoCApplication::CollapseHierarchy()
{
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
}

void ESPoCApplication::ExpandToTreeNumber(const std::string& tree_number_string)
{
    Wt::WModelIndex model_index;
    ExpandTreeNumberRecursive(tree_number_string, model_index);
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
    int row = 0;
    Wt::WModelIndex child_index;
    Wt::WStandardItem* standard_item;
    std::string item_tree_number_string;
    while (true)
    {
        child_index = top_level ? m_hierarchy_model->index(row, 0) : index.child(row, 0);
        if (!child_index.isValid())
            return false;

        standard_item = m_hierarchy_model->itemFromIndex(child_index);
        if (!standard_item)
            return false;

        item_tree_number_string = boost::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE));
        if (EQUAL == tree_number_string.compare(item_tree_number_string))
        {
            index = child_index;
            return true;
        }

        row++;
    }
}

void ESPoCApplication::SearchEditFocussed()
{
    m_search_edit->setText("");
}

void ESPoCApplication::TabChanged(int active_tab_index)
{
    if (TAB_INDEX_SEARCH >= active_tab_index)
    {
        m_search_edit->setFocus();
    }
    else
    {
        PopulateHierarchy();
    }
}

void ESPoCApplication::TreeItemExpanded(const Wt::WModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(index);
    if (!standard_item || !standard_item->hasChildren())
    {
        return;
    }

    Wt::WStandardItem* possible_placeholder = standard_item->child(0, 0);
    if (!possible_placeholder || //We don't have a children placeholder. This item should not be populated by children 
        !possible_placeholder->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE).empty()) //This is a real child, not a placeholder. No need to populate children one more time.
    {
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
}

void ESPoCApplication::TreeItemClicked(const Wt::WModelIndex& index, const Wt::WMouseEvent& mouse)
{
    if (!index.isValid())
    {
        return;
    }
    
    Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(index);
    if (!standard_item)
        return;
    
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
}

void ESPoCApplication::PopupMenuTriggered(Wt::WMenuItem* item)
{
    if (item && !m_popup_menu_id_string.empty())
    {
        ClearLayout();
        m_tab_widget->setCurrentIndex(TAB_INDEX_SEARCH);
        Search(m_popup_menu_id_string);
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
    if (m_has_populated_hierarchy_model)
    {
        return;
    }

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

void ESPoCApplication::GetParentTreeNumber(const std::string& child_tree_number, std::string& parent_tree_number)
{
    size_t substring_length = child_tree_number.find_last_of('.');
    parent_tree_number = (std::string::npos==substring_length) ? "" : child_tree_number.substr(0, substring_length);
}

bool ESPoCApplication::AddChildPlaceholderIfNeeded(const Json::Object& source_object, const std::string& current_tree_number_string, Wt::WStandardItem* current_item)
{
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
    return added_placeholder;
}
