#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <memory>

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WTabWidget.h>

#include "hierarchy_tab.h"
#include "search_tab.h"
#include "elasticsearchutil.h"

#define SUGGESTION_COUNT    (20)
#define RESULTLIST_COUNT    (10)

#define EQUAL                           (0)

#define SUGGESTIONLIST_ITEM_ID_ROLE     (Wt::ItemDataRole::User)
#define HIERARCHY_ITEM_TREE_NUMBER_ROLE (Wt::ItemDataRole::User+1)
#define HIERARCHY_ITEM_ID_ROLE          (Wt::ItemDataRole::User+2)


class MeSHApplication : public Wt::WApplication
{
public:
enum TabId {
  TAB_INDEX_SEARCH=0,
  TAB_INDEX_HIERARCHY=1,
  TAB_INDEX_ABOUT=2
};
static const int TAB_PAGE_COUNT = 3;

public:
  MeSHApplication(const Wt::WEnvironment& environment);

protected: //From Wt::WApplication
	virtual void handleJavaScriptError(const std::string& errorText);

  void OnSearch(const Wt::WString& mesh_id) {GetSearch()->OnSearch(mesh_id);}

protected:
  void OnInternalPathChange(const std::string& url);
  void OnTabChanged(int index);
 
private:
  void ParseIdFromUrl(const std::string& url, std::string& id);
 
public:
  void ClearLayout();
  void SetActiveTab(const TabId& index);

public:
  std::shared_ptr<ElasticSearchUtil> GetElasticSearchUtil() const {return m_es_util;}
  HierarchyTab* GetHierarchy() const {return m_hierarchy_tab;}
  SearchTab* GetSearch() const {return m_search_tab;}

private:
  bool m_layout_is_cleared;

  Wt::WTabWidget* m_tab_widget;
  HierarchyTab* m_hierarchy_tab;
  SearchTab* m_search_tab;

  Wt::JSignal<Wt::WString> m_search_signal;

  std::shared_ptr<ElasticSearchUtil> m_es_util;
};

#endif // _APPLICATION_H_
