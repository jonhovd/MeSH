#include "appname.h"

#include <Wt/WText>


AppName::AppName(Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent)
{
	m_layout = new Wt::WVBoxLayout();
	m_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(m_layout);

	Wt::WText* appname_text = new Wt::WText(Wt::WString::tr("AppName"));
	appname_text->setStyleClass("mesh-appname");
	m_layout->addWidget(appname_text);
}

AppName::~AppName()
{
}
