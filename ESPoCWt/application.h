#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include <Wt/WEnvironment>
#include <Wt/WGridLayout>
#include <Wt/WJavaScript>
#include <Wt/WLayout>
#include <Wt/WLineEdit>
#include <Wt/WMessageBox>
#include <Wt/WPanel>
#include <Wt/WPopupMenu>
#include <Wt/WStandardItem>
#include <Wt/WStandardItemModel>
#include <Wt/WSuggestionPopup>
#include <Wt/WTabWidget>
#include <Wt/WText>
#include <Wt/WTextArea>
#include <Wt/WTreeView>

#include "elasticsearch/elasticsearch.h"


class ESPoCApplication : public Wt::WApplication
{
enum TabId {
    TAB_INDEX_SEARCH,
    TAB_INDEX_HIERARCHY,
    TAB_INDEX_STATISTICS
};

public:
    ESPoCApplication(const Wt::WEnvironment& environment);
    ~ESPoCApplication();

private:
    Wt::WContainerWidget* CreateSearchTab();
    Wt::WContainerWidget* CreateStatisticsTab();
    
private:
    void FindIndirectHit(const std::string& haystack, const std::string& needles, double& best_hit_factor, std::string& indirect_hit_str);
protected:
    void FindIndirectHit(const Json::Object& source_object, const std::string& cleaned_filter_str, std::string& indirect_hit_str);
    void FilterSuggestion(const Wt::WString& filter);
    void SuggestionChanged(Wt::WStandardItem* item);
    void Search(const Wt::WString& mesh_id);
    void CollapseHierarchy();
    void ExpandToTreeNumber(const std::string& tree_number_string);
    void ExpandTreeNumberRecursive(const std::string& current_tree_number_string, Wt::WModelIndex& model_index);
    bool FindChildModelIndex(const std::string& tree_number_string, bool top_level, Wt::WModelIndex& index);

    void SearchEditFocussed();
    void TabChanged(int active_tab_index);
    void TreeItemExpanded(const Wt::WModelIndex& index);
    void TreeItemClicked(const Wt::WModelIndex& index, const Wt::WMouseEvent& mouse);
    void PopupMenuTriggered(Wt::WMenuItem* item);
    void InfoboxButtonClicked();
    void onInternalPathChange(const std::string& url);

private:
	long ESSearch(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result);
    void LogSearch(const std::string& search_string);

private:
    void PopulateHierarchy();
    void PopulateStatistics();
    void PopulateDayStatistics();
    void PopulateTextStatistics();
    Wt::WSuggestionPopup* CreateSuggestionPopup(Wt::WContainerWidget* parent);
    void ClearLayout();

    void CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str) const;
    void AddWildcard(const std::string filter_str, std::string& wildcard_filter_str) const;
    void GetParentTreeNumber(const std::string& child_tree_number, std::string& parent_tree_number);
    bool AddChildPlaceholderIfNeeded(const Json::Object& source_object, const std::string& current_tree_number_string, Wt::WStandardItem* current_item);

    void ShowOrHideInfobox();

    void MeSHToName(const std::string& mesh_id, std::string& name);

private:
    bool m_layout_is_cleared;
    
    Wt::WMessageBox* m_infobox;
    bool m_infobox_visible;
    
    Wt::WTabWidget* m_tab_widget;
    Wt::WContainerWidget* m_statistics_tab;

    Wt::WLineEdit* m_search_edit;
	Wt::WSuggestionPopup* m_search_suggestion;
	Wt::WStandardItemModel* m_search_suggestion_model;
    Wt::JSignal<Wt::WString> m_search_signal;
  
    Wt::WContainerWidget* m_result_container;

    Wt::WPanel* m_nor_term_panel;
    Wt::WLayout* m_nor_term_panel_layout;
    Wt::WText* m_nor_description_text;

    Wt::WPanel* m_eng_term_panel;
    Wt::WLayout* m_eng_term_panel_layout;
    Wt::WText* m_eng_description_text;
    
    Wt::WText* m_mesh_id_text;

    Wt::WGridLayout* m_links_layout;
    
    Wt::WTreeView* m_hierarchy_tree_view;
    Wt::WStandardItemModel* m_hierarchy_model;
    bool m_has_populated_hierarchy_model;
    Wt::WPopupMenu* m_hierarchy_popup_menu;
    std::string m_popup_menu_id_string;

    Wt::WGridLayout* m_statistics_layout;

    ElasticSearch* m_es;
};

#endif // _APPLICATION_H_
