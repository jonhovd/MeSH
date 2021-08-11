#include "application.h"

#include <string.h>

#include <Wt/WBootstrapTheme.h>
#include <Wt/WTemplate.h>

#include "global.h"

#include "about_tab.h"
#include "search_tab.h"
#include "hierarchy_tab.h"


MeSHApplication::MeSHApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_layout_is_cleared(true),
  m_tab_widget(nullptr),
  m_hierarchy_tab(nullptr),
  m_search_tab(nullptr),
  m_search_signal(this, "search")
{
  messageResourceBundle().use(appRoot() + "strings");

  m_es_util = std::make_shared<ElasticSearchUtil>();

  setTitle(Wt::WString::tr("AppName"));

  //Set standard styling
  auto bootstrapTheme = std::make_shared<Wt::WBootstrapTheme>();
  bootstrapTheme->setVersion(Wt::BootstrapVersion::v3);
  bootstrapTheme->setResponsive(true);
  setTheme(bootstrapTheme);
  // load the default bootstrap3 (sub-)theme
  useStyleSheet("resources/themes/bootstrap/3/bootstrap-theme.min.css");

  //Set custom styling
  useStyleSheet("MeSH.css");

  WApplication::instance()->internalPathChanged().connect(this, &MeSHApplication::OnInternalPathChange);
  
  auto t = std::make_unique<Wt::WTemplate>(Wt::WString::tr("pageTemplate"));
  
  auto tabWidget = std::make_unique<Wt::WTabWidget>();

  auto search_tab = std::make_unique<SearchTab>(Wt::WString::tr("searchTabTemplate"), this);
  m_search_tab = search_tab.get();
  tabWidget->addTab(std::move(search_tab), Wt::WString::tr("Search"));

  auto hierarchy_tab = std::make_unique<HierarchyTab>(Wt::WString::tr("hierarchyTabTemplate"), this);
  m_hierarchy_tab = hierarchy_tab.get();
  tabWidget->addTab(std::move(hierarchy_tab), Wt::WString::tr("Hierarchy"));
  tabWidget->currentChanged().connect(this, &MeSHApplication::OnTabChanged);

  tabWidget->addTab(std::make_unique<AboutTab>(Wt::WString::tr("aboutTabTemplate"), this), Wt::WString::tr("About"));

  m_tab_widget = t->bindWidget("content", std::move(tabWidget));
  root()->addWidget(std::move(t));

  m_search_signal.connect(this, &MeSHApplication::OnSearch);
  
  m_search_tab->FocusSearchEdit();

  OnInternalPathChange(environment.internalPath());
}

void MeSHApplication::handleJavaScriptError(const std::string& UNUSED(errorText))
{
}

void MeSHApplication::OnInternalPathChange(const std::string& url)
{
  std::string meshIdInternalPath = Wt::WString::tr("MeshIdInternalPath").toUTF8();
  if (EQUAL == url.compare(0, meshIdInternalPath.length(), meshIdInternalPath))
  {
    std::string meshId;
    ParseIdFromUrl(url, meshId);
    OnSearch(meshId);
  }
}

void MeSHApplication::OnTabChanged(int index)
{
  if (TAB_INDEX_SEARCH == index)
  {
    m_search_tab->FocusSearchEdit();
  }
  else if (TAB_INDEX_HIERARCHY == index)
  {
    m_hierarchy_tab->PopulateHierarchy();
  }
  else if (TAB_INDEX_ABOUT == index)
  {
  }
}

void MeSHApplication::ParseIdFromUrl(const std::string& url, std::string& id)
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

void MeSHApplication::ClearLayout()
{
  if (!m_layout_is_cleared)
  {
    GetSearch()->ClearLayout();
    m_layout_is_cleared = true;
  }
}

void MeSHApplication::SetActiveTab(const TabId& index)
{
  m_tab_widget->setCurrentIndex(index);
}
