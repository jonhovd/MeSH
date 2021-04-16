#include "mesh_resultlist.h"

#include <boost/locale/conversion.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <Wt/WAnchor.h>

#include "application.h"


MeshResultList::MeshResultList(MeSHApplication* mesh_application)
: Wt::WContainerWidget(),
  m_mesh_application(mesh_application)
{
  auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);
  m_layout = setLayout(std::move(layout));
}

void MeshResultList::ClearLayout()
{
}

void MeshResultList::OnSearch(const Wt::WString& filter)
{
  auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);
  m_layout = setLayout(std::move(layout));

  m_mesh_application->ClearLayout();
    
  std::string filter_str = filter.toUTF8();
  const std::string lowercase_filter_str = boost::locale::to_lower(filter_str);
  std::string cleaned_filter_str;
  Search::CleanFilterString(filter_str, cleaned_filter_str);

  Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(RESULTLIST_COUNT).arg(filter);

  Json::Object search_result;
  auto es_util = m_mesh_application->GetElasticSearchUtil();
  long result_size = es_util->search("mesh", query.toUTF8(), search_result);

  int row = 0;
  if (0 == result_size)
  {
    AppendHit("", Wt::WString::tr("NoHits").toUTF8(), "");
  }
  else
  {
    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    Json::Array::const_iterator iterator = hits_array.begin();
    for (row=0; row<RESULTLIST_COUNT && iterator!=hits_array.end(); row++)
    {
      const Json::Value hit_value = *iterator;
      const Json::Object hit_value_object = hit_value.getObject();
      const Json::Value source_value = hit_value_object.getValue("_source");
      const Json::Object source_object = source_value.getObject();

      std::string name_str, id_str;
      Search::InfoFromSourceObject(source_object, name_str, &id_str);

      const std::string lowercase_name_str = boost::locale::to_lower(name_str);
      std::string indirect_hit_str;
      if (std::string::npos == lowercase_name_str.find(lowercase_filter_str))
      {
        Search::FindIndirectHit(source_object, cleaned_filter_str, indirect_hit_str);
      }

      std::string description_str;
      if (source_object.member("nor_description"))
      {
        description_str = source_object.getValue("nor_description").getString();
      }
      else if (source_object.member("eng_description"))
      {
        description_str = source_object.getValue("eng_description").getString();
      }
      else
      {
        description_str = name_str;
      }
      
      boost::algorithm::replace_all(description_str, "\\n", "\n");

      if (!indirect_hit_str.empty())
      {
        boost::algorithm::replace_all(indirect_hit_str, "\\n", "");
        AppendHit(id_str, Wt::WString::tr("IndirectHit").arg(name_str).arg(indirect_hit_str).toUTF8(), description_str);
      }
      else
      {
        AppendHit(id_str, name_str, description_str);
      }

      ++iterator;
    }
  }
}

void MeshResultList::AppendHit(const std::string& mesh_id, const std::string& title, const std::string description)
{
  auto result_container = Wt::cpp14::make_unique<Wt::WContainerWidget>();
  result_container->setStyleClass("result");
  auto result_vbox = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
  result_vbox->setContentsMargins(0, 0, 0, 0);

  std::string url = (Wt::WString::tr("MeshIdInternalPath")+"&"+Wt::WString::tr("MeshIdInternalPathParam").arg(mesh_id)).toUTF8();
  auto title_anchor = Wt::cpp14::make_unique<Wt::WAnchor>(Wt::WLink(Wt::LinkType::InternalPath, url), Wt::WString::fromUTF8(title));
  title_anchor->setStyleClass("mesh-link");
  result_vbox->addWidget(std::move(title_anchor));

  auto description_text = Wt::cpp14::make_unique<Wt::WText>(Wt::WString::fromUTF8(description));
  description_text->setStyleClass("result-description");
  result_vbox->addWidget(std::move(description_text));

  result_container->setLayout(std::move(result_vbox));
  m_layout->addWidget(std::move(result_container));
}
