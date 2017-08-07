#include "application.h"

#include "global.h"

#include <Wt/WHBoxLayout>
#include <Wt/WScrollArea>
#include <Wt/WTemplate>
#include <Wt/WTable>


MeSHApplication::MeSHApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_signal(this, "search"),
  m_tabs_container(nullptr),
  m_header(nullptr),
  m_footer(nullptr)
{

	messageResourceBundle().use(appRoot() + "strings");
	useStyleSheet(Wt::WLink("MeSH.css"));

	m_es_util = new ElasticSearchUtil();

	setTitle(Wt::WString::tr("AppName"));
	addMetaHeader(Wt::MetaName, "viewport", "width=device-width, initial-scale=1.0");

	WApplication::instance()->internalPathChanged().connect(this, &MeSHApplication::onInternalPathChange);

	Wt::WVBoxLayout* root_vbox = new Wt::WVBoxLayout();
	root_vbox->setContentsMargins(0, 0, 0, 0);
	root()->setLayout(root_vbox);

	Wt::WTemplate* page_template = new Wt::WTemplate(Wt::WString::tr("PageTemplate"));
	page_template->bindWidget("HeaderWidget", CreateHeaderWidget());
	page_template->bindWidget("ContentWidget", CreateContentWidget());
	page_template->bindWidget("FooterWidget", CreateFooterWidget());
	root_vbox->addWidget(page_template);

	ClearLayout();

	onTabChanged(TAB_INDEX_SEARCH);
}

MeSHApplication::~MeSHApplication()
{
	delete m_es_util;
}

void MeSHApplication::handleJavaScriptError(const std::string& UNUSED(errorText))
{
}

void MeSHApplication::onTabChanged(int active_tab_index)
{
    switch(active_tab_index)
    {
        case TAB_INDEX_HIERARCHY: m_hierarchy->PopulateHierarchy(); break;

        case TAB_INDEX_STATISTICS: m_statistics->populate(); break;

        case TAB_INDEX_SEARCH: //Fallthrough
        default: m_search->FocusSearchEdit(); break;
    }
}

void MeSHApplication::ClearLayout()
{
	if (!m_layout_is_cleared)
	{
		m_search->ClearLayout();
		m_layout_is_cleared = true;
	}
}

void MeSHApplication::SetActiveTab(int tab_index)
{
	m_tab_widget->setCurrentIndex(tab_index);
}

void MeSHApplication::SearchMesh(const Wt::WString& mesh_id)
{
	m_search->OnSearch(mesh_id);
}

void MeSHApplication::onInternalPathChange(const std::string& url)
{
    WApplication::instance()->setInternalPath("/");

	std::string meshIdInternalPath = Wt::WString::tr("MeshIdInternalPath").toUTF8();
    if (EQUAL == url.compare(Wt::WString::tr("AppStatisticsInternalPath").toUTF8()))
    {
        m_tab_widget->setTabHidden(TAB_INDEX_STATISTICS, false);
    }
    else if (EQUAL == url.compare(0, meshIdInternalPath.length(), meshIdInternalPath))
    {
		std::string meshId;
		parseIdFromUrl(url, meshId);
		SearchMesh(meshId);
    }
}

void MeSHApplication::parseIdFromUrl(const std::string& url, std::string& id)
{
	id = "";

	// Ugly low-level parsing. Limitations in Wt and the us-asciiness of MeshID makes it work
	const char* str = url.c_str();
	const char* separator = strchr(str, '&');
	while (separator)
	{
		if (4<=strlen(separator) && EQUAL==strncmp(separator, "&id=", 4))
		{
			id = separator+4;
			break;
		}
		separator = strchr(separator+1, '&');
	}
}

Wt::WContainerWidget* MeSHApplication::CreateHeaderWidget()
{
	if (!m_header)
	{
		m_header = new Header();
	}
	return m_header;
}

Wt::WContainerWidget* MeSHApplication::CreateContentWidget()
{
	if (!m_tabs_container)
	{
		m_tabs_container = new Wt::WContainerWidget();
		m_tabs_container->setStyleClass("mesh-content");
		Wt::WHBoxLayout* tabs_hbox = new Wt::WHBoxLayout();
		tabs_hbox->setContentsMargins(0, 0, 0, 0);
		m_tabs_container->setLayout(tabs_hbox);

		m_tab_widget = new Wt::WTabWidget();
		m_tab_widget->setStyleClass("mesh-tabs");

		//Search-tab
		m_search = new Search(this);
		m_tab_widget->addTab(m_search, Wt::WString::tr("Search"));
		m_search_signal.connect(this, &MeSHApplication::SearchMesh);

		//Hierarchy-tab
		m_hierarchy = new Hierarchy(this);
		m_tab_widget->addTab(m_hierarchy, Wt::WString::tr("Hierarchy"));

		m_tab_widget->currentChanged().connect(this, &MeSHApplication::onTabChanged);

		//Statistics-tab
		m_statistics = new Statistics(this);
		m_tab_widget->addTab(m_statistics, Wt::WString::tr("Statistics"));
		m_tab_widget->setTabHidden(TAB_INDEX_STATISTICS, true);

		Wt::WContainerWidget* tmp_tab_container = new Wt::WContainerWidget(); //stretch and tabwidget doesn't mix very well. Wrap tabwidget in a containerwidget
		tmp_tab_container->addWidget(m_tab_widget);

		tabs_hbox->addStretch(1); //Add left margin
		tabs_hbox->addWidget(tmp_tab_container);
		tabs_hbox->addStretch(1); //Add right margin
	}
	return m_tabs_container;
}

Wt::WContainerWidget* MeSHApplication::CreateFooterWidget()
{
	if (!m_footer)
	{
		m_footer = new Footer();
	}
	return m_footer;
}
