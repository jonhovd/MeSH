#include "header.h"

#include <Wt/WHBoxLayout.h>
#include <Wt/WText.h>


Header::Header()
: Wt::WContainerWidget()
{
  setStyleClass("mesh-header");

  auto layout = Wt::cpp14::make_unique<Wt::WHBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);

  auto appname_text = Wt::cpp14::make_unique<Wt::WText>(Wt::WString::tr("AppName"));
  appname_text->setStyleClass("mesh-appname");
  layout->addWidget(std::move(appname_text), 0, Wt::AlignmentFlag::Center);

  setLayout(std::move(layout));
}
