#include "application.h"

#include "global.h"

#include <string.h>

#include <Wt/WHBoxLayout.h>
#include <Wt/WTemplate.h>
#include <Wt/WTable.h>

#include "info.h"


MeSHApplication::MeSHApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_search_signal(this, "search"),
  m_layout_is_cleared(true),
  m_statistics(nullptr),
  m_search(nullptr),
  m_hierarchy(nullptr)
{

	messageResourceBundle().use(appRoot() + "strings");
	useStyleSheet(Wt::WLink("MeSH.css"));

	m_es_util = std::make_shared<ElasticSearchUtil>();

	setTitle(Wt::WString::tr("AppName"));

	WApplication::instance()->internalPathChanged().connect(this, &MeSHApplication::onInternalPathChange);

	auto root_vbox = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
	root_vbox->setContentsMargins(0, 0, 0, 0);

	auto page_template = Wt::cpp14::make_unique<Wt::WTemplate>(Wt::WString::tr("PageTemplate"));
	page_template->bindWidget("HeaderWidget", Wt::cpp14::make_unique<Header>());
	page_template->bindWidget("ContentWidget", InitializeContentWidget());
    
	page_template->bindWidget("InfoWidget", Wt::cpp14::make_unique<Info>());

    root_vbox->addWidget(std::move(page_template));

	root()->setLayout(std::move(root_vbox));

    ClearLayout();

	onTabChanged(TAB_INDEX_SEARCH);

    onInternalPathChange(environment.internalPath());
}

MeSHApplication::~MeSHApplication()
{
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
    //WApplication::instance()->setInternalPath("/");

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

std::unique_ptr<Wt::WContainerWidget> MeSHApplication::InitializeContentWidget()
{
    auto tabs_container = Wt::cpp14::make_unique<Wt::WContainerWidget>();
    tabs_container->setStyleClass("mesh-content");
    auto tabs_hbox = Wt::cpp14::make_unique<Wt::WHBoxLayout>();
    tabs_hbox->setContentsMargins(0, 0, 0, 0);

    auto tab_widget = Wt::cpp14::make_unique<Wt::WTabWidget>();
    tab_widget->setStyleClass("mesh-tabs");

    //Search-tab
    auto search = Wt::cpp14::make_unique<Search>(this);
    m_search = search.get();
    tab_widget->addTab(std::move(search), Wt::WString::tr("Search"));
    m_search_signal.connect(this, &MeSHApplication::SearchMesh);

    //Hierarchy-tab
    auto hierarchy = Wt::cpp14::make_unique<Hierarchy>(this);
    m_hierarchy = hierarchy.get();
    tab_widget->addTab(std::move(hierarchy), Wt::WString::tr("Hierarchy"));

    tab_widget->currentChanged().connect(this, &MeSHApplication::onTabChanged);

    //Statistics-tab
    auto statistics = Wt::cpp14::make_unique<Statistics>(this);
    m_statistics = statistics.get();
    tab_widget->addTab(std::move(statistics), Wt::WString::tr("Statistics"));
    tab_widget->setTabHidden(TAB_INDEX_STATISTICS, true);

    auto tmp_tab_container = Wt::cpp14::make_unique<Wt::WContainerWidget>(); //stretch and tabwidget doesn't mix very well. Wrap tabwidget in a containerwidget
    m_tab_widget = tmp_tab_container->addWidget(std::move(tab_widget));

    tabs_hbox->addWidget(std::move(tmp_tab_container));

    tabs_container->setLayout(std::move(tabs_hbox));

    return std::move(tabs_container);
}
