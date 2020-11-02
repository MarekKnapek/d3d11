#pragma once


#include "ring_buffer_spsc.h"

#include "mk_bit_utils.h"

#include <cassert>


namespace mk
{
	namespace detail
	{
		template<typename t>
		class move_iterator_t
		{
		public:
			move_iterator_t(t const& v) : m_v(v) {}
			move_iterator_t(t&& v) : m_v(mk::move(v)) {}
			void operator++() { ++m_v; }
			decltype(auto) operator*() { return mk::move(*m_v); }
		public:
			t m_v;
		};
		template<typename t> inline bool operator!=(move_iterator_t<t> const& a, move_iterator_t<t> const& b) { return a.m_v != b.m_v; }
		template<typename t, typename u> inline bool operator!=(move_iterator_t<t> const& a, u&& b) { return a.m_v != b; }
		template<typename t>
		inline move_iterator_t<t> make_move_iterator(t const& v)
		{
			return move_iterator_t<t>{v};
		}
		template<typename t>
		inline move_iterator_t<t> make_move_iterator(t&& v)
		{
			return move_iterator_t<t>{mk::move(v)};
		}
	}
}


template<typename t, int capacity_v>
mk::ring_buffer_spsc_t<t, capacity_v>::ring_buffer_spsc_t() noexcept :
	m_read_idx(),
	m_write_idx(),
	m_arr()
{
	static_assert(s_capacity_v != 0 && s_capacity_v > 0 && mk::is_power_of_two(s_capacity_v));
}

template<typename t, int capacity_v>
mk::ring_buffer_spsc_t<t, capacity_v>::ring_buffer_spsc_t(ring_buffer_spsc_t const& other) :
	ring_buffer_spsc_t()
{
	auto const first_part = other.first_continuous_part();
	push(begin(first_part), end(first_part));
	auto const second_part = other.second_continuous_part();
	push(begin(second_part), end(second_part));
}

template<typename t, int capacity_v>
mk::ring_buffer_spsc_t<t, capacity_v>::ring_buffer_spsc_t(ring_buffer_spsc_t&& other) noexcept :
	ring_buffer_spsc_t()
{
	auto first_part = other.first_continuous_part();
	push(mk::detail::make_move_iterator(begin(first_part)), mk::detail::make_move_iterator(end(first_part)));
	auto second_part = other.second_continuous_part();
	push(mk::detail::make_move_iterator(begin(second_part)), mk::detail::make_move_iterator(end(second_part)));
	other.clear();
}

template<typename t, int capacity_v>
mk::ring_buffer_spsc_t<t, capacity_v>& mk::ring_buffer_spsc_t<t, capacity_v>::operator=(ring_buffer_spsc_t const& other)
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
mk::ring_buffer_spsc_t<t, capacity_v>& mk::ring_buffer_spsc_t<t, capacity_v>::operator=(ring_buffer_spsc_t&& other) noexcept
{
	swap(other);
	return *this;
}

template<typename t, int capacity_v>
void mk::ring_buffer_spsc_t<t, capacity_v>::swap(ring_buffer_spsc_t& other) noexcept
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
mk::ring_buffer_spsc_t<t, capacity_v>::~ring_buffer_spsc_t()
{
	clear();
}

template<typename t, int capacity_v>
bool mk::ring_buffer_spsc_t<t, capacity_v>::is_empty() const
{
	return size() == 0;
}

template<typename t, int capacity_v>
bool mk::ring_buffer_spsc_t<t, capacity_v>::is_full() const
{
	return size() == s_capacity_v;
}

template<typename t, int capacity_v>
int mk::ring_buffer_spsc_t<t, capacity_v>::size() const
{
	return m_write_idx.load(std::memory_order_acquire) - m_read_idx.load(std::memory_order_acquire);
}

template<typename t, int capacity_v>
int mk::ring_buffer_spsc_t<t, capacity_v>::capacity() const
{
	return s_capacity_v;
}

