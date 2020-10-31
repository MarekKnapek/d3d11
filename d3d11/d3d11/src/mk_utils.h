#pragma once


namespace mk
{

	namespace detail
	{
		template<int size_v, int align_v>
		struct aligned_storage_helper_t
		{
			alignas(align_v) unsigned char m_buff[size_v];
		};
	}
	template<typename t>
	using aligned_storage_t = detail::aligned_storage_helper_t<sizeof(t), alignof(t)>;

	namespace detail
	{
		template<typename t>
		struct remove_reference_helper_t
		{
			typedef t type;
		};
		template<typename t>
		struct remove_reference_helper_t<t&>
		{
			typedef t type;
		};
	}
	template<typename t>
	using remove_reference_t = typename detail::remove_reference_helper_t<t>::type;

	template<typename t>
	t const& min(t const& a, t const& b);
	template<typename t>
	t const& max(t const& a, t const& b);

	template<typename t>
	mk::remove_reference_t<t>&& move(t&& val);
	template<typename t>
	t&& forward(mk::remove_reference_t<t>& val);
	template<typename t>
	t&& forward(mk::remove_reference_t<t>&& val);

	template<typename t>
	void swap(t& a, t&b) noexcept;

}


#include "mk_utils.inl"
