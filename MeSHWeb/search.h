#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <Wt/WContainerWidget>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WStandardItem>
#include <Wt/WStandardItemModel>
#include <Wt/WSuggestionPopup>
#include <Wt/WVBoxLayout>

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

#include "elasticsearchutil.h"
#include "mesh_result.h"
#include "mesh_resultlist.h"


class MeSHApplication;
class Search : public Wt::WContainerWidget
{
public:
	Search(MeSHApplication* mesh_application, Wt::WContainerWidget* parent = 0);
	~Search();

public:
	void ClearLayout();

	void FocusSearchEdit();
	void OnSearch(const Wt::WString& mesh_id);
	
protected:
	void SearchButtonClicked();

	void SearchEditFocussed();
	void SuggestionChanged(Wt::WStandardItem* item);
	void FilterSuggestion(const Wt::WString& filter);
	
private:
	Wt::WSuggestionPopup* CreateSuggestionPopup(Wt::WContainerWidget* parent);

public:
	static void CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str);
	static void FindIndirectHit(const Json::Object& source_object, const std::string& cleaned_filter_str, std::string& indirect_hit_str);
	static void FindIndirectHit(const std::string& haystack, const std::string& needles, double& best_hit_factor, std::string& indirect_hit_str);

public:
	static void AddWildcard(const std::string filter_str, std::string& wildcard_filter_str);
	static void MeSHToName(ElasticSearchUtil* es_util, const std::string& mesh_id, std::string& name);

private:
	MeSHApplication* m_mesh_application;
	Wt::WVBoxLayout* m_layout;

	Wt::WLineEdit* m_search_edit;
	Wt::WSuggestionPopup* m_search_suggestion;
	Wt::WStandardItemModel* m_search_suggestion_model;

	Wt::WPushButton* m_search_button;

	MeshResultList* m_mesh_resultlist;
	MeshResult* m_mesh_result;
};

#endif // _SEARCH_H_
