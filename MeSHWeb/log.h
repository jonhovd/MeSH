#ifndef _LOG_H_
#define _LOG_H_

#include <string>


class MeSHApplication;
class Log
{
public:
	Log(const MeSHApplication* mesh_application);

public:
	void LogSearch(const std::string& search_string);

private:
	const MeSHApplication* m_mesh_application;
};

#endif // _LOG_H_
