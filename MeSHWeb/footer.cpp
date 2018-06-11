#include "footer.h"

#include <Wt/WHBoxLayout.h>
#include <Wt/WImage.h>

#include "info.h"


Footer::Footer()
: Wt::WContainerWidget()
{
	auto layout = Wt::cpp14::make_unique<Wt::WHBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);

	auto logo = Wt::cpp14::make_unique<Wt::WImage>("images/logo.png");
    logo->setStyleClass("mesh-footer-logo");
	layout->addWidget(std::move(logo), 1, Wt::AlignmentFlag::Left);
    
    layout->addWidget(Wt::cpp14::make_unique<Info>(), 1, Wt::AlignmentFlag::Right);

    setLayout(std::move(layout));
}

Footer::~Footer()
{
}
