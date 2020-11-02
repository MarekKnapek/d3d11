#include "ring_buffer_spsc.h"

#include <vector>
#include <string>


static int const ring_buffer_spsc = ([]()
{
	{
		mk::ring_buffer_spsc_t<std::string, 8> buffer;

		std::vector<std::string> vec;
		vec.push_back("1 ========== ==========");
		vec.push_back("2 ========== ==========");
		vec.push_back("3 ========== ==========");

		buffer.push(std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.cend()));
	}
	{
		mk::ring_buffer_spsc_t<std::string, 8> buffer;
		buffer.push("1 ========== ==========");
		buffer.push("2 ========== ==========");
		buffer.push("3 ========== ==========");

		auto buffer_2 = buffer;
	}
	{
		mk::ring_buffer_spsc_t<std::string, 8> buffer;
		buffer.push("1 ========== ==========");
		buffer.push("2 ========== ==========");
		buffer.push("3 ========== ==========");

		auto buffer_2 = std::move(buffer);
	}
}(), 0);
