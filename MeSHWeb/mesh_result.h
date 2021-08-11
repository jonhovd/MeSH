#ifndef _MESH_RESULT_H_
#define _MESH_RESULT_H_

#include <Wt/WContainerWidget.h>
#include <Wt/WPanel.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WTemplate.h>
#include <Wt/WText.h>
#include <Wt/WTreeView.h>
#include <Wt/WVBoxLayout.h>

#include "elasticsearchutil.h"

#include "links.h"


class MeSHApplication;
class MeshResult : public Wt::WTemplate
{
public:
	MeshResult(const Wt::WString& text, MeSHApplication* mesh_application);

public:
	void ClearLayout();

	void OnSearch(const Wt::WString& mesh_id, const std::string& search_text);

protected:
  void TreeItemClicked(const Wt::WModelIndex& index, const Wt::WMouseEvent& mouse);
  void PopupMenuTriggered(Wt::WMenuItem* item);

private:
  void SetAndActivateDescription(Wt::WText* text_ctrl, const std::string& text);
  void SetOtherTermTexts(Wt::WLayout* term_layout, const Json::Array& terms_array);
	void RecursiveAddHierarchyItem(std::shared_ptr<ElasticSearchUtil> es_util, int& row, std::map<std::string,Wt::WStandardItem*>& node_map, const std::string& tree_number, bool mark_item);
	void PopulateHierarchy(std::shared_ptr<ElasticSearchUtil> es_util, const Json::Object& source_object);

private:
	MeSHApplication* m_mesh_application;

	Wt::WPanel* m_nor_term_panel;
	Wt::WContainerWidget* m_nor_term_container;
	Wt::WVBoxLayout* m_nor_term_panel_layout;
	Wt::WText* m_nor_description_text;

	Wt::WPanel* m_eng_term_panel;
	Wt::WContainerWidget* m_eng_term_container;
	Wt::WVBoxLayout* m_eng_term_panel_layout;
	Wt::WText* m_eng_description_text;

  Wt::WAnchor* m_external_link;

	Wt::WContainerWidget* m_see_related_container;
	Links* m_links;

  Wt::WTreeView* m_hierarchy_tree_view;
	std::shared_ptr<Wt::WStandardItemModel> m_hierarchy_model;

  std::unique_ptr<Wt::WPopupMenu> m_hierarchy_popup_menu;
  std::string m_popup_menu_id_string;
};

#endif // _MESH_RESULT_H_
