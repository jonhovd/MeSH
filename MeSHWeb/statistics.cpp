#include "statistics.h"

#include <Wt/WPanel.h>

#include "application.h"
#include "elasticsearchutil.h"


Statistics::Statistics(const Wt::WString& text, MeSHApplication* mesh_application)
: Wt::WTemplate(text),
  m_mesh_application(mesh_application),
  m_content_is_populated(false)
{
  auto content = std::make_unique<Wt::WContainerWidget>();
  m_content = content.get();
  
  auto panel = std::make_unique<Wt::WPanel>();
  panel->setTitle(Wt::WString::tr("Statistics"));
  panel->setCentralWidget(std::move(content));
  panel->setCollapsible(true);
  panel->setCollapsed(true);
  panel->expanded().connect(this, &Statistics::Populate);
  
  bindWidget("statistics", std::move(panel));
}

void Statistics::Populate()
{
  if (m_content_is_populated)
    return;
  
  auto layout = std::make_unique<Wt::WGridLayout>();
  layout->setContentsMargins(0, 9, 0, 0);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 0);

  int row = 0;
  PopulateDayStatistics(layout, row);
  layout->addWidget(std::make_unique<Wt::WText>(""), row++, 0);
  PopulateTextStatistics(layout, row);
  m_content->setLayout(std::move(layout));
  m_content_is_populated = true;
}

void Statistics::PopulateDayStatistics(std::unique_ptr<Wt::WGridLayout>& layout, int& row)
{
  layout->addWidget(std::make_unique<Wt::WText>(Wt::WString::tr("StatisticsPerDay")), row++, 0);

  Wt::WString query = Wt::WString::tr("StatisticsDay");

  Json::Object search_result;
  auto es_util = m_mesh_application->GetElasticSearchUtil();
  long result_size = es_util->search("day_statistics", query.toUTF8(), search_result);
  if (0 == result_size)
  {
    return;
  }
  const Json::Value value = search_result.getValue("hits");
  const Json::Object value_object = value.getObject();
  const Json::Value hits_value = value_object.getValue("hits");
  const Json::Array hits_array = hits_value.getArray();

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

    layout->addWidget(std::make_unique<Wt::WText>(day_value_string), row, 0);
    layout->addWidget(std::make_unique<Wt::WText>(Wt::WString("{1}").arg(count_value_int)), row++, 1, Wt::AlignmentFlag::Right);
  }
}

void Statistics::PopulateTextStatistics(std::unique_ptr<Wt::WGridLayout>& layout, int& row)
{
  layout->addWidget(std::make_unique<Wt::WText>(Wt::WString::tr("StatisticsPerMeSH")), row++, 0);

  Wt::WString query = Wt::WString::tr("StatisticsText");

  Json::Object search_result;
  auto es_util = m_mesh_application->GetElasticSearchUtil();
  long result_size = es_util->search("text_statistics", query.toUTF8(), search_result);
  if (0 == result_size)
  {
    return;
  }
  const Json::Value value = search_result.getValue("hits");
  const Json::Object value_object = value.getObject();
  const Json::Value hits_value = value_object.getValue("hits");
  const Json::Array hits_array = hits_value.getArray();

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
    SearchTab::MeSHToName(es_util, mesh_value_string, name);
    layout->addWidget(std::make_unique<Wt::WText>(name), row, 0);
    layout->addWidget(std::make_unique<Wt::WText>(Wt::WString("{1}").arg(count_value_int)), row++, 1, Wt::AlignmentFlag::Right);
    row++;
  }
}
