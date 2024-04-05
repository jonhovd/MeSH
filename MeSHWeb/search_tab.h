#ifndef _SEARCH_TAB_H_
#define _SEARCH_TAB_H_

#include <Wt/WLineEdit.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WSuggestionPopup.h>
#include <Wt/WTemplate.h>

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

#include "elasticsearchutil.h"
#include "mesh_result.h"
#include "mesh_resultlist.h"


class MeSHApplication;
class SearchTab : public Wt::WTemplate
{
public:
  SearchTab(const Wt::WString& text, MeSHApplication* mesh_application);

public:
  void ClearLayout();

  void FocusSearchEdit();
  void OnSearch(const Wt::WString& mesh_id);

protected:
  void SearchButtonClicked();
  void OnSearchEditFocussed();
  void FilterSuggestion(const Wt::WString& filter);

private:
  std::unique_ptr<Wt::WSuggestionPopup> CreateSuggestionPopup();

public:
  static void CleanFilterString(const std::string filter_str, std::string& cleaned_filter_str);
  static void FindIndirectHit(const Json::Object& source_object, const std::string& cleaned_filter_str, std::string& indirect_hit_str);
  static void FindIndirectHit(const std::string& haystack, const std::string& needles, double& best_hit_factor, std::string& indirect_hit_str);

public:
  static void MeSHToName(std::shared_ptr<ElasticSearchUtil> es_util, const std::string& mesh_id, std::string& name);
  static void TreeNumberToName(std::shared_ptr<ElasticSearchUtil> es_util, const std::string& tree_number, std::string& name, std::string* mesh_id=nullptr);
  static void InfoFromSourceObject(const Json::Object& source_object, std::string& name, std::string* mesh_id=nullptr);
  static void InfoFromSearchResult(const Json::Object& search_result, std::string& name, std::string* mesh_id=nullptr);

private:
  MeSHApplication* m_mesh_application;

  Wt::WLineEdit* m_search_edit;
  std::unique_ptr<Wt::WSuggestionPopup> m_search_suggestion;
  std::shared_ptr<Wt::WStandardItemModel> m_search_suggestion_model;

  MeshResultList* m_mesh_resultlist;
  MeshResult* m_mesh_result;
};

#endif // _SEARCH_TAB_H_
