#ifndef _STATISTICS_TAB_H_
#define _STATISTICS_TAB_H_

#include <Wt/WContainerWidget.h>
#include <Wt/WGridLayout.h>
#include <Wt/WTemplate.h>


class MeSHApplication;
class Statistics : public Wt::WTemplate
{
public:
  Statistics(const Wt::WString& text, MeSHApplication* mesh_application);

protected:
  void Populate();

private:
  void PopulateDayStatistics(std::unique_ptr<Wt::WGridLayout>& layout, int& row);
  void PopulateTextStatistics(std::unique_ptr<Wt::WGridLayout>& layout, int& row);

private:
  const MeSHApplication* m_mesh_application;

  Wt::WContainerWidget* m_content;
  bool m_content_is_populated;
};

#endif // _STATISTICS_TAB_H_
