#include "global.h"

#include "application.h"
#include <Wt/WString>


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
    Wt::WString::setDefaultEncoding(Wt::UTF8);
    return Wt::WRun(argc, argv, &createApplication);
}
