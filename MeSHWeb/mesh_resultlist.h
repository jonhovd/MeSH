#ifndef _MESH_RESULTLIST_H_
#define _MESH_RESULTLIST_H_

#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>


class MeSHApplication;
class MeshResultList : public Wt::WContainerWidget
{
public:
	MeshResultList(MeSHApplication* mesh_application);

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
