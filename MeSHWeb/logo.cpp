#include "logo.h"

#include <Wt/WImage>


Logo::Logo(Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent)
{
	m_layout = new Wt::WVBoxLayout();
	m_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(m_layout);

	Wt::WImage* logo = new Wt::WImage("images/logo.png");
	m_layout->addWidget(logo, 0, Wt::AlignLeft|Wt::AlignTop);
}

Logo::~Logo()
{
}
