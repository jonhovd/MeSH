#include "info.h"

#include <Wt/WAnchor.h>
#include <Wt/WVBoxLayout.h>


Info::Info()
: Wt::WContainerWidget()
{
	setStyleClass("info-box");
	auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
	layout->setContentsMargins(0, 0, 8, 0);

    Wt::WLink appabout_link(Wt::WString::tr("AppAboutUrl").toUTF8());
    appabout_link.setTarget(Wt::LinkTarget::NewWindow);
    layout->addWidget(Wt::cpp14::make_unique<Wt::WAnchor>(appabout_link, Wt::WString::tr("AppAbout")));

    Wt::WLink appquestion_link(Wt::WString::tr("AppSendQuestionUrl").toUTF8());
    appquestion_link.setTarget(Wt::LinkTarget::NewWindow);
	layout->addWidget(Wt::cpp14::make_unique<Wt::WAnchor>(appquestion_link, Wt::WString::tr("AppSendQuestion")));

    Wt::WLink appstatistics_link(Wt::LinkType::InternalPath, Wt::WString::tr("AppStatisticsInternalPath").toUTF8());
    appstatistics_link.setTarget(Wt::LinkTarget::NewWindow);
    layout->addWidget(Wt::cpp14::make_unique<Wt::WAnchor>(appstatistics_link, Wt::WString::tr("AppStatistics")));

    setLayout(std::move(layout));
}

Info::~Info()
{
}
