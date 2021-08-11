#include "mesh_result.h"

#include <boost/algorithm/string/replace.hpp>

#include <Wt/WAnchor.h>
#include <Wt/WImage.h>
#include <Wt/WStandardItem.h>
#include <Wt/WStringListModel.h>
#include <Wt/Utils.h>

#include "application.h"
#include "global.h"
#include "log.h"


MeshResult::MeshResult(const Wt::WString& text, MeSHApplication* mesh_application)
: Wt::WTemplate(text),
  m_mesh_application(mesh_application),
  m_nor_term_panel(nullptr),
  m_nor_term_container(nullptr),
  m_nor_term_panel_layout(nullptr),
  m_nor_description_text(nullptr),
  m_eng_term_panel(nullptr),
  m_eng_term_container(nullptr),
  m_eng_term_panel_layout(nullptr),
  m_eng_description_text(nullptr),
  m_external_link(nullptr),
  m_see_related_container(nullptr),
  m_links(nullptr),
  m_hierarchy_tree_view(nullptr)
{
  auto nor_term_panel = std::make_unique<Wt::WPanel>();
  nor_term_panel->setCollapsible(true);
  m_nor_term_container = nor_term_panel->setCentralWidget(std::make_unique<Wt::WContainerWidget>());

  auto nor_flag = std::make_unique<Wt::WImage>("images/nor.png");
  nor_flag->setHeight(Wt::WLength(1.0, Wt::LengthUnit::FontEm));
  nor_flag->setMargin(Wt::WLength(3.0, Wt::LengthUnit::Pixel));
  nor_term_panel->titleBarWidget()->insertWidget(0, std::move(nor_flag));

  m_nor_term_panel = bindWidget("nor_panel", std::move(nor_term_panel));

  m_nor_description_text = bindWidget("nor_description", std::make_unique<Wt::WText>());

  auto eng_term_panel = std::make_unique<Wt::WPanel>();
  eng_term_panel->setCollapsible(true);
  m_eng_term_container = eng_term_panel->setCentralWidget(std::make_unique<Wt::WContainerWidget>());

  auto eng_flag = std::make_unique<Wt::WImage>("images/eng.png");
  eng_flag->setHeight(Wt::WLength(1.0, Wt::LengthUnit::FontEm));
  eng_flag->setMargin(Wt::WLength(3.0, Wt::LengthUnit::Pixel));
  eng_term_panel->titleBarWidget()->insertWidget(0, std::move(eng_flag));

  m_eng_term_panel = bindWidget("eng_panel", std::move(eng_term_panel));

  m_eng_description_text = bindWidget("eng_description", std::make_unique<Wt::WText>());

  auto external_link_anchor = std::make_unique<Wt::WAnchor>();
  external_link_anchor->setStyleClass("mesh-link");
  m_external_link = bindWidget("external_link", std::move(external_link_anchor));

  m_see_related_container = bindWidget("see_related", std::make_unique<Wt::WContainerWidget>());

  m_links = bindWidget("links", std::make_unique<Links>());

  m_hierarchy_model = std::make_shared<Wt::WStandardItemModel>();
  m_hierarchy_model->setSortRole(HIERARCHY_ITEM_TREE_NUMBER_ROLE);
  auto hierarchy_tree_view = std::make_unique<Wt::WTreeView>();
  hierarchy_tree_view->setModel(m_hierarchy_model);
  hierarchy_tree_view->setSelectionMode(Wt::SelectionMode::Single);
  hierarchy_tree_view->setColumnWidth(0, Wt::WLength::Auto);
  hierarchy_tree_view->clicked().connect(this, &MeshResult::TreeItemClicked);
  m_hierarchy_tree_view = bindWidget("hierarchy", std::move(hierarchy_tree_view));
}

void MeshResult::ClearLayout()
{
  m_nor_term_panel->setTitle("");
  m_nor_term_panel->expand();
  m_nor_term_panel_layout = m_nor_term_container->setLayout(std::make_unique<Wt::WVBoxLayout>());

  m_nor_description_text->setText("");
  m_nor_description_text->hide();

  m_eng_term_panel->setTitle("");
  m_eng_term_panel->collapse();
  m_eng_term_panel_layout = m_eng_term_container->setLayout(std::make_unique<Wt::WVBoxLayout>());

  m_eng_description_text->setText("");
  m_eng_description_text->hide();

  m_external_link->setText("");
  m_external_link->hide();
  
  setCondition("show-related", false);
  m_see_related_container->clear();

  m_links->clear();

  m_hierarchy_model->clear();
}

