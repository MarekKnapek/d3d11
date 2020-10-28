#pragma once


#include "circular_buffer.h"


template<typename t, unsigned capacity>
mk::circular_buffer_t<t, capacity>::circular_buffer_t() :
	m_read_idx(),
	m_write_idx(),
	m_arr()
{
	static_assert(capacity != 0 && is_power_of_two(capacity));
}

template<typename t, unsigned capacity>
mk::circular_buffer_t<t, capacity>::~circular_buffer_t()
{
}

template<typename t, unsigned capacity>
bool mk::circular_buffer_t<t, capacity>::is_empty() const
{
	return m_read_idx == m_write_idx;
}

template<typename t, unsigned capacity>
bool mk::circular_buffer_t<t, capacity>::is_full() const
{
	return m_write_idx == m_read_idx + capacity;
}

template<typename t, unsigned capacity>
unsigned mk::circular_buffer_t<t, capacity>::size() const
{
	return m_write_idx - m_read_idx;
}

template<typename t, unsigned capacity>
t const& mk::circular_buffer_t<t, capacity>::read() const
{
	assert(!is_empty());
	return m_arr[m_read_idx & (capacity - 1)];
}

template<typename t, unsigned capacity>
t& mk::circular_buffer_t<t, capacity>::read()
{
	assert(!is_empty());
	return m_arr[m_read_idx & (capacity - 1)];
}

template<typename t, unsigned capacity>
void mk::circular_buffer_t<t, capacity>::pop()
{
	assert(!is_empty());
	++m_read_idx;
}

template<typename t, unsigned capacity>
t& mk::circular_buffer_t<t, capacity>::push(t const& val)
{
	assert(!is_full());
	t& ref = m_arr[m_write_idx & (capacity - 1)];
	ref = val;
	++m_write_idx;
	return ref;
}

template<typename t, unsigned capacity>
template<typename u>
t& mk::circular_buffer_t<t, capacity>::push(u&& val)
{
	assert(!is_full());
	t& ref = m_arr[m_write_idx & (capacity - 1)];
	ref = std::forward<u>(val);
	++m_write_idx;
	return ref;
}
