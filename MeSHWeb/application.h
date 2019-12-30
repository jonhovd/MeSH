#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <memory>

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WMessageBox.h>
#include <Wt/WStackedWidget>
#include <Wt/WText>

#include "elasticsearchutil.h"
#include "hierarchy.h"
#include "search.h"
#include "statistics.h"

#define SUGGESTION_COUNT    (20)
#define RESULTLIST_COUNT    (10)
#define LANGUAGE            "nor"

#define EQUAL                           (0)

#define SUGGESTIONLIST_ITEM_ID_ROLE     (Wt::ItemDataRole::User)
#define HIERARCHY_ITEM_TREE_NUMBER_ROLE (Wt::ItemDataRole::User+1)
#define HIERARCHY_ITEM_ID_ROLE          (Wt::ItemDataRole::User+2)


class MeSHApplication : public Wt::WApplication
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
  MeSHApplication(const Wt::WEnvironment& environment);
  ~MeSHApplication();

protected: //From Wt::WApplication
	virtual void handleJavaScriptError(const std::string& errorText);

protected:
  void OnInternalPathChange(const std::string& url);
  void OnPreviousButtonClicked();
  void OnNextButtonClicked();

private:
  void ParseIdFromUrl(const std::string& url, std::string& id);
  TabId GetPreviousStackedWidgetIndex() const;
  TabId GetNextStackedWidgetIndex() const;

public:
  void ClearLayout();
  void SetActiveStackedWidget(TabId index);
  void SearchMesh(const Wt::WString& mesh_id);

public:
  std::shared_ptr<ElasticSearchUtil> GetElasticSearchUtil() const {return m_es_util;}
  Hierarchy* GetHierarchy() const {return m_hierarchy;}

private:
  std::unique_ptr<Wt::WContainerWidget> CreateContentWidget();
  std::unique_ptr<Wt::WContainerWidget> CreateSearchWidget();

private:
  Wt::WStackedWidget* m_stacked_widget;

  bool m_layout_is_cleared;

  Wt::WString m_stacked_widget_titles[TAB_PAGE_COUNT];

  TabId m_visible_stacked_widget;
  Wt::WText* m_stacked_widget_title;
  bool m_statistics_page_is_hidden;
  Wt::WPushButton* m_previous_tab_button;
  Wt::WPushButton* m_next_tab_button;

	Statistics* m_statistics;

  Search* m_search;
  Wt::JSignal<Wt::WString> m_search_signal;

  Hierarchy* m_hierarchy;

  std::shared_ptr<ElasticSearchUtil> m_es_util;
};

#endif // _APPLICATION_H_
