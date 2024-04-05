#ifndef _ABOUT_TAB_H_
#define _ABOUT_TAB_H_

#include <Wt/WTemplate.h>


class MeSHApplication;
class AboutTab : public Wt::WTemplate
{
public:
  AboutTab(const Wt::WString& text, MeSHApplication* mesh_application);
};

#endif // _ABOUT_TAB_H_
