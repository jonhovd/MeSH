#ifndef _HIERARCHY_H_
#define _HIERARCHY_H_

#include <Wt/WContainerWidget.h>
#include <Wt/WModelIndex.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WStandardItem.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WTreeView.h>

#include "elasticsearchutil.h"


class MeSHApplication;
class Hierarchy : public Wt::WContainerWidget
{
public:
	Hierarchy(MeSHApplication* mesh_application);

public:
	void PopulateHierarchy();
  void ClearMarkedItems();
  void Collapse();
	void ExpandToTreeNumber(const std::string& tree_number_string);

protected:
  void TreeItemExpanded(const Wt::WModelIndex& index);
	void TreeItemClicked(const Wt::WModelIndex& index, const Wt::WMouseEvent& mouse);
	void PopupMenuTriggered(Wt::WMenuItem* item);

private:
	void ExpandTreeNumberRecursive(const std::string& current_tree_number_string, Wt::WModelIndex& model_index);
	bool FindChildModelIndex(const std::string& tree_number_string, bool top_level, Wt::WModelIndex& index);
	bool AddChildPlaceholderIfNeeded(const Json::Object& source_object, const std::string& current_tree_number_string, std::unique_ptr<Wt::WStandardItem>& current_item);
public:
	static void GetParentTreeNumber(const std::string& child_tree_number, std::string& parent_tree_number);

private:
  MeSHApplication* m_mesh_application;

  Wt::WTreeView* m_hierarchy_tree_view;
  std::shared_ptr<Wt::WStandardItemModel> m_hierarchy_model;
  bool m_has_populated_hierarchy_model;
	std::vector< Wt::WStandardItem*> m_marked_hierarchy_items;

	std::unique_ptr<Wt::WPopupMenu> m_hierarchy_popup_menu;
	std::string m_popup_menu_id_string;
};

#endif // _HIERARCHY_H_
