#include "links.h"

#include <boost/algorithm/string/replace.hpp>

#include <Wt/WAnchor.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WLink.h>
#include <Wt/WText.h>


Links::Links()
: Wt::WContainerWidget(),
  m_layout(nullptr)
{
}

Links::~Links()
{
}

void Links::clear()
{
	if (m_layout)
	{
        m_layout = setLayout(std::make_unique<Wt::WVBoxLayout>());
	}
	Wt::WContainerWidget::clear();
}

void Links::populate(const Wt::WString& mesh_id, const std::string& preferred_term, const std::string& url_encoded_term, const std::string& url_encoded_filtertext)
{
	clear();

	auto layout = std::make_unique<Wt::WVBoxLayout>();
	layout->setContentsMargins(0, 0, 0, 0);
	m_layout = setLayout(std::move(layout));
	setStyleClass("mesh-links");

    m_layout->addWidget(std::make_unique<Wt::WText>(Wt::WString::tr("LinkLabel").arg(preferred_term)));

	int link_category_index = 0;
	int link_index;
	while (true)
	{
		link_category_index++;
		Wt::WString link_category_key = Wt::WString::tr("LinkCategoryFormat").arg(link_category_index);
		Wt::WString link_category_text = Wt::WString::tr(link_category_key.toUTF8());
		if ('?' == link_category_text.toUTF8().at(0))
			break;

		if (!link_category_text.trim().empty())
		{
			m_layout->addWidget(std::make_unique<Wt::WText>(link_category_text));
		}

		auto links_container = std::make_unique<Wt::WContainerWidget>();

		link_index = 0;
		while (true)
		{
			link_index++;
			Wt::WString link_text_key = Wt::WString::tr("LinkTextFormat").arg(link_category_index).arg(link_index);
			Wt::WString link_url_key = Wt::WString::tr("LinkUrlFormat").arg(link_category_index).arg(link_index);
			Wt::WString link_text = Wt::WString::tr(link_text_key.toUTF8());
			if ('?' == link_text.toUTF8().at(0))
				break;
			
			Wt::WString link_url = Wt::WString::tr(link_url_key.toUTF8()).arg(mesh_id).arg(url_encoded_term).arg(url_encoded_filtertext);
			std::string link_str = link_url.toUTF8();
			boost::algorithm::replace_all(link_str, "&amp;", "&");
            Wt::WLink link(link_str);
            link.setTarget(Wt::LinkTarget::NewWindow);
			auto anchor = std::make_unique<Wt::WAnchor>(link, link_text);
			anchor->setStyleClass("mesh-link");
			links_container->addWidget(std::move(anchor));
		}
		m_layout->addWidget(std::move(links_container));
	}
}
