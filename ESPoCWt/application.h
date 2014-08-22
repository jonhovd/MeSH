#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <Wt/WApplication>
#include <Wt/WJavaScript>
#include <Wt/WLineEdit>
#include <Wt/WStandardItem>
#include <Wt/WStandardItemModel>
#include <Wt/WSuggestionPopup>
#include <Wt/WTextArea>

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

private:
	long ESSearch(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result);

private:
    Wt::WSuggestionPopup* CreateSuggestionPopup(Wt::WContainerWidget* parent);

private:
	Wt::WLineEdit* m_search_edit;
	Wt::WSuggestionPopup* m_search_suggestion;
	Wt::WStandardItemModel* m_search_suggestion_model;
    Wt::JSignal<Wt::WString> m_search_signal;
  
	Wt::WTextArea* m_result_edit;
	ElasticSearch* m_es;
};

#endif // _APPLICATION_H_
