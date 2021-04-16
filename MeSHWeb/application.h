#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <memory>

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WMessageBox.h>
#include <Wt/WText.h>

#include "content.h"
#include "elasticsearchutil.h"

#define SUGGESTION_COUNT    (20)
#define RESULTLIST_COUNT    (10)

#define EQUAL                           (0)

#define SUGGESTIONLIST_ITEM_ID_ROLE     (Wt::ItemDataRole::User)
#define HIERARCHY_ITEM_TREE_NUMBER_ROLE (Wt::ItemDataRole::User+1)
#define HIERARCHY_ITEM_ID_ROLE          (Wt::ItemDataRole::User+2)


class MeSHApplication : public Wt::WApplication
{
public:
  MeSHApplication(const Wt::WEnvironment& environment);

protected: //From Wt::WApplication
	virtual void handleJavaScriptError(const std::string& errorText);
  void OnSearch(const Wt::WString& mesh_id) {GetContent()->GetSearch()->OnSearch(mesh_id);}

protected:
  void OnInternalPathChange(const std::string& url);
 
private:
  void ParseIdFromUrl(const std::string& url, std::string& id);
 
public:
  void ClearLayout();

public:
  std::shared_ptr<ElasticSearchUtil> GetElasticSearchUtil() const {return m_es_util;}
  Content* GetContent() const {return m_content;}

private:
  bool m_layout_is_cleared;

  Content* m_content;

  Wt::JSignal<Wt::WString> m_search_signal;

  std::shared_ptr<ElasticSearchUtil> m_es_util;
};

#endif // _APPLICATION_H_
