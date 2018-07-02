#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <memory>

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WMessageBox.h>
#include <Wt/WTabWidget.h>

#include "elasticsearchutil.h"
#include "header.h"
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
	std::shared_ptr<ElasticSearchUtil> GetElasticSearchUtil() const {return m_es_util;}
	Hierarchy* GetHierarchy() const {return m_hierarchy;}

private:
	std::unique_ptr<Wt::WContainerWidget> InitializeContentWidget();

private:
	Wt::JSignal<Wt::WString> m_search_signal;

	bool m_layout_is_cleared;

	Wt::WTabWidget* m_tab_widget;
	Statistics* m_statistics;

	Search* m_search;
	Hierarchy* m_hierarchy;

	std::shared_ptr<ElasticSearchUtil> m_es_util;
};

#endif // _APPLICATION_H_
