#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/WMessageBox>
#include <Wt/WTabWidget>

#include "elasticsearchutil.h"
#include "hierarchy.h"
#include "search.h"
#include "statistics.h"
#include "footer.h"

#define SUGGESTION_COUNT    (20)
#define RESULTLIST_COUNT    (10)
#define LANGUAGE            "nor"

#define EQUAL                           (0)

#define SUGGESTIONLIST_ITEM_ID_ROLE     (Wt::UserRole)
#define HIERARCHY_ITEM_TREE_NUMBER_ROLE (Wt::UserRole+1)
#define HIERARCHY_ITEM_ID_ROLE          (Wt::UserRole+2)


class MeSHApplication : public Wt::WApplication
{
public:
enum TabId {
	TAB_INDEX_SEARCH,
	TAB_INDEX_HIERARCHY,
	TAB_INDEX_STATISTICS
};

public:
	MeSHApplication(const Wt::WEnvironment& environment);
	~MeSHApplication();

protected: //From Wt::WApplication
	virtual void handleJavaScriptError(const std::string& errorText);

protected:
	void onTabChanged(int active_tab_index);
	void onInternalPathChange(const std::string& url);

private:
	void parseIdFromUrl(const std::string& url, std::string& id);

public:
	void ClearLayout();
	void SetActiveTab(int tab_index);
	void SearchMesh(const Wt::WString& mesh_id);

public:
	ElasticSearchUtil* GetElasticSearchUtil() const {return m_es_util;}
	Hierarchy* GetHierarchy() const {return m_hierarchy;}

private:
	Wt::JSignal<Wt::WString> m_search_signal;

	bool m_layout_is_cleared;

	Wt::WTabWidget* m_tab_widget;
	Statistics* m_statistics;

	Search* m_search;
	Hierarchy* m_hierarchy;
	Footer* m_footer;

	Wt::WGridLayout* m_statistics_layout;

	ElasticSearchUtil* m_es_util;
};

#endif // _APPLICATION_H_
