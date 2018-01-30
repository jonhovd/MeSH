#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include <Wt/WContainerWidget.h>
#include <Wt/WGridLayout.h>


class MeSHApplication;
class Statistics : public Wt::WContainerWidget
{
public:
	Statistics(const MeSHApplication* mesh_application);
	~Statistics();

public:
  virtual void clear();
  virtual void populate();

private:
	void PopulateDayStatistics();
	void PopulateTextStatistics();

private:
	const MeSHApplication* m_mesh_application;
	Wt::WGridLayout* m_layout;
};

#endif // _STATISTICS_H_
