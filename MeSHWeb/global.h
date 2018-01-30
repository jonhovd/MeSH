#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <Wt/WApplication.h>

//#define LOG_SUPPORT

#define UNUSED(x) /*make compiler happy*/

extern Wt::WLogger g_logger;
void Log(const std::string& type, const std::string& msg);

#endif // _GLOBAL_H_
