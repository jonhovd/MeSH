#include "global.h"

#include "application.h"
#include <boost/locale.hpp>
#include <Wt/WString.h>


Wt::WLogger g_logger;
#ifdef LOG_SUPPORT
void Log(const std::string& type, const std::string& msg)
{
    g_logger.entry(type) << msg;
}
#else
void Log(const std::string& UNUSED(type), const std::string& UNUSED(msg))
{
}
#endif

std::unique_ptr<Wt::WApplication> createApplication(const Wt::WEnvironment& env)
{
    return std::make_unique<MeSHApplication>(env);
}

int main(int argc, char** argv)
{
    g_logger.addField("message", true);
    Wt::WString::setDefaultEncoding(Wt::CharEncoding::UTF8);
    
    // Create system default locale
    boost::locale::generator gen;
    std::locale loc = gen(""); 
    std::locale::global(loc);
    
    return Wt::WRun(argc, argv, &createApplication);
}
