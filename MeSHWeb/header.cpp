#include "header.h"

#include <Wt/WHBoxLayout.h>

#include "appname.h"


Header::Header()
: Wt::WContainerWidget()
{
	setStyleClass("mesh-header");

	auto layout = std::make_unique<Wt::WHBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(std::move(layout));

	auto app_name = std::make_unique<AppName>();
	layout->addWidget(std::move(app_name), 0, Wt::AlignmentFlag::Center);
}

Header::~Header()
{
}
