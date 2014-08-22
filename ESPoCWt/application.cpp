#include "application.h"

#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WStandardItem>
#include <Wt/WTabWidget>
#include <Wt/WText>

#define SUGGESTION_COUNT	(20)

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__


ESPoCApplication::ESPoCApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_edit(NULL),
  m_search_suggestion(NULL),
  m_search_suggestion_model(NULL),
  m_search_signal(this, "search"),
  m_result_edit(NULL),
  m_es(NULL)
{
	m_es = new ElasticSearch("localhost:9200");

	setTitle("ElasticSearch Proof-of-Concept Wt application");

	Wt::WTabWidget* tab_widget = new Wt::WTabWidget(root());

	Wt::WContainerWidget* search_tab = new Wt::WContainerWidget();
	Wt::WContainerWidget* hierarchy_tab = new Wt::WContainerWidget();

	m_search_edit = new Wt::WLineEdit(search_tab);
	m_search_suggestion = CreateSuggestionPopup(search_tab);// new Wt::WSuggestionPopup(suggestion_options, search_tab);
	m_search_suggestion->forEdit(m_search_edit);
	m_search_suggestion_model = new Wt::WStandardItemModel(search_tab);
	m_search_suggestion->setModel(m_search_suggestion_model);
	m_search_suggestion->setFilterLength(2);
    m_search_suggestion->filterModel().connect(this, &ESPoCApplication::FilterSuggestion);
	m_search_suggestion_model->itemChanged().connect(this, &ESPoCApplication::SuggestionChanged);
    m_search_signal.connect(this, &ESPoCApplication::Search);
	
	m_search_edit->setFocus();

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

	Wt::WString query = Wt::WString::tr("SuggestionFilterQuery").arg(0).arg(SUGGESTION_COUNT+1 /* +1 to see if we got more than SUGGESTION_COUNT hits */).arg(filter);

	Json::Object search_result;
	long result_size = ESSearch("mesh", "nor", query.toUTF8(), search_result);

	const Json::Value value = search_result.getValue("hits");
	const Json::Object value_object = value.getObject();
	const Json::Value hits_value = value_object.getValue("hits");
	const Json::Array hits_array = hits_value.getArray();
	
	int row = 0;
	if (0 == result_size)
	{
            Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::tr("NoHits"));
            item->setData(boost::any(""), Wt::UserRole);
            m_search_suggestion_model->setItem(row++, 0, item);
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
            Json::Value english_name_value;
            if (source_object.member("english_name")) {
                english_name_value = source_object.getValue("english_name");
            }

			Wt::WString suggestion_text;
            if (english_name_value.empty() || 0==name_value.getString().compare(english_name_value.getString()))
            {
                suggestion_text = Wt::WString::fromUTF8(name_value.getString());
            }
            else
            {
                suggestion_text = Wt::WString::tr("SuggestionFormat").arg(Wt::WString::fromUTF8(name_value.getString())).arg(Wt::WString::fromUTF8(english_name_value.getString()));
            }
               
			Wt::WStandardItem* item = new Wt::WStandardItem(suggestion_text);
			item->setData(boost::any(id_value.getString()), Wt::UserRole);
			m_search_suggestion_model->setItem(row, 0, item);

			++iterator;
		}

        m_search_suggestion_model->sort(0);

        if (hits_array.size() > SUGGESTION_COUNT)
        {
            Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::tr("MoreHits"));
            item->setData(boost::any(""), Wt::UserRole);
            m_search_suggestion_model->setItem(row++, 0, item);
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

void ESPoCApplication::Search(const Wt::WString& mesh_id)
{
	Json::Object search_result;
	int64_t result_size = ESSearch("mesh", "nor", "{\"query\":{\"match_all\":{}}}", search_result);

	m_result_edit->setText(Wt::WString("We found {1} result(s):\n{2}\n{3}").arg(result_size).arg(search_result.str()).arg(mesh_id));
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

Wt::WSuggestionPopup* ESPoCApplication::CreateSuggestionPopup(Wt::WContainerWidget* parent)
{
    std::string matcherJS = INLINE_JAVASCRIPT(
        function (edit) {
            var value = edit.value;
            return function(suggestion) {
                if (!suggestion)
                    return value;

                var match = suggestion.toLowerCase().indexOf(value.toLowerCase());
                if (-1 == match) {
                    return {match: true, suggestion: suggestion};
                } else {
                    var s = suggestion.substr(0, match)+"<b>"+suggestion.substr(match, value.length)+"</b>"+suggestion.substr(match+value.length);
                    return { match : true, suggestion: s};
                }
            }
        }
    );

    std::string replacerJS = INLINE_JAVASCRIPT(
        function(edit, suggestionText, suggestionValue) {
            var text = suggestionText;
            var match = text.indexOf("<b>");
            if (-1 != match) {
                text = text.substr(0, match)+text.substr(match+3);
            }
            match = text.indexOf("</b>");
            if (-1 != match) {
                text = text.substr(0, match)+text.substr(match+4);
            }
            edit.value = text;

            Wt.emit(Wt, 'search', suggestionValue);
        }
     );

    return new Wt::WSuggestionPopup(matcherJS, replacerJS, parent);
}
