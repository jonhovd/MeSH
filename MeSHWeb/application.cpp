#include "application.h"

#include <string.h>

#include <Wt/WBootstrapTheme.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WPushButton.h>
#include <Wt/WVBoxLayout.h>

#include "global.h"
#include "header.h"
//#include "info.h"


MeSHApplication::MeSHApplication(const Wt::WEnvironment& environment)
: Wt::WApplication(environment),
  m_stacked_widget(nullptr),
  m_layout_is_cleared(true),
  m_visible_stacked_widget(TAB_INDEX_SEARCH),
  m_stacked_widget_title(nullptr),
  m_statistics_page_is_hidden(true),
  m_previous_tab_button(nullptr),
  m_next_tab_button(nullptr),
  m_statistics(nullptr),
  m_search(nullptr),
  m_search_signal(this, "search"),
  m_hierarchy(nullptr)
{
  messageResourceBundle().use(appRoot() + "strings");

  m_es_util = std::make_shared<ElasticSearchUtil>();

  setTitle(Wt::WString::tr("AppName"));

  m_stacked_widget_titles[TAB_INDEX_CONTACTINFO] = Wt::WString::tr("ContactInfo");
  m_stacked_widget_titles[TAB_INDEX_SEARCH] = Wt::WString::tr("Search");
  m_stacked_widget_titles[TAB_INDEX_HIERARCHY] = Wt::WString::tr("Hierarchy");
  m_stacked_widget_titles[TAB_INDEX_STATISTICS] = Wt::WString::tr("Statistics");

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
  root_vbox->addWidget(CreateContentWidget(), 1, Wt::AlignmentFlag::Top);

  root()->setOverflow(Wt::Overflow::Auto);
	root()->setLayout(std::move(root_vbox));

  SetActiveStackedWidget(TAB_INDEX_SEARCH);

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

void MeSHApplication::ClearLayout()
{
	if (!m_layout_is_cleared)
	{
		m_search->ClearLayout();
		m_layout_is_cleared = true;
	}
}

void MeSHApplication::SetActiveStackedWidget(TabId index)
{
  m_visible_stacked_widget = index;
  m_stacked_widget->setCurrentIndex(m_visible_stacked_widget);
  m_previous_tab_button->setText(Wt::WString::tr("PreviousPrefix") + m_stacked_widget_titles[GetPreviousStackedWidgetIndex()]);
  m_stacked_widget_title->setText(m_stacked_widget_titles[m_visible_stacked_widget]);
  m_next_tab_button->setText(m_stacked_widget_titles[GetNextStackedWidgetIndex()] + Wt::WString::tr("NextPostfix"));
  if (index == TAB_INDEX_SEARCH)
  {
    m_search->FocusSearchEdit();
  }
  else if (index == TAB_INDEX_HIERARCHY)
  {
    m_hierarchy->PopulateHierarchy();
  }
  else if (index == TAB_INDEX_STATISTICS)
  {
    m_statistics->populate();
  }
}

void MeSHApplication::SearchMesh(const Wt::WString& mesh_id)
{
	m_search->OnSearch(mesh_id);
}

void MeSHApplication::OnInternalPathChange(const std::string& url)
{
  std::string meshIdInternalPath = Wt::WString::tr("MeshIdInternalPath").toUTF8();
  if (EQUAL == url.compare(Wt::WString::tr("AppStatisticsInternalPath").toUTF8()))
  {
    m_statistics_page_is_hidden = false;
  }
  else if (EQUAL == url.compare(0, meshIdInternalPath.length(), meshIdInternalPath))
  {
    std::string meshId;
    ParseIdFromUrl(url, meshId);
//    SearchMesh(meshId);
  }
}

void MeSHApplication::OnPreviousButtonClicked()
{
  SetActiveStackedWidget(GetPreviousStackedWidgetIndex());
}

void MeSHApplication::OnNextButtonClicked()
{
  SetActiveStackedWidget(GetNextStackedWidgetIndex());
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

MeSHApplication::TabId MeSHApplication::GetPreviousStackedWidgetIndex() const
{
  switch(m_visible_stacked_widget) {
    case TAB_INDEX_CONTACTINFO: return m_statistics_page_is_hidden ? TAB_INDEX_HIERARCHY : TAB_INDEX_STATISTICS;
    case TAB_INDEX_SEARCH: return TAB_INDEX_CONTACTINFO;
    case TAB_INDEX_HIERARCHY: return TAB_INDEX_SEARCH;
    default: return TAB_INDEX_HIERARCHY;
  }
}

MeSHApplication::TabId MeSHApplication::GetNextStackedWidgetIndex() const
{
  switch(m_visible_stacked_widget) {
    case TAB_INDEX_CONTACTINFO: return TAB_INDEX_SEARCH;
    case TAB_INDEX_SEARCH: return TAB_INDEX_HIERARCHY;
    case TAB_INDEX_HIERARCHY: return m_statistics_page_is_hidden ? TAB_INDEX_CONTACTINFO : TAB_INDEX_STATISTICS;
    default: return TAB_INDEX_CONTACTINFO;
  }
}

std::unique_ptr<Wt::WContainerWidget> MeSHApplication::CreateContentWidget()
{
  auto content_container = Wt::cpp14::make_unique<Wt::WContainerWidget>();
  //content_container->setStyleClass("mesh-content");
  auto content_vbox = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
  content_vbox->setContentsMargins(0, 0, 0, 0);

  //Stack selector
  auto selector_container = Wt::cpp14::make_unique<Wt::WContainerWidget>();
  auto selector_hbox = Wt::cpp14::make_unique<Wt::WHBoxLayout>();
  selector_hbox->setContentsMargins(0, 0, 0, 0);
  auto previous_tab_button = Wt::cpp14::make_unique<Wt::WPushButton>(Wt::WString("Placeholder"));
  previous_tab_button->clicked().connect(this, &MeSHApplication::OnPreviousButtonClicked);
  m_previous_tab_button = selector_hbox->addWidget(std::move(previous_tab_button), 0, Wt::AlignmentFlag::Left);

  m_stacked_widget_title = selector_hbox->addWidget(Wt::cpp14::make_unique<Wt::WText>(Wt::WString("Placeholder")), 1 /*stretch*/, Wt::AlignmentFlag::Center|Wt::AlignmentFlag::Middle);

  auto next_tab_button = Wt::cpp14::make_unique<Wt::WPushButton>(Wt::WString("Placeholder"));
  next_tab_button->clicked().connect(this, &MeSHApplication::OnNextButtonClicked);
  m_next_tab_button = selector_hbox->addWidget(std::move(next_tab_button), 0, Wt::AlignmentFlag::Right);
  selector_container->setLayout(std::move(selector_hbox));
  content_vbox->addWidget(std::move(selector_container));

  //Stacked content widget
  auto stack_widget = Wt::cpp14::make_unique<Wt::WStackedWidget>();
  Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromLeft);
  stack_widget->setTransitionAnimation(animation, true);
  m_stacked_widget = stack_widget.get();

  m_stacked_widget->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>()); //TAB_INDEX_CONTACTINFO

  m_stacked_widget->addWidget(CreateSearchWidget());

  m_hierarchy = m_stacked_widget->addWidget(Wt::cpp14::make_unique<Hierarchy>(this)); //TAB_INDEX_HIERARCHY

  m_statistics = m_stacked_widget->addWidget(Wt::cpp14::make_unique<Statistics>(this)); //TAB_INDEX_STATISTICS

  content_vbox->addWidget(std::move(stack_widget), 1);

  content_vbox->addStretch(1);

  content_container->setLayout(std::move(content_vbox));
  return std::move(content_container);
}

std::unique_ptr<Wt::WContainerWidget> MeSHApplication::CreateSearchWidget()
{
  auto search = Wt::cpp14::make_unique<Search>(this);
  m_search = search.get();
  m_search_signal.connect(this, &MeSHApplication::SearchMesh);
  return std::move(std::move(search));
}
