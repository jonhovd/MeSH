#ifndef _MESH_RESULTLIST_H_
#define _MESH_RESULTLIST_H_

#include <Wt/WContainerWidget>
#include <Wt/WVBoxLayout>
/*
#include <Wt/WLineEdit>
#include <Wt/WPanel>
#include <Wt/WPushButton>
#include <Wt/WStandardItem>
#include <Wt/WStandardItemModel>
#include <Wt/WSuggestionPopup>
#include <Wt/WText>

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

#include "elasticsearchutil.h"
#include "links.h"

*/
class MeSHApplication;
class MeshResultList : public Wt::WContainerWidget
{
public:
	MeshResultList(MeSHApplication* mesh_application, Wt::WContainerWidget* parent = 0);
	~MeshResultList();

public:
	void ClearLayout();

	void OnSearch(const Wt::WString& filter);
	
private:
	void AppendHit(const std::string& mesh_id, const std::string& title, const std::string description);

private:
	MeSHApplication* m_mesh_application;
	Wt::WVBoxLayout* m_layout;
};

#endif // _MESH_RESULTLIST_H_
