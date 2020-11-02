#pragma once


#include "ring_buffer.h"

#include "mk_bit_utils.h"

#include <cassert>


template<typename t, int capacity_v>
mk::ring_buffer_t<t, capacity_v>::ring_buffer_t() noexcept :
	m_read_idx(),
	m_write_idx(),
	m_arr()
{
	static_assert(s_capacity_v != 0 && s_capacity_v > 0 && mk::is_power_of_two(s_capacity_v));
}

template<typename t, int capacity_v>
mk::ring_buffer_t<t, capacity_v>::ring_buffer_t(ring_buffer_t const& other) :
	ring_buffer_t()
{
	int const n = other.size();
	for(int i = 0; i != n; ++i)
	{
		push(other[i]);
	}
}

template<typename t, int capacity_v>
mk::ring_buffer_t<t, capacity_v>::ring_buffer_t(ring_buffer_t&& other) noexcept :
	ring_buffer_t()
{
	swap(other);
}

template<typename t, int capacity_v>
mk::ring_buffer_t<t, capacity_v>& mk::ring_buffer_t<t, capacity_v>::operator=(ring_buffer_t const& other)
{
	int const n1 = size();
	int const n2 = other.size();
	int const n = mk::min(n1, n2);
	for(int i = 0; i != n; ++i)
	{
		operator[](i) = other[i];
	}
	for(int i = n; i < n2; ++i)
	{
		push(other[i]);
	}
	for(int i = 0; i < n1 - n2; ++i)
	{
		pop();
	}
	return *this;
}

template<typename t, int capacity_v>
mk::ring_buffer_t<t, capacity_v>& mk::ring_buffer_t<t, capacity_v>::operator=(ring_buffer_t&& other) noexcept
{
	swap(other);
	return *this;
}

template<typename t, int capacity_v>
void mk::ring_buffer_t<t, capacity_v>::swap(ring_buffer_t& other) noexcept
{
	int const n1 = size();
	int const n2 = other.size();
	int const n = mk::min(n1, n2);
	for(int i = 0; i != n; ++i)
	{
		using mk::swap;
		swap(operator[](i), other[i]);
	}
	for(int i = n; i < n2; ++i)
	{
		push(mk::move(other[i]));
	}
	for(int i = 0; i < n1 - n2; ++i)
	{
		pop();
	}
}

template<typename t, int capacity_v>
mk::ring_buffer_t<t, capacity_v>::~ring_buffer_t()
{
	clear();
}

template<typename t, int capacity_v>
bool mk::ring_buffer_t<t, capacity_v>::is_empty() const
{
	return size() == 0;
}

template<typename t, int capacity_v>
bool mk::ring_buffer_t<t, capacity_v>::is_full() const
{
	return size() == s_capacity_v;
}

template<typename t, int capacity_v>
int mk::ring_buffer_t<t, capacity_v>::size() const
{
	return m_write_idx - m_read_idx;
}

template<typename t, int capacity_v>
int mk::ring_buffer_t<t, capacity_v>::capacity() const
{
	return s_capacity_v;
}

