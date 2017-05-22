#ifndef _LOGO_H_
#define _LOGO_H_

#include <Wt/WContainerWidget>
#include <Wt/WVBoxLayout>


class Logo : public Wt::WContainerWidget
{
public:
	Logo(Wt::WContainerWidget* parent = 0);
	~Logo();

private:
   Wt::WVBoxLayout* m_layout;
};

#endif // _LOGO_H_
