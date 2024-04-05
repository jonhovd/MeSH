#include "about_tab.h"

#include "statistics.h"


AboutTab::AboutTab(const Wt::WString& text, MeSHApplication* mesh_application)
: Wt::WTemplate(text)
{
  bindWidget("statistics", std::make_unique<Statistics>(Wt::WString::tr("statisticsTemplate"), mesh_application));
}