void MeshResult::OnSearch(const Wt::WString& mesh_id, const std::string& search_text)
{
  std::string preferred_term;
  std::string preferred_eng_term;

  m_nor_term_panel_layout = m_nor_term_container->setLayout(std::make_unique<Wt::WVBoxLayout>());
  m_nor_term_panel_layout->setContentsMargins(0, 0, 0, 0);
  m_eng_term_panel_layout = m_eng_term_container->setLayout(std::make_unique<Wt::WVBoxLayout>());
  m_eng_term_panel_layout->setContentsMargins(0, 0, 0, 0);

  m_nor_description_text->hide();
  m_eng_description_text->hide();

  setCondition("show-related", false);
  m_see_related_container->clear();

  m_hierarchy_model->clear();

  Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id.toUTF8());

  Json::Object search_result;
  auto es_util = m_mesh_application->GetElasticSearchUtil();
  long result_size = es_util->search("mesh", query.toUTF8(), search_result);
  if (0 == result_size)
  {
    return;
  }

  MeshLog log(m_mesh_application);
  log.LogSearch(mesh_id.toUTF8());

  const Json::Value value = search_result.getValue("hits");
  const Json::Object value_object = value.getObject();
  const Json::Value hits_value = value_object.getValue("hits");
  const Json::Array hits_array = hits_value.getArray();
  const Json::Value hit_value = hits_array.first();
  const Json::Object hit_value_object = hit_value.getObject();
  const Json::Value source_value = hit_value_object.getValue("_source");
  const Json::Object source_object = source_value.getObject();

  if (source_object.member("nor_preferred_term_text"))
  {
    preferred_term = source_object.getValue("nor_preferred_term_text").getArray().first().getString();
    m_nor_term_panel->setTitle(preferred_term);
    m_nor_term_panel->expand();
    m_eng_term_panel->collapse();
  }
  else
  {
    m_nor_term_panel->setTitle(Wt::WString::tr("NotTranslated"));
    m_nor_term_panel->collapse();
    m_eng_term_panel->expand();
  }
  
  if (source_object.member("eng_preferred_term_text"))
  {
    preferred_eng_term = source_object.getValue("eng_preferred_term_text").getArray().first().getString();
    if (preferred_term.empty())
    {
      preferred_term = preferred_eng_term;
    }
    m_eng_term_panel->setTitle(source_object.getValue("eng_preferred_term_text").getArray().first().getString());
  }

  if (source_object.member("nor_description"))
  {
    std::string description = source_object.getValue("nor_description").getString();
    boost::algorithm::replace_all(description, "\\n", "\n");
    SetAndActivateDescription(m_nor_description_text, description);
  }
  
  if (source_object.member("eng_description"))
  {
    std::string description = source_object.getValue("eng_description").getString();
    boost::algorithm::replace_all(description, "\\n", "\n");
    SetAndActivateDescription(m_eng_description_text, description);
  }
  
  if (source_object.member("nor_other_term_texts"))
  {
    m_nor_term_panel_layout->addWidget(std::make_unique<Wt::WText>(Wt::WString::tr("NonPreferredNorwegianTerms")));
    SetOtherTermTexts(m_nor_term_panel_layout, source_object.getValue("nor_other_term_texts").getArray());
  }
  
  if (source_object.member("eng_other_term_texts"))
  {
    m_eng_term_panel_layout->addWidget(std::make_unique<Wt::WText>(Wt::WString::tr("NonPreferredEnglishTerms")));
    SetOtherTermTexts(m_eng_term_panel_layout, source_object.getValue("eng_other_term_texts").getArray());
  }

  std::string url = (Wt::WString::tr("MeshIdInternalPath")+"&"+Wt::WString::tr("MeshIdInternalPathParam").arg(mesh_id)).toUTF8();
  m_external_link->setText(m_mesh_application->makeAbsoluteUrl(url));
  m_external_link->setLink(Wt::WLink(Wt::LinkType::InternalPath, url));
  m_external_link->show();

  //See Related
  if (source_object.member("see_related"))
  {
    setCondition("show-related", true);

    const Json::Array see_related_array = source_object.getValue("see_related").getArray();
    Json::Array::const_iterator see_related_iterator = see_related_array.begin();
    for (; see_related_iterator!=see_related_array.end(); ++see_related_iterator)
    {
      const Json::Value see_related_value = *see_related_iterator;

      std::string see_related_id = see_related_value.getString();
      std::string title;
      SearchTab::MeSHToName(es_util, see_related_id, title);
      std::string url = (Wt::WString::tr("MeshIdInternalPath")+"&"+Wt::WString::tr("MeshIdInternalPathParam").arg(see_related_id)).toUTF8();
      auto see_related_anchor = std::make_unique<Wt::WAnchor>(Wt::WLink(Wt::LinkType::InternalPath, url), Wt::WString::fromUTF8(title));
      see_related_anchor->setStyleClass("mesh-link");
      m_see_related_container->addWidget(std::move(see_related_anchor));
    }
  }

  //Links
  std::string url_encoded_term = Wt::Utils::urlEncode(preferred_eng_term);
  std::string url_encoded_filtertext = Wt::Utils::urlEncode(search_text);
	m_links->populate(mesh_id, preferred_term, url_encoded_term, url_encoded_filtertext);

  PopulateHierarchy(es_util, source_object);

  //Mark search-result in hierarchy search tab
  HierarchyTab* hierarchy = m_mesh_application->GetHierarchy();
  hierarchy->ClearMarkedItems();
  hierarchy->Collapse();
  if (source_object.member("tree_numbers"))
  {
    const Json::Array tree_numbers_array = source_object.getValue("tree_numbers").getArray();
    Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
    for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
    {
      const Json::Value tree_number_value = *tree_numbers_iterator;
      hierarchy->ExpandToTreeNumber(tree_number_value.getString());
    }
  }
}

