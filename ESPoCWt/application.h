#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include <Wt/WEnvironment>
#include <Wt/WJavaScript>
#include <Wt/WLayout>
#include <Wt/WLineEdit>
#include <Wt/WPanel>
#include <Wt/WStandardItem>
#include <Wt/WStandardItemModel>
#include <Wt/WSuggestionPopup>
#include <Wt/WText>
#include <Wt/WTextArea>
#include <Wt/WTreeView>

#include "elasticsearch/elasticsearch.h"


class ESPoCApplication : public Wt::WApplication
{
public:
    ESPoCApplication(const Wt::WEnvironment& environment);
    ~ESPoCApplication();

protected:
	void FilterSuggestion(const Wt::WString& filter);
	void SuggestionChanged(Wt::WStandardItem* item);
    void Search(const Wt::WString& mesh_id);
    void TabChanged(int active_tab_index);

private:
	long ESSearch(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result);

private:
    void PopulateHierarchy();
    Wt::WSuggestionPopup* CreateSuggestionPopup(Wt::WContainerWidget* parent);
    void ClearLayout();

private:
    bool m_layout_is_cleared;
    
	Wt::WLineEdit* m_search_edit;
	Wt::WSuggestionPopup* m_search_suggestion;
	Wt::WStandardItemModel* m_search_suggestion_model;
    Wt::JSignal<Wt::WString> m_search_signal;
  
    Wt::WPanel* m_nor_term_panel;
    Wt::WLayout* m_nor_term_panel_layout;
    Wt::WPanel* m_eng_term_panel;
    Wt::WLayout* m_eng_term_panel_layout;

    Wt::WText* m_description_text;
    
    Wt::WText* m_mesh_id_text;

    Wt::WLayout* m_links_layout;
    
    Wt::WTreeView* m_hierarchy_tree_view;
    Wt::WStandardItemModel* m_hierarchy_model;
    
    ElasticSearch* m_es;
};

#endif // _APPLICATION_H_
