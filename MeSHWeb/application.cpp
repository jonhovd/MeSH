#include "application.h"

#include <string.h>

#include <Wt/WBootstrapTheme.h>
#include <Wt/WVBoxLayout.h>

#include "global.h"

#include "header.h"
//#include "info.h"


MeSHApplication::MeSHApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_layout_is_cleared(true),
  m_content(nullptr)
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
  
	auto root_vbox = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
	root_vbox->setContentsMargins(0, 0, 0, 0);

  root_vbox->addWidget(Wt::cpp14::make_unique<Header>());
  m_content = root_vbox->addWidget(Wt::cpp14::make_unique<Content>(this)/*, 1, Wt::AlignmentFlag::Top*/);

  root()->setOverflow(Wt::Overflow::Auto);
	root()->setLayout(std::move(root_vbox));

  GetContent()->SetActiveStackedWidget(Content::TAB_INDEX_SEARCH);

  ClearLayout();

#if 0
	//page_template->bindWidget("InfoWidget", Wt::cpp14::make_unique<Info>());

    OnInternalPathChange(environment.internalPath());
#endif
}

MeSHApplication::~MeSHApplication()
{
}

void MeSHApplication::handleJavaScriptError(const std::string& UNUSED(errorText))
{
}

void MeSHApplication::OnInternalPathChange(const std::string& url)
{
  std::string meshIdInternalPath = Wt::WString::tr("MeshIdInternalPath").toUTF8();
  if (EQUAL == url.compare(Wt::WString::tr("AppStatisticsInternalPath").toUTF8()))
  {
    m_content->SetStatisticsPageIasHidden(false);
  }
  else if (EQUAL == url.compare(0, meshIdInternalPath.length(), meshIdInternalPath))
  {
    std::string meshId;
    ParseIdFromUrl(url, meshId);
//    SearchMesh(meshId);
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
    m_content->ClearLayout();
		m_layout_is_cleared = true;
	}
}
