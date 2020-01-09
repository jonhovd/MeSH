#include "content.h"

#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>


Content::Content(MeSHApplication* mesh_application)
: Wt::WContainerWidget(),
  m_stacked_widget(nullptr),
  m_visible_stacked_widget(TAB_INDEX_SEARCH),
  m_stacked_widget_title(nullptr),
  m_statistics_page_is_hidden(true),
  m_previous_tab_button(nullptr),
  m_next_tab_button(nullptr),
  m_search(nullptr),
  m_search_signal(this, "search"),
  m_hierarchy(nullptr),
  m_statistics(nullptr)
{
  m_stacked_widget_titles[TAB_INDEX_CONTACTINFO] = Wt::WString::tr("ContactInfo");
  m_stacked_widget_titles[TAB_INDEX_SEARCH] = Wt::WString::tr("Search");
  m_stacked_widget_titles[TAB_INDEX_HIERARCHY] = Wt::WString::tr("Hierarchy");
  m_stacked_widget_titles[TAB_INDEX_STATISTICS] = Wt::WString::tr("Statistics");

  setStyleClass("mesh-content");
  
  auto layout = Wt::cpp14::make_unique<Wt::WVBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);

  //Stack selector
  auto selector_container = Wt::cpp14::make_unique<Wt::WContainerWidget>();
  auto selector_hbox = Wt::cpp14::make_unique<Wt::WHBoxLayout>();
  selector_hbox->setContentsMargins(0, 0, 0, 0);
  auto previous_tab_button = Wt::cpp14::make_unique<Wt::WPushButton>(Wt::WString("Placeholder"));
  previous_tab_button->clicked().connect(this, &Content::OnPreviousButtonClicked);
  m_previous_tab_button = selector_hbox->addWidget(std::move(previous_tab_button), 0, Wt::AlignmentFlag::Left);

  m_stacked_widget_title = selector_hbox->addWidget(Wt::cpp14::make_unique<Wt::WText>(Wt::WString("Placeholder")), 1 /*stretch*/, Wt::AlignmentFlag::Center|Wt::AlignmentFlag::Middle);

  auto next_tab_button = Wt::cpp14::make_unique<Wt::WPushButton>(Wt::WString("Placeholder"));
  next_tab_button->clicked().connect(this, &Content::OnNextButtonClicked);
  m_next_tab_button = selector_hbox->addWidget(std::move(next_tab_button), 0, Wt::AlignmentFlag::Right);
  selector_container->setLayout(std::move(selector_hbox));
  layout->addWidget(std::move(selector_container));

  //Stacked content widget
  auto stack_widget = Wt::cpp14::make_unique<Wt::WStackedWidget>();
  stack_widget->setStyleClass("stacked-content");
  Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromLeft);
  stack_widget->setTransitionAnimation(animation, true);
  m_stacked_widget = stack_widget.get();

  m_stacked_widget->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>()); //TAB_INDEX_CONTACTINFO

  m_stacked_widget->addWidget(CreateSearchWidget(mesh_application));

  m_hierarchy = m_stacked_widget->addWidget(Wt::cpp14::make_unique<Hierarchy>(mesh_application)); //TAB_INDEX_HIERARCHY

  m_statistics = m_stacked_widget->addWidget(Wt::cpp14::make_unique<Statistics>(mesh_application)); //TAB_INDEX_STATISTICS

  layout->addWidget(std::move(stack_widget), 1);

  layout->addStretch(1);

  setLayout(std::move(layout));
}

Content::~Content()
{
}

void Content::ClearLayout()
{
  m_search->ClearLayout();
}

void Content::SetActiveStackedWidget(TabId index)
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

void Content::SearchMesh(const Wt::WString& mesh_id)
{
	m_search->OnSearch(mesh_id);
}

void Content::OnPreviousButtonClicked()
{
  SetActiveStackedWidget(GetPreviousStackedWidgetIndex());
}

void Content::OnNextButtonClicked()
{
  SetActiveStackedWidget(GetNextStackedWidgetIndex());
}

Content::TabId Content::GetPreviousStackedWidgetIndex() const
{
  switch(m_visible_stacked_widget) {
    case TAB_INDEX_CONTACTINFO: return m_statistics_page_is_hidden ? TAB_INDEX_HIERARCHY : TAB_INDEX_STATISTICS;
    case TAB_INDEX_SEARCH: return TAB_INDEX_CONTACTINFO;
    case TAB_INDEX_HIERARCHY: return TAB_INDEX_SEARCH;
    default: return TAB_INDEX_HIERARCHY;
  }
}

Content::TabId Content::GetNextStackedWidgetIndex() const
{
  switch(m_visible_stacked_widget) {
    case TAB_INDEX_CONTACTINFO: return TAB_INDEX_SEARCH;
    case TAB_INDEX_SEARCH: return TAB_INDEX_HIERARCHY;
    case TAB_INDEX_HIERARCHY: return m_statistics_page_is_hidden ? TAB_INDEX_CONTACTINFO : TAB_INDEX_STATISTICS;
    default: return TAB_INDEX_CONTACTINFO;
  }
}

void Content::SetStatisticsPageIasHidden(bool hidden)
{
  m_statistics_page_is_hidden = hidden;
  if (TAB_INDEX_STATISTICS == m_visible_stacked_widget)
  {
    SetActiveStackedWidget(GetNextStackedWidgetIndex());
  }
}

std::unique_ptr<Wt::WContainerWidget> Content::CreateSearchWidget(MeSHApplication* mesh_application)
{
  auto search = Wt::cpp14::make_unique<Search>(mesh_application);
  m_search = search.get();
  m_search_signal.connect(this, &Content::SearchMesh);
  return std::move(std::move(search));
}
