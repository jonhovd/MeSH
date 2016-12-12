#include "statistics.h"

#include <Wt/WText>

#include "application.h"
#include "elasticsearchutil.h"


Statistics::Statistics(const MeSHApplication* mesh_application, Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
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
		m_layout->clear();
		m_layout = nullptr;
	}
	Wt::WContainerWidget::clear();
}

void Statistics::populate()
{
	clear();

	m_layout = new Wt::WGridLayout();
    m_layout->setContentsMargins(0, 9, 0, 0);
    setLayout(m_layout);

    int i;
    for (i=0; i<8; i++)
    {
        m_layout->setColumnStretch(i, 0);
    }
    for (i=0; i<=8; i+=4)
    {
        m_layout->setColumnStretch(i, 1);
        m_layout->addWidget(new Wt::WText(""), 0, i);
    }

    PopulateDayStatistics();
    PopulateTextStatistics();
}

void Statistics::PopulateDayStatistics()
{
    m_layout->addWidget(new Wt::WText(Wt::WString::tr("StatisticsPerDay")), 0, 1);

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

        m_layout->addWidget(new Wt::WText(day_value_string), row, 2);
        m_layout->addWidget(new Wt::WText(Wt::WString("{1}").arg(count_value_int)), row, 3);
        row++;
    }
}

void Statistics::PopulateTextStatistics()
{
    m_layout->addWidget(new Wt::WText(Wt::WString::tr("StatisticsPerMeSH")), 0, 5);

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
        MeSHToName(mesh_value_string, name);
        m_layout->addWidget(new Wt::WText(name), row, 6);
        m_layout->addWidget(new Wt::WText(Wt::WString("{1}").arg(count_value_int)), row, 7);
        row++;
    }
}

void Statistics::MeSHToName(const std::string& mesh_id, std::string& name) const
{
    name = mesh_id;

    Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id);
    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
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
