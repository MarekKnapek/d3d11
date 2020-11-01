#pragma once


#include <chrono>
#include <string>


namespace mk
{


	class counter_t
	{
	public:
		counter_t();
		counter_t(std::string const& name);
		~counter_t();
	public:
		void rename(std::string const& name);
		void count();
	private:
		int m_count;
		std::chrono::high_resolution_clock::time_point m_next;
		std::string m_name;
	};


}
