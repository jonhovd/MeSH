#include "search.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/locale/conversion.hpp>
#include <Wt/Utils>
#include <Wt/WCssDecorationStyle>
#include <Wt/WHBoxLayout>
#include <Wt/WImage>
#include <Wt/WStringListModel>

#include "application.h"
#include "log.h"


Search::Search(MeSHApplication* mesh_application, Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
  m_mesh_application(mesh_application)
{
    m_layout = new Wt::WVBoxLayout();
    m_layout->setContentsMargins(0, 9, 0, 0);
    setLayout(m_layout);

    m_search_edit = new Wt::WLineEdit();
	m_search_edit->setStyleClass("search-field");
    m_search_edit->focussed().connect(this, &Search::SearchEditFocussed);
    m_search_suggestion = CreateSuggestionPopup(this);
    m_search_suggestion->forEdit(m_search_edit);
    m_search_suggestion_model = new Wt::WStandardItemModel(m_layout);
    m_search_suggestion->setModel(m_search_suggestion_model);
    m_search_suggestion->setFilterLength(2);
    m_search_suggestion->filterModel().connect(this, &Search::FilterSuggestion);
    m_search_suggestion_model->itemChanged().connect(this, &Search::SuggestionChanged);
    m_layout->addWidget(m_search_edit);

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

	Wt::WText* nor_description_label = new Wt::WText(Wt::WString::tr("NorwegianDescription"));
	nor_description_label->setStyleClass("scope scope-class");
    result_vbox->addWidget(nor_description_label);
    m_nor_description_text = new Wt::WText();
	m_nor_description_text->setStyleClass("scope-note scope-class");
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

    Wt::WText* eng_description_label = new Wt::WText(Wt::WString::tr("EnglishDescription"));
	eng_description_label->setStyleClass("scope scope-class");
    result_vbox->addWidget(eng_description_label);
    m_eng_description_text = new Wt::WText();
	m_eng_description_text->setStyleClass("scope-note scope-class");
    result_vbox->addWidget(m_eng_description_text);
 
    Wt::WContainerWidget* mesh_id_container = new Wt::WContainerWidget();
    Wt::WHBoxLayout* mesh_id_hbox = new Wt::WHBoxLayout();
    mesh_id_hbox->setContentsMargins(0, 0, 0, 0);
    mesh_id_container->setLayout(mesh_id_hbox);
    m_mesh_id_text = new Wt::WText();
    mesh_id_hbox->addWidget(new Wt::WText(Wt::WString::tr("MeSH_ID")), 0, Wt::AlignLeft);
    mesh_id_hbox->addWidget(m_mesh_id_text, 1, Wt::AlignLeft);
    result_vbox->addWidget(mesh_id_container);

    m_links = new Links();
//    result_vbox->addWidget(new Wt::WText(Wt::WString::tr("Links")));
    result_vbox->addWidget(m_links);
    
    m_layout->addWidget(m_result_container);
}

Search::~Search()
{
}

void Search::FocusSearchEdit()
{
	m_search_edit->setFocus(true);
}

void Search::OnSearch(const Wt::WString& mesh_id)
{
    Wt::WString preferred_eng_term;

    m_mesh_id_text->setText(mesh_id);

    Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id.toUTF8());

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        return;
    }
    
    Log log(m_mesh_application);
	log.LogSearch(mesh_id.toUTF8());

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
	if (!source_object.member("concepts")) //Probably a top-node
		return;

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
	
	m_links->populate(mesh_id, url_encoded_term, url_encoded_filtertext);
    
    delete non_preferred_nor_terms;
    delete non_preferred_eng_terms;
    
	Hierarchy* hierarchy = m_mesh_application->GetHierarchy();
	hierarchy->ClearMarkedItems();
	hierarchy->Collapse();

	if (source_object.member("tree_numbers"))
	{
		const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
		const Json::Array tree_numbers_array = tree_numbers_value.getArray();
		Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
		for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
		{
			const Json::Value tree_number_value = *tree_numbers_iterator;
			hierarchy->ExpandToTreeNumber(tree_number_value.getString());
		}
	}
}

void Search::ClearLayout()
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

    m_links->clear();

//    m_result_container->hide();
}

void Search::SearchEditFocussed()
{
    m_search_edit->setText("");
}

void Search::SuggestionChanged(Wt::WStandardItem* item)
{
	if (item)
	{
		item = NULL;
	}
}

void Search::FilterSuggestion(const Wt::WString& filter)
{
	m_mesh_application->ClearLayout();
    
	m_search_suggestion_model->clear();

    std::string filter_str = filter.toUTF8();
    const std::string lowercase_filter_str = boost::locale::to_lower(filter_str);
    std::string cleaned_filter_str;
    CleanFilterString(filter_str, cleaned_filter_str);
    std::string wildcard_filter_str;
    AddWildcard(cleaned_filter_str, wildcard_filter_str);
    
    Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(SUGGESTION_COUNT+1 /* +1 to see if we got more than SUGGESTION_COUNT hits */).arg(filter).arg(wildcard_filter_str);

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);

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
}

Wt::WSuggestionPopup* Search::CreateSuggestionPopup(Wt::WContainerWidget* parent)
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

    Wt::WSuggestionPopup* popup = new Wt::WSuggestionPopup(matcherJS, replacerJS, parent);
	popup->setJavaScriptMember("wtNoReparent", "true");
    return popup;
}

void Search::CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str) const
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

void Search::AddWildcard(const std::string filter_str, std::string& wildcard_filter_str) const
{
    wildcard_filter_str = filter_str;
    size_t filter_length = wildcard_filter_str.length();
    if (0<filter_length && ' '!=wildcard_filter_str[filter_length-1])
    {
        wildcard_filter_str.append("*");
    }
}

void Search::FindIndirectHit(const std::string& haystack, const std::string& needles, double& best_hit_factor, std::string& indirect_hit_str)
{
    const std::string lowercase_haystack = boost::locale::to_lower(haystack);
    size_t haystack_length = haystack.length();
    if (0 == haystack_length)
    {
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
}

void Search::FindIndirectHit(const Json::Object& source_object, const std::string& cleaned_filter_str, std::string& indirect_hit_str)
{
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
}
