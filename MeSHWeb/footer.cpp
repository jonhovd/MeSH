#include "footer.h"

#include <Wt/WHBoxLayout.h>
#include <Wt/WImage.h>

#include "info.h"


Footer::Footer()
: Wt::WContainerWidget()
{
	setStyleClass("mesh-footer");

	auto layout = Wt::cpp14::make_unique<Wt::WHBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(Wt::cpp14::make_unique<Info>(), 0, Wt::AlignmentFlag::Right);

    setLayout(std::move(layout));
}

Footer::~Footer()
{
}
