#include "mesh_resultlist.h"

#include <boost/locale/conversion.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <Wt/WAnchor.h>

#include "application.h"
#include "search.h"


MeshResultList::MeshResultList(MeSHApplication* mesh_application)
: Wt::WContainerWidget(),
  m_mesh_application(mesh_application)
{
	auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);
	m_layout = setLayout(std::move(layout));
}

MeshResultList::~MeshResultList()
{
}

void MeshResultList::ClearLayout()
{
	hide();
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
    std::string wildcard_filter_str;
    Search::AddWildcard(cleaned_filter_str, wildcard_filter_str);
    
    Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(RESULTLIST_COUNT).arg(filter).arg(wildcard_filter_str);

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);

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
            
            const Json::Value id_value = source_object.getValue("id");
            const Json::Value name_value = source_object.getValue("name");

            const std::string lowercase_name_value_str = boost::locale::to_lower(name_value.getString());
            std::string indirect_hit_str;
            if (std::string::npos == lowercase_name_value_str.find(lowercase_filter_str))
            {
                Search::FindIndirectHit(source_object, cleaned_filter_str, indirect_hit_str);
            }

            std::string description;
			if (source_object.member("concepts"))
			{
				const Json::Value concepts_value = source_object.getValue("concepts");
				const Json::Array concepts_array = concepts_value.getArray();

				Json::Array::const_iterator concept_iterator = concepts_array.begin();
				for (; concept_iterator!=concepts_array.end(); ++concept_iterator)
				{
					const Json::Value concept_value = *concept_iterator;
					const Json::Object concept_object = concept_value.getObject();
					const Json::Value preferred_concept_value = concept_object.getValue("preferred");
					bool preferred_concept = (EQUAL == preferred_concept_value.getString().compare("yes"));
					if (preferred_concept)
					{
						Json::Value description_value;
						if (concept_object.member("description"))
						{
							description_value = concept_object.getValue("description");
						}
						else if (concept_object.member("english_description"))
						{
							description_value = concept_object.getValue("english_description");
						}
						else
						{
							continue;
						}
						description = description_value.getString();
						boost::algorithm::replace_all(description, "\\n", "\n");
					}
				}
			}

            if (!indirect_hit_str.empty())
            {
				AppendHit(id_value.getString(), Wt::WString::tr("IndirectHit").arg(name_value.getString()).arg(indirect_hit_str).toUTF8(), description);
            }
            else
            {
				AppendHit(id_value.getString(), name_value.getString(), description);
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
