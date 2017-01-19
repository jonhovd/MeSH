#include "search.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/locale/conversion.hpp>
#include <Wt/Utils>
#include <Wt/WHBoxLayout>
#include <Wt/WStringListModel>

#include "application.h"
#include "log.h"


Search::Search(MeSHApplication* mesh_application, Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
  m_mesh_application(mesh_application)
{
    m_layout = new Wt::WVBoxLayout();
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);

    Wt::WContainerWidget* searchform_container = new Wt::WContainerWidget();
	searchform_container->setStyleClass("search-box");
    Wt::WHBoxLayout*  searchform_hbox = new Wt::WHBoxLayout();
    searchform_container->setLayout(searchform_hbox);

	m_search_edit = new Wt::WLineEdit();
    m_search_edit->focussed().connect(this, &Search::SearchEditFocussed);
    m_search_suggestion = CreateSuggestionPopup(this);
    m_search_suggestion->forEdit(m_search_edit);
    m_search_suggestion_model = new Wt::WStandardItemModel(m_layout);
    m_search_suggestion->setModel(m_search_suggestion_model);
    m_search_suggestion->setFilterLength(2);
    m_search_suggestion->filterModel().connect(this, &Search::FilterSuggestion);
    m_search_suggestion_model->itemChanged().connect(this, &Search::SuggestionChanged);
    searchform_hbox->addWidget(m_search_edit, 1, Wt::AlignJustify);

	m_search_button = new Wt::WPushButton(Wt::WString::tr("SearchButton"));
	m_search_button->clicked().connect(this, &Search::SearchButtonClicked);
	searchform_hbox->addWidget(m_search_button, 0, Wt::AlignLeft);

	m_layout->addWidget(searchform_container);

	m_mesh_resultlist = new MeshResultList(mesh_application);
    m_layout->addWidget(m_mesh_resultlist);

	m_mesh_result = new MeshResult(mesh_application);
    m_layout->addWidget(m_mesh_result);
	
	m_layout->addStretch(1);
}

Search::~Search()
{
}

void Search::FocusSearchEdit()
{
	m_search_edit->setFocus(true);
}

void Search::ClearLayout()
{
	m_mesh_result->ClearLayout();
}

void Search::OnSearch(const Wt::WString& mesh_id)
{
	m_mesh_result->OnSearch(mesh_id, m_search_edit->text().toUTF8());

	m_mesh_resultlist->hide();
	m_mesh_result->show();
}

void Search::SearchButtonClicked()
{
	m_mesh_resultlist->OnSearch(m_search_edit->text().toUTF8());

	m_mesh_resultlist->show();
	m_mesh_result->hide();
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

void Search::CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str)
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

void Search::AddWildcard(const std::string filter_str, std::string& wildcard_filter_str)
{
    wildcard_filter_str = filter_str;
    size_t filter_length = wildcard_filter_str.length();
    if (0<filter_length && ' '!=wildcard_filter_str[filter_length-1])
    {
        wildcard_filter_str.append("*");
    }
}

void Search::MeSHToName(ElasticSearchUtil* es_util, const std::string& mesh_id, std::string& name)
{
    name = mesh_id;

    Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id);
    Json::Object search_result;
    long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);
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
