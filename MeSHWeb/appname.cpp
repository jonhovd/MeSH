#include "appname.h"

#include <memory>

#include <Wt/WVBoxLayout.h>
#include <Wt/WText.h>


AppName::AppName()
: Wt::WContainerWidget()
{
	auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);

	auto appname_text = Wt::cpp14::make_unique<Wt::WText>(Wt::WString::tr("AppName"));
	appname_text->setStyleClass("mesh-appname");
	layout->addWidget(std::move(appname_text));

    setLayout(std::move(layout));
}

AppName::~AppName()
{
}
