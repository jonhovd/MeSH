#include "application.h"

#include "global.h"

#include <Wt/WHBoxLayout>


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
	Wt::WContainerWidget* header_container = new Wt::WContainerWidget();
	Wt::WHBoxLayout* header_hbox = new Wt::WHBoxLayout();
	header_hbox->setContentsMargins(0, 0, 0, 0);
	header_container->setLayout(header_hbox);
	m_app_name = new AppName();
	header_hbox->addWidget(m_app_name);
	root_vbox->addWidget(header_container);

	//Tabs
	m_tab_widget = new Wt::WTabWidget();
	root_vbox->addWidget(m_tab_widget);

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

	root_vbox->addStretch(1);

	//Footer
	Wt::WContainerWidget* footer_container = new Wt::WContainerWidget();
	Wt::WHBoxLayout* footer_hbox = new Wt::WHBoxLayout();
	footer_hbox->setContentsMargins(0, 0, 0, 0);
	footer_container->setLayout(footer_hbox);
	m_logo = new Logo();
	footer_hbox->addWidget(m_logo);
	footer_hbox->addStretch(1);
	m_info = new Info();
	footer_hbox->addWidget(m_info);
	root_vbox->addWidget(footer_container);

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
    if (EQUAL == url.compare(Wt::WString::tr("AppStatisticsInternalPath").toUTF8()))
    {
        m_tab_widget->setTabHidden(TAB_INDEX_STATISTICS, false);
    }
    WApplication::instance()->setInternalPath("/");
}
