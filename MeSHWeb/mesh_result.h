#ifndef _MESH_RESULT_H_
#define _MESH_RESULT_H_

#include <Wt/WContainerWidget>
#include <Wt/WPanel>
#include <Wt/WText>
#include <Wt/WVBoxLayout>

#include "links.h"


class MeSHApplication;
class MeshResult : public Wt::WContainerWidget
{
public:
	MeshResult(MeSHApplication* mesh_application, Wt::WContainerWidget* parent = 0);
	~MeshResult();

public:
	void ClearLayout();

	void OnSearch(const Wt::WString& mesh_id, const std::string& search_text);

private:
	MeSHApplication* m_mesh_application;
	Wt::WVBoxLayout* m_layout;

	Wt::WPanel* m_nor_term_panel;
	Wt::WVBoxLayout* m_nor_term_panel_layout;
	Wt::WText* m_nor_description_text;

	Wt::WPanel* m_eng_term_panel;
	Wt::WVBoxLayout* m_eng_term_panel_layout;
	Wt::WText* m_eng_description_text;

	Wt::WVBoxLayout* m_see_related_vbox;
	
	Links* m_links;
};

#endif // _MESH_RESULT_H_
