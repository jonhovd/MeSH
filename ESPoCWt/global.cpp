#include "global.h"

#include "application.h"
//#include <Wt/WServer>


Wt::WLogger g_logger;
Wt::WLogEntry Log(const std::string& type)
{
	return g_logger.entry(type);
}

Wt::WApplication* createApplication(const Wt::WEnvironment& env)
{
	return new ESPoCApplication(env);
}

int main(int argc, char** argv)
{
	g_logger.addField("message", true);
	return Wt::WRun(argc, argv, &createApplication);
}
