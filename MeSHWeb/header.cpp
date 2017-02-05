#include "header.h"

#include <Wt/WImage>


Header::Header(Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent)
{
	setStyleClass("mesh-header");

	m_layout = new Wt::WHBoxLayout();
	m_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(m_layout);
	
	m_app_name = new AppName();
	m_layout->addWidget(m_app_name, 0, Wt::AlignCenter);
}

Header::~Header()
{
}
