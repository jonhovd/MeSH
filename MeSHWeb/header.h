#ifndef _HEADER_H_
#define _HEADER_H_

#include <Wt/WContainerWidget>
#include <Wt/WHBoxLayout>

#include "appname.h"


class Header : public Wt::WContainerWidget
{
public:
	Header(Wt::WContainerWidget* parent = 0);
	~Header();

private:
	Wt::WHBoxLayout* m_layout;
	AppName* m_app_name;
};

#endif // _HEADER_H_
