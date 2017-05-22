#ifndef _FOOTER_H_
#define _FOOTER_H_

#include <Wt/WContainerWidget>
#include <Wt/WHBoxLayout>

#include "info.h"
#include "logo.h"


class Footer : public Wt::WContainerWidget
{
public:
	Footer(Wt::WContainerWidget* parent = 0);
	~Footer();

private:
	Wt::WHBoxLayout* m_layout;
	Logo*  m_logo;
	Info*  m_info;
};

#endif // _FOOTER_H_
