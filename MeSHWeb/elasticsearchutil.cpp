#include "elasticsearchutil.h"


ElasticSearchUtil::ElasticSearchUtil()
{
	m_es = new ElasticSearch("localhost:9200");
}

ElasticSearchUtil::~ElasticSearchUtil()
{
	delete m_es;
}

long ElasticSearchUtil::search(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result)
{
	try
	{
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
		return m_es->upsert(index, type, id, jData);
	}
    catch(...)
	{
		return false;
    }
}
