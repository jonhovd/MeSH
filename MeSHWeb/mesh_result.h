#ifndef _MESH_RESULT_H_
#define _MESH_RESULT_H_

#include <Wt/WContainerWidget.h>
#include <Wt/WPanel.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WText.h>
#include <Wt/WTreeView.h>
#include <Wt/WVBoxLayout.h>

#include "elasticsearchutil.h"

#include "links.h"


class MeSHApplication;
class MeshResult : public Wt::WContainerWidget
{
public:
	MeshResult(MeSHApplication* mesh_application);
	~MeshResult();

public:
	void ClearLayout();

	void OnSearch(const Wt::WString& mesh_id, const std::string& search_text);

private:
	void RecursiveAddHierarchyItem(std::shared_ptr<ElasticSearchUtil> es_util, int& row, std::map<std::string,Wt::WStandardItem*>& node_map, const std::string& tree_number, bool mark_item);
	void PopulateHierarchy(std::shared_ptr<ElasticSearchUtil> es_util, const Json::Object& source_object);

private:
	MeSHApplication* m_mesh_application;

	Wt::WPanel* m_nor_term_panel;
	Wt::WContainerWidget* m_nor_term_container;
	Wt::WVBoxLayout* m_nor_term_panel_layout;
	Wt::WText* m_nor_description_label;
	Wt::WText* m_nor_description_text;

	Wt::WPanel* m_eng_term_panel;
	Wt::WContainerWidget* m_eng_term_container;
	Wt::WVBoxLayout* m_eng_term_panel_layout;
	Wt::WText* m_eng_description_label;
	Wt::WText* m_eng_description_text;

	Wt::WTreeView* m_hierarchy_tree_view;
	std::shared_ptr<Wt::WStandardItemModel> m_hierarchy_model;

	Wt::WText* m_see_related_text;
	Wt::WContainerWidget* m_see_related_container;
	Links* m_links;
};

#endif // _MESH_RESULT_H_
