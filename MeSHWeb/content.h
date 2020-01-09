#ifndef _CONTENT_H_
#define _CONTENT_H_


#include <Wt/WContainerWidget.h>
#include <Wt/WPushButton.h>
#include <Wt/WStackedWidget>

#include "hierarchy.h"
#include "search.h"
#include "statistics.h"


class MeSHApplication;

class Content : public Wt::WContainerWidget
{
public:
enum TabId {
  TAB_INDEX_CONTACTINFO=0,
  TAB_INDEX_SEARCH=1,
  TAB_INDEX_HIERARCHY=2,
  TAB_INDEX_STATISTICS=3
};
static const int TAB_PAGE_COUNT = 4;

public:
	Content(MeSHApplication* mesh_application);
	~Content();

protected:
  void OnPreviousButtonClicked();
  void OnNextButtonClicked();

private:
  TabId GetPreviousStackedWidgetIndex() const;
  TabId GetNextStackedWidgetIndex() const;

public:
  void ClearLayout();
  void SetActiveStackedWidget(TabId index);
  void SetStatisticsPageIasHidden(bool hidden);
  void SearchMesh(const Wt::WString& mesh_id);

public:
  Hierarchy* GetHierarchy() const {return m_hierarchy;}

private:
  std::unique_ptr<Wt::WContainerWidget> CreateSearchWidget(MeSHApplication* mesh_application);

private:
  Wt::WStackedWidget* m_stacked_widget;

  Wt::WString m_stacked_widget_titles[TAB_PAGE_COUNT];
  TabId m_visible_stacked_widget;
  Wt::WText* m_stacked_widget_title;
  bool m_statistics_page_is_hidden;
  Wt::WPushButton* m_previous_tab_button;
  Wt::WPushButton* m_next_tab_button;

  Search* m_search;
  Wt::JSignal<Wt::WString> m_search_signal;

  Hierarchy* m_hierarchy;
	Statistics* m_statistics;
};

#endif // _CONTENT_H_
