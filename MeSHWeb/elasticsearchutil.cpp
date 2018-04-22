#include "elasticsearchutil.h"


ElasticSearchUtil::ElasticSearchUtil()
{
	m_es = std::make_unique<ElasticSearch>("localhost:9200");
}

ElasticSearchUtil::~ElasticSearchUtil()
{
}

long ElasticSearchUtil::search(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result)
{
	try
	{
        errno = 0;
		return m_es->search(index, type, query, search_result);
	}
    catch(...)
	{
		return 0L;
    }
}

bool ElasticSearchUtil::getDocument(const char* index, const char* type, const char* id, Json::Object& msg)
{
	try
	{
        errno = 0;
		return m_es->getDocument(index, type, id, msg);
	}
    catch(...)
	{
		return false;
    }
}

bool ElasticSearchUtil::upsert(const std::string& index, const std::string& type, const std::string& id, const Json::Object& jData)
{
	try
	{
        errno = 0;
		return m_es->upsert(index, type, id, jData);
	}
    catch(...)
	{
		return false;
    }
}
