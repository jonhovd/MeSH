#include "search.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/locale/conversion.hpp>
#include <Wt/WHBoxLayout.h>
#include <Wt/WPushButton.h>
#include <Wt/WVBoxLayout.h>

#include "application.h"


Search::Search(MeSHApplication* mesh_application)
: Wt::WContainerWidget(),
  m_mesh_application(mesh_application)
{
  auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);

  auto searchform_container = Wt::cpp14::make_unique<Wt::WContainerWidget>();
  searchform_container->setStyleClass("search-box");
  auto searchform_hbox = Wt::cpp14::make_unique<Wt::WHBoxLayout>();

  auto search_edit = Wt::cpp14::make_unique<Wt::WLineEdit>();
  search_edit->setTextSize(20); //HTML default value
  search_edit->setToolTip(Wt::WString::tr("SearchTooltip"));
  search_edit->focussed().connect(this, &Search::OnSearchEditFocussed);

  m_search_suggestion = CreateSuggestionPopup();
  m_search_suggestion->forEdit(search_edit.get());
  m_search_suggestion_model = std::make_shared<Wt::WStandardItemModel>();
  m_search_suggestion->setModel(m_search_suggestion_model);
  m_search_suggestion->setFilterLength(2);

  m_search_suggestion->filterModel().connect(this, &Search::FilterSuggestion);
  m_search_suggestion_model->itemChanged().connect(this, &Search::SuggestionChanged);

  m_search_edit = searchform_hbox->addWidget(std::move(search_edit), 0, Wt::AlignmentFlag::Left);

  auto search_button = Wt::cpp14::make_unique<Wt::WPushButton>(Wt::WString::tr("SearchButton"));
  search_button->setToolTip(Wt::WString::tr("SearchbuttonTooltip"));
  search_button->clicked().connect(this, &Search::SearchButtonClicked);
  searchform_hbox->addWidget(std::move(search_button), 0, Wt::AlignmentFlag::Left);
  searchform_hbox->addStretch(1);

  searchform_container->setLayout(std::move(searchform_hbox));
  layout->addWidget(std::move(searchform_container));

  auto stack_widget = Wt::cpp14::make_unique<Wt::WStackedWidget>();
  m_stacked_widget = stack_widget.get();
  m_mesh_result = m_stacked_widget->addWidget(Wt::cpp14::make_unique<MeshResult>(mesh_application)); //TAB_INDEX_RESULT
  m_mesh_resultlist = m_stacked_widget->addWidget(Wt::cpp14::make_unique<MeshResultList>(mesh_application)); //TAB_INDEX_RESULTLIST
  layout->addWidget(std::move(stack_widget), 1);

  layout->addStretch(1);

  setLayout(std::move(layout));
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

  m_stacked_widget->setCurrentIndex(TAB_INDEX_RESULT);
}

void Search::SearchButtonClicked()
{
	m_mesh_resultlist->OnSearch(m_search_edit->text().toUTF8());

  m_stacked_widget->setCurrentIndex(TAB_INDEX_RESULTLIST);
}

void Search::OnSearchEditFocussed()
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

  Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(SUGGESTION_COUNT+1 /* +1 is to see if we got more than SUGGESTION_COUNT hits */).arg(filter).arg(wildcard_filter_str);

  Json::Object search_result;
	auto es_util = m_mesh_application->GetElasticSearchUtil();
  long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);

  int row = 0;
  if (0 == result_size)
  {
    auto item = Wt::cpp14::make_unique<Wt::WStandardItem>(Wt::WString::tr("NoHits"));
    item->setData(Wt::cpp17::any(), SUGGESTIONLIST_ITEM_ID_ROLE);
    m_search_suggestion_model->setItem(row++, 0, std::move(item));
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
      std::unique_ptr<Wt::WStandardItem> item;
      if (std::string::npos == lowercase_name_value_str.find(lowercase_filter_str))
      {
        FindIndirectHit(source_object, cleaned_filter_str, indirect_hit_str);
      }

      if (!indirect_hit_str.empty())
      {
        item = Wt::cpp14::make_unique<Wt::WStandardItem>(Wt::WString::tr("IndirectHit").arg(name_value.getString()).arg(indirect_hit_str));
      }
      else
      {
        item = Wt::cpp14::make_unique<Wt::WStandardItem>(Wt::WString::fromUTF8(name_value.getString()));
      }
      item->setData(Wt::cpp17::any(id_value.getString()), SUGGESTIONLIST_ITEM_ID_ROLE);
      m_search_suggestion_model->setItem(row, 0, std::move(item));

      ++iterator;
    }

    m_search_suggestion_model->sort(0);

    if (hits_array.size() > SUGGESTION_COUNT)
    {
      auto item = Wt::cpp14::make_unique<Wt::WStandardItem>(Wt::WString::tr("MoreHits").arg(SUGGESTION_COUNT));
      item->setData(Wt::cpp17::any(), SUGGESTIONLIST_ITEM_ID_ROLE);
      m_search_suggestion_model->setItem(row++, 0, std::move(item));
    }
  }

  m_search_suggestion_model->setData(--row, 0, std::string("Wt-more-data"), Wt::ItemDataRole::StyleClass);
}

Wt::WSuggestionPopup* Search::CreateSuggestionPopup()
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

  Wt::WSuggestionPopup* popup = new Wt::WSuggestionPopup(matcherJS, replacerJS);
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
      boost::algorithm::replace_all(description_str, "\\n", "\n");
      FindIndirectHit(description_str, cleaned_filter_str, best_hit_factor, indirect_hit_str);
    }
    if (concept_object.member("english_description"))
    {
      const Json::Value description_value = concept_object.getValue("english_description");
      std::string description_str = description_value.getString();
      boost::algorithm::replace_all(description_str, "\\n", "\n");
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

void Search::MeSHToName(std::shared_ptr<ElasticSearchUtil> es_util, const std::string& mesh_id, std::string& name)
{
  name = mesh_id;

  Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id);
  Json::Object search_result;
  long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);
  if (0 == result_size)
  {
    return;
  }

  InfoFromSearchResult(search_result, name);
}

void Search::TreeNumberToName(std::shared_ptr<ElasticSearchUtil> es_util, const std::string& tree_number, std::string& name, std::string* mesh_id)
{
	name = tree_number;

	Wt::WString query = Wt::WString::tr("HierarchyTreeNodeQuery").arg(tree_number);
	Json::Object search_result;
	long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);
	if (0 == result_size)
	{
		return;
	}

	InfoFromSearchResult(search_result, name, mesh_id);
}

void Search::InfoFromSearchResult(const Json::Object& search_result, std::string& name, std::string* mesh_id)
{
  const Json::Value value = search_result.getValue("hits");
  const Json::Object value_object = value.getObject();
  const Json::Value hits_value = value_object.getValue("hits");
  const Json::Array hits_array = hits_value.getArray();
  const Json::Value hit_value = hits_array.first();
  const Json::Object hit_value_object = hit_value.getObject();
  const Json::Value source_value = hit_value_object.getValue("_source");
  const Json::Object source_object = source_value.getObject();

  if (nullptr != mesh_id)
  {
    const Json::Value id_value = source_object.getValue("id");
    *mesh_id = id_value.getString();
  }
  if (!source_object.member("concepts")) //Probably a top-node
  {
    const Json::Value name_value = source_object.getValue("name");
    name = name_value.getString();
    return;
  }

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
