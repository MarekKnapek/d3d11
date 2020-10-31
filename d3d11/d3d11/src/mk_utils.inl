#pragma once


#include "mk_utils.h"


template<typename t>
inline t const& mk::min(t const& a, t const& b)
{
	return b < a ? b : a;
}

template<typename t>
inline t const& mk::max(t const& a, t const& b)
{
	return b < a ? a : b;
}


template<typename t>
inline mk::remove_reference_t<t>&& mk::move(t&& val)
{
	return static_cast<mk::remove_reference_t<t>&&>(val);
}

template<typename t>
inline t&& mk::forward(mk::remove_reference_t<t>& val)
{
	return static_cast<t&&>(val);
}

template<typename t>
inline t&& mk::forward(mk::remove_reference_t<t>&& val)
{
	return static_cast<t&&>(val);
}


template<typename t>
inline void mk::swap(t& a, t&b) noexcept
{
	t tmp = mk::move(a);
	a = mk::move(b);
	b = mk::move(tmp);
}
