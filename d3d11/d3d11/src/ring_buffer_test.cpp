#include "ring_buffer.h"

#include <string>

#include <stdio.h>


void print(mk::ring_buffer_t<std::string, 8> const& buff)
{
	for(auto const& e : buff.first_continuous_part())
	{
		printf("%s ", e.c_str());
	}
	for(auto const& e : buff.second_continuous_part())
	{
		printf("%s ", e.c_str());
	}
	printf("\n");
}

void test()
{
	mk::ring_buffer_t<std::string, 8> buff;
	for(int j = 0; j != 10; ++j)
	{
		buff.push();
		buff.pop();
		for(int i = 0; i != 10; ++i)
		{
			buff.push("1 ========== ==========");
			buff.push("2 ========== ==========");
			buff.push("3 ========== ==========");
			buff.push("4 ========== ==========");
			buff.push("5 ========== ==========");
			buff.push("6 ========== ==========");
			buff.push("7 ========== ==========");
			print(buff);
			buff.pop();
			buff.pop();
			buff.pop();
			buff.pop();
			buff.pop();
			buff.pop();
			buff.pop();
		}
	}

	int volatile a;
	a = 0;
}


static int test_int = (test(), 0);
