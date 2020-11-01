#include "mk_counter.h"

#include <cassert>
#include <cstdio>
#include <iterator>


static constexpr auto const s_frequency = std::chrono::seconds{5};


mk::counter_t::counter_t() :
	m_name(),
	m_next(std::chrono::high_resolution_clock::now() + s_frequency),
	m_count()
{
}

mk::counter_t::counter_t(std::string const& name) :
	m_name(name),
	m_next(std::chrono::high_resolution_clock::now() + s_frequency),
	m_count()
{
}

mk::counter_t::~counter_t()
{
}

void mk::counter_t::rename(std::string const& name)
{
	m_name = name;
}

void mk::counter_t::count()
{
	++m_count;
	auto const now = std::chrono::high_resolution_clock::now();
	if(now >= m_next)
	{
		auto const diff = now - m_next + s_frequency;
		double const diff_sec = std::chrono::duration_cast<std::chrono::duration<double>>(diff).count();
		double const items_per_sec = static_cast<double>(m_count) / diff_sec;
		char buff[1 * 1024];
		int const formatted = std::snprintf(buff, std::size(buff), "%#.2f %s per second, counted %d %s in last %#.2f seconds.\n", items_per_sec, m_name.c_str(), m_count, m_name.c_str(), diff_sec);
		assert(formatted >= 0 && formatted < static_cast<int>(std::size(buff)));
		std::printf(buff);
		m_next = now + s_frequency;
		m_count = 0;
	}
}
