#ifndef _ELASTICSEARCHUTIL_H_
#define _ELASTICSEARCHUTIL_H_

#include "elasticsearch/elasticsearch.h"

#include <memory>


class ElasticSearchUtil
{

public:
	ElasticSearchUtil();
	~ElasticSearchUtil();

public:
	long search(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result);

	bool getDocument(const char* index, const char* type, const char* id, Json::Object& msg);

	bool upsert(const std::string& index, const std::string& type, const std::string& id, const Json::Object& jData);

private:
	std::unique_ptr<ElasticSearch> m_es;
};

#endif // _ELASTICSEARCHUTIL_H_
