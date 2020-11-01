#pragma once


#include "mk_utils.h"


namespace mk
{


	template<typename t>
	struct ring_buffer_const_continuous_part_t
	{
		t const* m_begin;
		t const* m_end;
	};
	template<typename t> inline t const* begin(ring_buffer_const_continuous_part_t<t> const& val) { return val.m_begin; }
	template<typename t> inline t const* end(ring_buffer_const_continuous_part_t<t> const& val) { return val.m_end; }

	template<typename t>
	struct ring_buffer_continuous_part_t
	{
		t* m_begin;
		t* m_end;
	};
	template<typename t> inline t const* begin(ring_buffer_continuous_part_t<t> const& val) { return val.m_begin; }
	template<typename t> inline t* begin(ring_buffer_continuous_part_t<t>& val) { return val.m_begin; }
	template<typename t> inline t const* end(ring_buffer_continuous_part_t<t> const& val) { return val.m_end; }
	template<typename t> inline t* end(ring_buffer_continuous_part_t<t>& val) { return val.m_end; }


	template<typename t, int capacity_v>
	class ring_buffer_t
	{
	public:
		//typename t type_t; // visualiser freaks out with this line
		using type_t = t; // use this line instead to be gentle to the visualiser
		static constexpr int s_capacity_v = capacity_v;
	public:
		ring_buffer_t() noexcept;
		ring_buffer_t(ring_buffer_t const& other);
		ring_buffer_t(ring_buffer_t&& other) noexcept;
		ring_buffer_t& operator=(ring_buffer_t const& other);
		ring_buffer_t& operator=(ring_buffer_t&& other) noexcept;
		void swap(ring_buffer_t& other) noexcept;
		~ring_buffer_t();
	public:
		bool is_empty() const;
		bool is_full() const;
		int size() const;
		int capacity() const;
		void clear();
		t const& front() const;
		t& front();
		t const& back() const;
		t& back();
		t const& operator[](int const& idx) const;
		t& operator[](int const& idx);
		mk::ring_buffer_const_continuous_part_t<t> first_continuous_part() const;
		mk::ring_buffer_continuous_part_t<t> first_continuous_part();
		mk::ring_buffer_const_continuous_part_t<t> second_continuous_part() const;
		mk::ring_buffer_continuous_part_t<t> second_continuous_part();
		void resize(int const& count);
		t& push();
		template<typename u>
		t& push(u&& val);
		void pop();
		void pop(int const& count);
	private:
		t const& internal_get(unsigned const& idx) const;
		t& internal_get(unsigned const& idx);
	private:
		unsigned m_read_idx;
		unsigned m_write_idx;
		mk::aligned_storage_t<t> m_arr[s_capacity_v];
	};

	template<typename t, int capacity_v> inline void swap(ring_buffer_t<t, capacity_v>& a, ring_buffer_t<t, capacity_v>& b) noexcept { a.swap(b); }


}


#include "ring_buffer.inl"
