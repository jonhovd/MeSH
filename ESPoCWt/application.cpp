#include "application.h"

#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WPushButton>
#include <Wt/WStandardItem>
#include <Wt/WTabWidget>
#include <Wt/WText>

#define SUGGESTION_COUNT	(20)


ESPoCApplication::ESPoCApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_edit(NULL),
  m_search_suggestion(NULL),
  m_search_suggestion_model(NULL),
  m_result_edit(NULL),
  m_es(NULL)
{
	m_es = new ElasticSearch("localhost:9200");

	setTitle("ElasticSearch Proof-of-Concept Wt application");

	Wt::WTabWidget* tab_widget = new Wt::WTabWidget(root());

	Wt::WContainerWidget* search_tab = new Wt::WContainerWidget();
	Wt::WContainerWidget* hierarchy_tab = new Wt::WContainerWidget();

	m_search_edit = new Wt::WLineEdit(search_tab);

	Wt::WSuggestionPopup::Options suggestion_options;
	suggestion_options.highlightBeginTag = "<b>";
	suggestion_options.highlightEndTag = "</b>";
	suggestion_options.listSeparator = 0;
	suggestion_options.whitespace = " \\n";
#if (WT_VERSION >= 0x03030000)
	suggestion_options.wordStartRegexp = ".*";
#endif
	m_search_suggestion = new Wt::WSuggestionPopup(suggestion_options, search_tab);
	m_search_suggestion->forEdit(m_search_edit);
	m_search_suggestion_model = new Wt::WStandardItemModel(search_tab);
	m_search_suggestion->setModel(m_search_suggestion_model);
	m_search_suggestion->setFilterLength(2);
	m_search_suggestion->filterModel().connect(this, &ESPoCApplication::FilterSuggestion);
	m_search_suggestion_model->itemChanged().connect(this, &ESPoCApplication::SuggestionChanged);
	
	m_search_edit->setFocus();
	m_search_edit->enterPressed().connect(this, &ESPoCApplication::Search);

	Wt::WPushButton* search_button = new Wt::WPushButton(Wt::WText::tr("Search"), search_tab);
	search_button->clicked().connect(this, &ESPoCApplication::Search);

	search_tab->addWidget(new Wt::WBreak());

	m_result_edit = new Wt::WTextArea(search_tab);
	m_result_edit->setReadOnly(true);

	tab_widget->addTab(search_tab, Wt::WString::tr("Search"));
	tab_widget->addTab(hierarchy_tab, Wt::WString::tr("Hierarchy"));
}

ESPoCApplication::~ESPoCApplication()
{
	delete m_es;
}

void ESPoCApplication::FilterSuggestion(const Wt::WString& filter)
{
	m_search_suggestion_model->clear();

	Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(SUGGESTION_COUNT).arg(filter);

	Json::Object search_result;
	long result_size = ESSearch("mesh", "descriptor", query.toUTF8(), search_result);

	const Json::Value value = search_result.getValue("hits");
	const Json::Object value_object = value.getObject();
	const Json::Value hits_value = value_object.getValue("hits");
	const Json::Array hits_array = hits_value.getArray();
	
	int row = 0;
	if (0 == result_size)
	{
//		m_search_suggestion_model->addString("<no matches>");
//		row = 1;
	}
	else
	{
		Json::Array::const_iterator iterator = hits_array.begin();
		for (row=0; row<SUGGESTION_COUNT && iterator!=hits_array.end(); row++)
		{
			const Json::Value value = *iterator;
			const Json::Object value_object = value.getObject();
			const Json::Value source_value = value_object.getValue("_source");
			const Json::Object source_object = source_value.getObject();
			
			const Json::Value id_value = source_object.getValue("id");
			const Json::Value name_value = source_object.getValue("name");
			
			Wt::WStandardItem* item = new Wt::WStandardItem(name_value.getString());
			item->setData(boost::any(id_value.getString()), Wt::UserRole);
			m_search_suggestion_model->setItem(row, 0, item);

			++iterator;
		}
	}

	m_search_suggestion_model->setData(--row, 0, std::string("Wt-more-data"), Wt::StyleClassRole);
}

void ESPoCApplication::SuggestionChanged(Wt::WStandardItem* item)
{
	if (item)
	{
		item = NULL;
	}
}

void ESPoCApplication::Search()
{
	m_search_edit->text().toUTF8();

	Json::Object search_result;
	int64_t result_size = ESSearch("mesh", "descriptor", "{\"query\":{\"match_all\":{}}}", search_result);

	m_result_edit->setText(Wt::WString("We found {1} result(s):\n{2}").arg(result_size).arg(search_result.str()));
}

long ESPoCApplication::ESSearch(const std::string& index, const std::string& type, const std::string& query, Json::Object& search_result)
{
	try
	{
		return m_es->search(index, type, query, search_result);
	}
    catch(...)
	{
		return 0L;
    }
}