void MeshResult::TreeItemClicked(const Wt::WModelIndex& index, const Wt::WMouseEvent& mouse)
{
  if (!index.isValid())
  {
    return;
  }

  Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(index);
  if (!standard_item)
  {
    return;
  }

  m_hierarchy_popup_menu = std::make_unique<Wt::WPopupMenu>();
  m_hierarchy_popup_menu->setAutoHide(true, 1000);

  m_popup_menu_id_string = Wt::cpp17::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_ID_ROLE));

  Wt::WString soek = Wt::WString::tr("SearchFromHierarchy").arg(standard_item->text().toUTF8());
  m_hierarchy_popup_menu->addItem(soek)->triggered().connect(this, &MeshResult::PopupMenuTriggered);

  m_hierarchy_popup_menu->popup(mouse);
}

void MeshResult::PopupMenuTriggered(Wt::WMenuItem* item)
{
  if (item && !m_popup_menu_id_string.empty())
  {
    m_mesh_application->GetSearch()->OnSearch(m_popup_menu_id_string);
  }
}

void MeshResult::SetAndActivateDescription(Wt::WText* text_ctrl, const std::string& text)
{
  text_ctrl->setTextFormat(Wt::TextFormat::Plain);
  text_ctrl->setText(Wt::WString::fromUTF8(text));
  text_ctrl->show();
}

void MeshResult::SetOtherTermTexts(Wt::WLayout* term_layout, const Json::Array& terms_array)
{
  Json::Array::const_iterator term_iterator = terms_array.begin();
  for (; term_iterator!=terms_array.end(); ++term_iterator)
  {
    const Json::Value term_value = *term_iterator;
    term_layout->addWidget(std::make_unique<Wt::WText>(Wt::WString::fromUTF8(term_value.getString()), Wt::TextFormat::Plain));
  }
}

void MeshResult::RecursiveAddHierarchyItem(std::shared_ptr<ElasticSearchUtil> es_util, int& row, std::map<std::string, Wt::WStandardItem*>& node_map, const std::string& tree_number, bool mark_item) {
  if (tree_number.empty() || //parent of top-node
    0<node_map.count(tree_number)) //Already added?
  {
    return;
  }

  //Make sure parent is added
  std::string parent_tree_number;
  HierarchyTab::GetParentTreeNumber(tree_number, parent_tree_number);
  RecursiveAddHierarchyItem(es_util, row, node_map, parent_tree_number, false);

  std::string name;
  std::string mesh_id;
  SearchTab::TreeNumberToName(es_util, tree_number, name, &mesh_id);
  std::stringstream node_text;
  node_text << name;
  if (!tree_number.empty())
  {
    node_text << " [" << tree_number << "]";
  }
  auto item = std::make_unique<Wt::WStandardItem>(Wt::WString::fromUTF8(node_text.str()));
  auto item_ptr = item.get();
  item->setData(Wt::cpp17::any(tree_number), HIERARCHY_ITEM_TREE_NUMBER_ROLE);
  item->setData(Wt::cpp17::any(mesh_id), HIERARCHY_ITEM_ID_ROLE);

  if (mark_item)
  {
    item->setStyleClass("marked_item");
  }

  std::map<std::string,Wt::WStandardItem*>::iterator iter = node_map.find(parent_tree_number);
  if (node_map.end() == iter) //No parent -> top-node
  {
    m_hierarchy_model->setItem(row++, 0, std::move(item));
  }
  else
  {
    Wt::WStandardItem* parent_item = iter->second;
    parent_item->setChild(parent_item->rowCount(), 0, std::move(item));
    m_hierarchy_tree_view->expand(parent_item->index());
  }

  node_map[tree_number] = item_ptr;
}

void MeshResult::PopulateHierarchy(std::shared_ptr<ElasticSearchUtil> es_util, const Json::Object& source_object)
{
  m_hierarchy_model->clear();
  int row = 0;

  std::map<std::string,Wt::WStandardItem*> node_map;

  if (source_object.member("tree_numbers"))
  {
    const Json::Array tree_numbers_array = source_object.getValue("tree_numbers").getArray();
    Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
    for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
    {
      const Json::Value tree_number_value = *tree_numbers_iterator;
      RecursiveAddHierarchyItem(es_util, row, node_map, tree_number_value.getString(), true);
    }
  }
  if (source_object.member("child_tree_numbers"))
  {
    const Json::Array child_tree_numbers_array = source_object.getValue("child_tree_numbers").getArray();
    Json::Array::const_iterator child_tree_numbers_iterator = child_tree_numbers_array.begin();
    for (; child_tree_numbers_iterator!=child_tree_numbers_array.end(); ++child_tree_numbers_iterator)
    {
      const Json::Value child_tree_number_value = *child_tree_numbers_iterator;
      RecursiveAddHierarchyItem(es_util, row, node_map, child_tree_number_value.getString(), false);
    }
  }
}
