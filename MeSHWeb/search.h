#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <Wt/WContainerWidget>
#include <Wt/WLineEdit>
#include <Wt/WPanel>
#include <Wt/WStandardItem>
#include <Wt/WStandardItemModel>
#include <Wt/WSuggestionPopup>
#include <Wt/WText>
#include <Wt/WVBoxLayout>

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

#include "elasticsearchutil.h"
#include "links.h"


class MeSHApplication;
class Search : public Wt::WContainerWidget
{
public:
	Search(MeSHApplication* mesh_application, Wt::WContainerWidget* parent = 0);
	~Search();

public:
	void FocusSearchEdit();
	void OnSearch(const Wt::WString& mesh_id);
	void ClearLayout();
	
protected:
	void SearchEditFocussed();
	void SuggestionChanged(Wt::WStandardItem* item);
	void FilterSuggestion(const Wt::WString& filter);

private:
	Wt::WSuggestionPopup* CreateSuggestionPopup(Wt::WContainerWidget* parent);
	void CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str) const;
	void AddWildcard(const std::string filter_str, std::string& wildcard_filter_str) const;
	void FindIndirectHit(const std::string& haystack, const std::string& needles, double& best_hit_factor, std::string& indirect_hit_str);
	void FindIndirectHit(const Json::Object& source_object, const std::string& cleaned_filter_str, std::string& indirect_hit_str);

private:
	MeSHApplication* m_mesh_application;
	Wt::WVBoxLayout* m_layout;

	Wt::WLineEdit* m_search_edit;
	Wt::WSuggestionPopup* m_search_suggestion;
	Wt::WStandardItemModel* m_search_suggestion_model;

	Wt::WContainerWidget* m_result_container;

	Wt::WPanel* m_nor_term_panel;
	Wt::WLayout* m_nor_term_panel_layout;
	Wt::WText* m_nor_description_text;

	Wt::WPanel* m_eng_term_panel;
	Wt::WLayout* m_eng_term_panel_layout;
	Wt::WText* m_eng_description_text;

	Wt::WText* m_mesh_id_text;

	Links* m_links;
};

#endif // _SEARCH_H_
