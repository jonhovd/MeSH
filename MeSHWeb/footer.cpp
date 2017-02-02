#include "footer.h"

#include <Wt/WImage>


Footer::Footer(Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent)
{
	setStyleClass("mesh-footer");
	m_layout = new Wt::WHBoxLayout();
	m_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(m_layout);
	m_logo = new Logo();
	m_layout->addWidget(m_logo);
	m_layout->addStretch(1);
	m_info = new Info();
	m_layout->addWidget(m_info);
}

Footer::~Footer()
{
}
