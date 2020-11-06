#pragma once


namespace mk
{


	constexpr bool is_power_of_two(unsigned const& n) noexcept
	{
		return n == 0 || (n & (n - 1)) == 0;
	}

	constexpr unsigned equal_or_next_power_of_two(unsigned const& n) noexcept
	{
		unsigned x = n;
		x--; 
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x++;
		return x;
	}


}
