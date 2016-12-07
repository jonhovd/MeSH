#ifndef _LINKS_H_
#define _LINKS_H_

#include <Wt/WContainerWidget>
#include <Wt/WVBoxLayout>
//#include <Wt/WWidget>
/*
#include <Wt/WApplication>
#include <Wt/WEnvironment>
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
*/

class Links : public Wt::WContainerWidget
{
public:
	Links(Wt::WContainerWidget* parent = 0);
	~Links();

public:
  virtual void clear();
  virtual void populate(const Wt::WString& mesh_id, const std::string& url_encoded_term, const std::string& url_encoded_filtertext);

private:
   Wt::WVBoxLayout* m_layout;
};

#endif // _LINKS_H_
