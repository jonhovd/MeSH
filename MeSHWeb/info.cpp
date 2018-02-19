#include "info.h"

#include <Wt/WAnchor.h>
#include <Wt/WVBoxLayout.h>


Info::Info()
: Wt::WContainerWidget()
{
	setStyleClass("info-box");
	auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
	layout->setContentsMargins(0, 0, 8, 0);
	setLayout(std::move(layout));

    Wt::WLink appabout_link(Wt::WString::tr("AppAboutUrl").toUTF8());
    appabout_link.setTarget(Wt::LinkTarget::NewWindow);
	auto appabout_anchor = Wt::cpp14::make_unique<Wt::WAnchor>(appabout_link, Wt::WString::tr("AppAbout"));
    layout->addWidget(std::move(appabout_anchor));

    Wt::WLink appquestion_link(Wt::WString::tr("AppSendQuestionUrl").toUTF8());
    appquestion_link.setTarget(Wt::LinkTarget::NewWindow);
    auto appquestion_anchor = Wt::cpp14::make_unique<Wt::WAnchor>(appquestion_link, Wt::WString::tr("AppSendQuestion"));
	layout->addWidget(std::move(appquestion_anchor));

    Wt::WLink appstatistics_link(Wt::LinkType::InternalPath, Wt::WString::tr("AppStatisticsInternalPath").toUTF8());
    appstatistics_link.setTarget(Wt::LinkTarget::NewWindow);
    auto appstatistics_anchor = Wt::cpp14::make_unique<Wt::WAnchor>(appstatistics_link, Wt::WString::tr("AppStatistics"));
	layout->addWidget(std::move(appstatistics_anchor));
}

Info::~Info()
{
}
