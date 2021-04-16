#include "search.h"

#include <boost/algorithm/string/replace.hpp>
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
  if (filter_str.empty())
  {
    return;
  }
  
  const std::string lowercase_filter_str = boost::locale::to_lower(filter_str);
  std::string cleaned_filter_str;
  CleanFilterString(filter_str, cleaned_filter_str);

  Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(SUGGESTION_COUNT+1 /* +1 is to see if we got more than SUGGESTION_COUNT hits */).arg(filter);

  Json::Object search_result;
	auto es_util = m_mesh_application->GetElasticSearchUtil();
  long result_size = es_util->search("mesh", query.toUTF8(), search_result);

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
      std::string name_str;
      InfoFromSourceObject(source_object, name_str);

      const std::string lowercase_name_str = boost::locale::to_lower(name_str);
      std::string indirect_hit_str;
      std::unique_ptr<Wt::WStandardItem> item;
      if (std::string::npos == lowercase_name_str.find(lowercase_filter_str))
      {
        FindIndirectHit(source_object, cleaned_filter_str, indirect_hit_str);
      }

      if (!indirect_hit_str.empty())
      {
        boost::algorithm::replace_all(indirect_hit_str, "\\n", "");
        item = Wt::cpp14::make_unique<Wt::WStandardItem>(Wt::WString::tr("IndirectHit").arg(name_str).arg(indirect_hit_str.substr(0, 100))); //Trim at 100 characters
      }
      else
      {
        item = Wt::cpp14::make_unique<Wt::WStandardItem>(Wt::WString::fromUTF8(name_str));
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

  const std::string search_fields[] = {"id", "other_ids", "nor_name", "nor_preferred_term_text", "nor_description", "eng_name", "eng_preferred_term_text", "eng_description",
                                       "nor_other_term_texts", "eng_other_term_texts", "see_related", "tree_numbers", "parent_tree_numbers", "child_tree_numbers"};

  for (size_t i=0; i<sizeof(search_fields)/sizeof(search_fields[0]); i++)
  {
    if (!source_object.member(search_fields[i]))
      continue;
    
    const Json::Value search_value = source_object.getValue(search_fields[i]);
    if (search_value.isArray())
    {
      Json::Array search_values = search_value.getArray();
      Json::Array::const_iterator search_value_iterator = search_values.begin();
      for (; search_value_iterator!=search_values.end(); ++search_value_iterator)
      {
        FindIndirectHit((*search_value_iterator).getString(), cleaned_filter_str, best_hit_factor, indirect_hit_str);
      }
    }
    else
    {
      FindIndirectHit(search_value.getString(), cleaned_filter_str, best_hit_factor, indirect_hit_str);
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

void Search::MeSHToName(std::shared_ptr<ElasticSearchUtil> es_util, const std::string& mesh_id, std::string& name)
{
  name = mesh_id;

  Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id);
  Json::Object search_result;
  long result_size = es_util->search("mesh", query.toUTF8(), search_result);
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
	long result_size = es_util->search("mesh", query.toUTF8(), search_result);
	if (0 == result_size)
	{
		return;
	}

	InfoFromSearchResult(search_result, name, mesh_id);
}

void Search::InfoFromSourceObject(const Json::Object& source_object, std::string& name, std::string* mesh_id)
{
  if (nullptr != mesh_id)
  {
    *mesh_id = source_object.getValue("id").getString();
  }

  if (source_object.member("nor_name"))
  {
    name = source_object.getValue("nor_name").getString();
  }
  else if (source_object.member("eng_name"))
  {
    name = source_object.getValue("eng_name").getString();
  }
  else if (source_object.member("id"))
  {
    name = source_object.getValue("id").getString();
  }
  
  boost::algorithm::replace_all(name, "\\n", "");
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

  InfoFromSourceObject(source_object, name, mesh_id);
}
