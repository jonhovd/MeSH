#include "header.h"

#include <Wt/WHBoxLayout.h>

#include "appname.h"


Header::Header()
: Wt::WContainerWidget()
{
  setStyleClass("mesh-header");

  auto layout = Wt::cpp14::make_unique<Wt::WHBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(Wt::cpp14::make_unique<AppName>(), 0, Wt::AlignmentFlag::Center);

  setLayout(std::move(layout));
}

Header::~Header()
{
}
