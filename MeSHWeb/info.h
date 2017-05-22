#ifndef _INFO_H_
#define _INFO_H_

#include <Wt/WContainerWidget>
#include <Wt/WVBoxLayout>


class Info : public Wt::WContainerWidget
{
public:
	Info(Wt::WContainerWidget* parent = 0);
	~Info();

private:
   Wt::WVBoxLayout* m_layout;
};

#endif // _INFO_H_