template<typename t, int capacity_v>
void mk::ring_buffer_t<t, capacity_v>::clear()
{
	pop(size());
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_t<t, capacity_v>::front() const
{
	assert(!is_empty());
	t const& ref = operator[](0);
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_t<t, capacity_v>::front()
{
	assert(!is_empty());
	t& ref = operator[](0);
	return ref;
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_t<t, capacity_v>::back() const
{
	assert(!is_empty());
	t const& ref = operator[](size() - 1);
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_t<t, capacity_v>::back()
{
	assert(!is_empty());
	t& ref = operator[](size() - 1);
	return ref;
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_t<t, capacity_v>::operator[](int const& idx) const
{
	assert(idx < size());
	t const& ref = internal_get(m_read_idx + idx);
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_t<t, capacity_v>::operator[](int const& idx)
{
	assert(idx < size());
	t& ref = internal_get(m_read_idx + idx);
	return ref;
}

template<typename t, int capacity_v>
mk::ring_buffer_const_continuous_part_t<t> mk::ring_buffer_t<t, capacity_v>::first_continuous_part() const
{
	bool const is_split = (m_read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t const* const begin = &internal_get(m_read_idx + 0);
	t const* const end = &internal_get(m_read_idx + size());
	t const* const array_end = reinterpret_cast<t const*>(m_arr[0].m_buff) + s_capacity_v;
	t const* const first_end = is_split ? array_end : end;
	return mk::ring_buffer_const_continuous_part_t<t>{begin, first_end};
}

template<typename t, int capacity_v>
mk::ring_buffer_continuous_part_t<t> mk::ring_buffer_t<t, capacity_v>::first_continuous_part()
{
	bool const is_split = (m_read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t* const begin = &internal_get(m_read_idx + 0);
	t* const end = &internal_get(m_read_idx + size());
	t* const array_end = reinterpret_cast<t*>(m_arr[0].m_buff) + s_capacity_v;
	t* const first_end = is_split ? array_end : end;
	return mk::ring_buffer_continuous_part_t<t>{begin, first_end};
}

template<typename t, int capacity_v>
mk::ring_buffer_const_continuous_part_t<t> mk::ring_buffer_t<t, capacity_v>::second_continuous_part() const
{
	bool const is_split = (m_read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t const* const end = &internal_get(m_read_idx + size());
	t const* const array_begin = reinterpret_cast<t const*>(m_arr[0].m_buff) + 0;
	t const* const second_begin = is_split ? array_begin : end;
	return mk::ring_buffer_const_continuous_part_t<t>{second_begin, end};
}

template<typename t, int capacity_v>
mk::ring_buffer_continuous_part_t<t> mk::ring_buffer_t<t, capacity_v>::second_continuous_part()
{
	bool const is_split = (m_read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t* const end = &internal_get(m_read_idx + size());
	t* const array_begin = reinterpret_cast<t*>(m_arr[0].m_buff) + 0;
	t* const second_begin = is_split ? array_begin : end;
	return mk::ring_buffer_continuous_part_t<t>{second_begin, end};
}


template<typename t, int capacity_v>
void mk::ring_buffer_t<t, capacity_v>::resize(int const& count)
{
	assert(count <= s_capacity_v);
	int const n = size();
	for(int i = 0; i < count - n; ++i)
	{
		push();
	}
	for(int i = 0; i < n - count; ++i)
	{
		pop();
	}
}

template<typename t, int capacity_v>
t& mk::ring_buffer_t<t, capacity_v>::push()
{
	assert(!is_full());
	return push(t{});
}

template<typename t, int capacity_v>
template<typename u>
t& mk::ring_buffer_t<t, capacity_v>::push(u&& val)
{
	assert(!is_full());
	t& ref = internal_get(m_write_idx);
	new(static_cast<void*>(&ref)) t{mk::forward<u>(val)};
	++m_write_idx;
	return ref;
}

template<typename t, int capacity_v>
void mk::ring_buffer_t<t, capacity_v>::pop()
{
	return pop(1);
}

template<typename t, int capacity_v>
void mk::ring_buffer_t<t, capacity_v>::pop(int const& count)
{
	assert(count <= size());
	for(int i = 0; i != count; ++i)
	{
		t& ref = internal_get(m_read_idx + i);
		ref.~t();
	}
	m_read_idx += count;
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_t<t, capacity_v>::internal_get(unsigned const& idx) const
{
	t const& ref = *static_cast<t const*>(static_cast<void const*>(m_arr[idx & (s_capacity_v - 1)].m_buff));
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_t<t, capacity_v>::internal_get(unsigned const& idx)
{
	t& ref = *static_cast<t*>(static_cast<void*>(m_arr[idx & (s_capacity_v - 1)].m_buff));
	return ref;
}
