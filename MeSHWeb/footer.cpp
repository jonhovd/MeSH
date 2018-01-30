#include "footer.h"

#include <Wt/WHBoxLayout.h>
#include <Wt/WImage.h>

#include "info.h"


Footer::Footer()
: Wt::WContainerWidget()
{
	setStyleClass("mesh-footer");

	auto layout = std::make_unique<Wt::WHBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(std::move(layout));

	layout->addWidget(std::make_unique<Info>(), 0, Wt::AlignmentFlag::Right);
}

Footer::~Footer()
{
}
