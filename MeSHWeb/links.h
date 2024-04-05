#ifndef _LINKS_H_
#define _LINKS_H_

#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>


class Links : public Wt::WContainerWidget
{
public:
	Links();

public:
  virtual void populate(const Wt::WString& mesh_id, const std::string& preferred_term, const std::string& url_encoded_term, const std::string& url_encoded_filtertext);

private:
  Wt::WVBoxLayout* m_layout;
};

#endif // _LINKS_H_
