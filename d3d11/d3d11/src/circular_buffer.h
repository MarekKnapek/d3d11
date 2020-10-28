#pragma once


namespace mk
{


	template<typename t, unsigned capacity>
	class circular_buffer_t
	{
	public:
		//typename t type_t; // visualiser freaks out with this line
		using type_t = t; // use this line instead to be gentle to the visualiser
		static constexpr unsigned s_capacity_v = capacity;
	public:
		circular_buffer_t();
		~circular_buffer_t();
	public:
		bool is_empty() const;
		bool is_full() const;
		unsigned size() const;
		t const& read() const;
		t& read();
		void pop();
		t& push(t const& val);
		template<typename u>
		t& push(u&& val);
	private:
		unsigned m_read_idx;
		unsigned m_write_idx;
		t m_arr[capacity];
	};


}


#include "circular_buffer.inl"
