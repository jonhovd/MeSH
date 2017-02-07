#include "application.h"

#include "global.h"
#include "header.h"
#include "footer.h"

#include <Wt/WHBoxLayout>
#include <Wt/WScrollArea>


MeSHApplication::MeSHApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_signal(this, "search")
{
	messageResourceBundle().use(appRoot() + "strings");
	useStyleSheet(Wt::WLink("MeSH.css"));

	m_es_util = new ElasticSearchUtil();

	setTitle(Wt::WString::tr("AppName"));

	WApplication::instance()->internalPathChanged().connect(this, &MeSHApplication::onInternalPathChange);

	Wt::WVBoxLayout* root_vbox = new Wt::WVBoxLayout();
	root_vbox->setContentsMargins(0, 0, 0, 0);
	root()->setLayout(root_vbox);

	//Header
	Header* header = new Header();
	root_vbox->addWidget(header);

	//Tabs
	Wt::WContainerWidget* tabs_container = new Wt::WContainerWidget();
	Wt::WHBoxLayout* tabs_hbox = new Wt::WHBoxLayout();
	tabs_hbox->setContentsMargins(0, 0, 0, 0);
	tabs_container->setLayout(tabs_hbox);

	m_tab_widget = new Wt::WTabWidget();
	m_tab_widget->setStyleClass("mesh-tabs");

	//Search-tab
	m_search = new Search(this);
	Wt::WScrollArea* scroll_area = new Wt::WScrollArea();
	scroll_area->setWidget(m_search);
	m_tab_widget->addTab(scroll_area, Wt::WString::tr("Search"));
    m_search_signal.connect(this, &MeSHApplication::SearchMesh);

	//Hierarchy-tab
	m_hierarchy = new Hierarchy(this);
	m_tab_widget->addTab(m_hierarchy, Wt::WString::tr("Hierarchy"));

	m_tab_widget->currentChanged().connect(this, &MeSHApplication::onTabChanged);

	//Statistics-tab
	m_statistics = new Statistics(this);
	m_tab_widget->addTab(m_statistics, Wt::WString::tr("Statistics"));
	m_tab_widget->setTabHidden(TAB_INDEX_STATISTICS, true);

	tabs_hbox->addStretch(1); //Add left margin
	tabs_hbox->addWidget(m_tab_widget);
	tabs_hbox->addStretch(1); //Add right margin

	root_vbox->addWidget(tabs_container, 1); //Header and footer is static. tabs_container should take rest of available space

	//Footer
	Footer* footer = new Footer();
	root_vbox->addWidget(footer);

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
