#include "info.h"

#include <Wt/WAnchor>


Info::Info(Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
  m_layout(nullptr)
{
	setStyleClass("info-box");
	m_layout = new Wt::WVBoxLayout();
	m_layout->setContentsMargins(0, 0, 8, 0);
	setLayout(m_layout);

	Wt::WAnchor* appabout_anchor = new Wt::WAnchor(Wt::WLink(Wt::WString::tr("AppAboutUrl").toUTF8()), Wt::WString::tr("AppAbout"));
	appabout_anchor->setTarget(Wt::TargetNewWindow);
	m_layout->addWidget(appabout_anchor);
	Wt::WAnchor* appquestion_anchor = new Wt::WAnchor(Wt::WLink(Wt::WString::tr("AppSendQuestionUrl").toUTF8()), Wt::WString::tr("AppSendQuestion"));
	m_layout->addWidget(appquestion_anchor);
	Wt::WAnchor* appstatistics_anchor = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, Wt::WString::tr("AppStatisticsInternalPath").toUTF8()), Wt::WString::tr("AppStatistics"));
	m_layout->addWidget(appstatistics_anchor);
}

Info::~Info()
{
}
