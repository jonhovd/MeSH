#include "mesh_resultlist.h"

#include <boost/locale/conversion.hpp>

#include "application.h"
#include "search.h"
/*
#include <boost/algorithm/string/split.hpp>
#include <Wt/Utils>
#include <Wt/WCssDecorationStyle>
#include <Wt/WHBoxLayout>
#include <Wt/WImage>
#include <Wt/WStringListModel>

#include "log.h"
*/

MeshResultList::MeshResultList(MeSHApplication* mesh_application, Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
  m_mesh_application(mesh_application)
{
	m_layout = new Wt::WVBoxLayout();
	m_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(m_layout);
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
	m_layout->clear();

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
            
            if (!indirect_hit_str.empty())
            {
				AppendHit(id_value.getString(), Wt::WString::tr("IndirectHit").arg(name_value.getString()).arg(indirect_hit_str).toUTF8(), "todo");
            }
            else
            {
				AppendHit(id_value.getString(), name_value.getString(), "todo");
            }

            ++iterator;
        }
    }
}

void MeshResultList::AppendHit(const std::string& mesh_id, const std::string& title, const std::string description)
{
	Wt::WContainerWidget* result_container = new Wt::WContainerWidget();
	result_container->setStyleClass("result");
	Wt::WVBoxLayout* result_vbox = new Wt::WVBoxLayout();
	result_container->setLayout(result_vbox);

	Wt::WAnchor* title_anchor = new Wt::WAnchor(Wt::WLink("/mesh/"+mesh_id), Wt::WString::fromUTF8(title));
	title_anchor->setTarget(Wt::TargetThisWindow);
	result_vbox->addWidget(title_anchor);

	Wt::WText* description_text = new Wt::WText(Wt::WString::fromUTF8(description));
	description_text->setStyleClass("result-description");
	result_vbox->addWidget(description_text);
	
	m_layout->addWidget(result_container);
}
