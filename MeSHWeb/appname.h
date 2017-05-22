#ifndef _APPNAME_H_
#define _APPNAME_H_

#include <Wt/WContainerWidget>
#include <Wt/WVBoxLayout>


class AppName : public Wt::WContainerWidget
{
public:
	AppName(Wt::WContainerWidget* parent = 0);
	~AppName();

private:
   Wt::WVBoxLayout* m_layout;
};

#endif // _APPNAME_H_
