#include "appname.h"

#include <memory>

#include <Wt/WVBoxLayout.h>
#include <Wt/WText.h>


AppName::AppName()
: Wt::WContainerWidget()
{
	auto layout = std::make_unique<Wt::WVBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(std::move(layout));

	auto appname_text = std::make_unique<Wt::WText>(Wt::WString::tr("AppName"));
	appname_text->setStyleClass("mesh-appname");
	layout->addWidget(std::move(appname_text));
}

AppName::~AppName()
{
}