template<typename t, int capacity_v>
void mk::ring_buffer_spsc_t<t, capacity_v>::clear()
{
	pop(size());
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_spsc_t<t, capacity_v>::front() const
{
	assert(!is_empty());
	t const& ref = operator[](0);
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_spsc_t<t, capacity_v>::front()
{
	assert(!is_empty());
	t& ref = operator[](0);
	return ref;
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_spsc_t<t, capacity_v>::back() const
{
	assert(!is_empty());
	t const& ref = operator[](size() - 1);
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_spsc_t<t, capacity_v>::back()
{
	assert(!is_empty());
	t& ref = operator[](size() - 1);
	return ref;
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_spsc_t<t, capacity_v>::operator[](int const& idx) const
{
	assert(idx < size());
	t const& ref = internal_get(m_read_idx.load(std::memory_order_acquire) + idx);
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_spsc_t<t, capacity_v>::operator[](int const& idx)
{
	assert(idx < size());
	t& ref = internal_get(m_read_idx.load(std::memory_order_acquire) + idx);
	return ref;
}

template<typename t, int capacity_v>
mk::ring_buffer_spsc_const_continuous_part_t<t> mk::ring_buffer_spsc_t<t, capacity_v>::first_continuous_part() const
{
	unsigned const read_idx = m_read_idx.load(std::memory_order_acquire);
	bool const is_split = (read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx.load(std::memory_order_acquire) &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t const* const begin = &internal_get(read_idx + 0);
	t const* const end = &internal_get(read_idx + size());
	t const* const array_end = reinterpret_cast<t const*>(m_arr[0].m_buff) + s_capacity_v;
	t const* const first_end = is_split ? array_end : end;
	return mk::ring_buffer_spsc_const_continuous_part_t<t>{begin, first_end};
}

template<typename t, int capacity_v>
mk::ring_buffer_spsc_continuous_part_t<t> mk::ring_buffer_spsc_t<t, capacity_v>::first_continuous_part()
{
	unsigned const read_idx = m_read_idx.load(std::memory_order_acquire);
	bool const is_split = (read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx.load(std::memory_order_acquire) &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t* const begin = &internal_get(read_idx + 0);
	t* const end = &internal_get(read_idx + size());
	t* const array_end = reinterpret_cast<t*>(m_arr[0].m_buff) + s_capacity_v;
	t* const first_end = is_split ? array_end : end;
	return mk::ring_buffer_spsc_continuous_part_t<t>{begin, first_end};
}

template<typename t, int capacity_v>
mk::ring_buffer_spsc_const_continuous_part_t<t> mk::ring_buffer_spsc_t<t, capacity_v>::second_continuous_part() const
{
	unsigned const read_idx = m_read_idx.load(std::memory_order_acquire);
	bool const is_split = (read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx.load(std::memory_order_acquire) &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t const* const end = &internal_get(read_idx + size());
	t const* const array_begin = reinterpret_cast<t const*>(m_arr[0].m_buff) + 0;
	t const* const second_begin = is_split ? array_begin : end;
	return mk::ring_buffer_spsc_const_continuous_part_t<t>{second_begin, end};
}

template<typename t, int capacity_v>
mk::ring_buffer_spsc_continuous_part_t<t> mk::ring_buffer_spsc_t<t, capacity_v>::second_continuous_part()
{
	unsigned const read_idx = m_read_idx.load(std::memory_order_acquire);
	bool const is_split = (read_idx &~ (static_cast<unsigned>(s_capacity_v) - 1)) != (m_write_idx.load(std::memory_order_acquire) &~ (static_cast<unsigned>(s_capacity_v) - 1));
	t* const end = &internal_get(read_idx + size());
	t* const array_begin = reinterpret_cast<t*>(m_arr[0].m_buff) + 0;
	t* const second_begin = is_split ? array_begin : end;
	return mk::ring_buffer_spsc_continuous_part_t<t>{second_begin, end};
}


template<typename t, int capacity_v>
void mk::ring_buffer_spsc_t<t, capacity_v>::resize(int const& count)
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
t& mk::ring_buffer_spsc_t<t, capacity_v>::push()
{
	assert(!is_full());
	return push(t{});
}

template<typename t, int capacity_v>
template<typename u>
t& mk::ring_buffer_spsc_t<t, capacity_v>::push(u&& val)
{
	assert(!is_full());
	unsigned const write_idx = m_write_idx.load(std::memory_order_acquire);
	t& ref = internal_get(write_idx);
	new(static_cast<void*>(&ref)) t{mk::forward<u>(val)};
	m_write_idx.store(write_idx + 1, std::memory_order_release);
	return ref;
}

template<typename t, int capacity_v>
template<typename u, typename v>
void mk::ring_buffer_spsc_t<t, capacity_v>::push(u begin, v const& end)
{
	int count = 0;
	unsigned const write_idx = m_write_idx.load(std::memory_order_acquire);
	while(begin != end)
	{
		assert(!is_full());
		t& ref = internal_get(write_idx + count);
		new(static_cast<void*>(&ref)) t{*begin};
		++begin;
		++count;
	}
	m_write_idx.store(write_idx + count, std::memory_order_release);
}

template<typename t, int capacity_v>
void mk::ring_buffer_spsc_t<t, capacity_v>::pop()
{
	return pop(1);
}

template<typename t, int capacity_v>
void mk::ring_buffer_spsc_t<t, capacity_v>::pop(int const& count)
{
	assert(count <= size());
	unsigned const read_idx = m_read_idx.load(std::memory_order_acquire);
	for(int i = 0; i != count; ++i)
	{
		t& ref = internal_get(read_idx + i);
		ref.~t();
	}
	m_read_idx.store(read_idx + count, std::memory_order_release);
}

template<typename t, int capacity_v>
t const& mk::ring_buffer_spsc_t<t, capacity_v>::internal_get(unsigned const& idx) const
{
	t const& ref = *static_cast<t const*>(static_cast<void const*>(m_arr[idx & (s_capacity_v - 1)].m_buff));
	return ref;
}

template<typename t, int capacity_v>
t& mk::ring_buffer_spsc_t<t, capacity_v>::internal_get(unsigned const& idx)
{
	t& ref = *static_cast<t*>(static_cast<void*>(m_arr[idx & (s_capacity_v - 1)].m_buff));
	return ref;
}
