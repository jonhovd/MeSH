#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <Wt/WApplication>
//#include <Wt/WEnvironment>
//#include <Wt/WLogger>

#define UNUSED(x) /*make compiler happy*/

extern Wt::WLogger g_logger;
Wt::WLogEntry Log(const std::string& type);

Wt::WApplication* createApplication(const Wt::WEnvironment& env);

int main(int argc, char** argv);

#endif // _GLOBAL_H_
