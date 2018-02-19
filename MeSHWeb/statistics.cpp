#include "statistics.h"

#include <Wt/WText.h>

#include "application.h"
#include "elasticsearchutil.h"
#include "search.h"


Statistics::Statistics(const MeSHApplication* mesh_application)
: Wt::WContainerWidget(),
  m_mesh_application(mesh_application),
  m_layout(nullptr)
{
}

Statistics::~Statistics()
{
}

void Statistics::clear()
{
	if (m_layout)
	{
		m_layout = setLayout(Wt::cpp14::make_unique<Wt::WGridLayout>());
	}
	Wt::WContainerWidget::clear();
}

void Statistics::populate()
{
	clear();

	auto layout = Wt::cpp14::make_unique<Wt::WGridLayout>();
    layout->setContentsMargins(0, 9, 0, 0);
    m_layout = setLayout(std::move(layout));

    int i;
    for (i=0; i<8; i++)
    {
        m_layout->setColumnStretch(i, 0);
    }
    for (i=0; i<=8; i+=4)
    {
        m_layout->setColumnStretch(i, 1);
        m_layout->addWidget(Wt::cpp14::make_unique<Wt::WText>(""), 0, i);
    }

    PopulateDayStatistics();
    PopulateTextStatistics();
}

void Statistics::PopulateDayStatistics()
{
    m_layout->addWidget(Wt::cpp14::make_unique<Wt::WText>(Wt::WString::tr("StatisticsPerDay")), 0, 1);

    Wt::WString query = Wt::WString::tr("StatisticsDay");

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("statistics", "day", query.toUTF8(), search_result);
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

        const Json::Value day_value = hit_value_object.getValue("_id");
        std::string day_value_string = day_value.getString();

        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value count_value = source_object.getValue("count");
        int count_value_int = count_value.getInt();

        m_layout->addWidget(Wt::cpp14::make_unique<Wt::WText>(day_value_string), row, 2);
        m_layout->addWidget(Wt::cpp14::make_unique<Wt::WText>(Wt::WString("{1}").arg(count_value_int)), row, 3);
        row++;
    }
}

void Statistics::PopulateTextStatistics()
{
    m_layout->addWidget(Wt::cpp14::make_unique<Wt::WText>(Wt::WString::tr("StatisticsPerMeSH")), 0, 5);

    Wt::WString query = Wt::WString::tr("StatisticsText");

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("statistics", "text", query.toUTF8(), search_result);
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

        const Json::Value mesh_value = hit_value_object.getValue("_id");
        std::string mesh_value_string = mesh_value.getString();

        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value count_value = source_object.getValue("count");
        int count_value_int = count_value.getInt();

        std::string name;
        Search::MeSHToName(es_util, mesh_value_string, name);
        m_layout->addWidget(Wt::cpp14::make_unique<Wt::WText>(name), row, 6);
        m_layout->addWidget(Wt::cpp14::make_unique<Wt::WText>(Wt::WString("{1}").arg(count_value_int)), row, 7);
        row++;
    }
}
