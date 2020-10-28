#include "vlp16.h"

#include <cassert>
#include <cstring>
#include <iterator>
#include <algorithm>


mk::vlp16::single_mode_packet_t mk::vlp16::raw_data_to_single_mode_packet(void const* const& data, int const& len)
{
	static constexpr auto const s_read = []<typename t>(void const* const& data, int& idx) -> t
	{
		t ret;
		std::memcpy(&ret, static_cast<unsigned char const*>(data) + idx, sizeof(ret));
		idx += sizeof(ret);
		return ret;
	};

	assert(len == s_packet_size); (void)len;
	single_mode_packet_t packet;
	int idx = 0;
	for(int i_data_block = 0; i_data_block != s_data_blocks_count; ++i_data_block)
	{
		packet.m_data_blocks[i_data_block].m_flag = s_read.operator()<std::uint16_t>(data, idx);
		packet.m_data_blocks[i_data_block].m_azimuth = s_read.operator()<std::uint16_t>(data, idx);
		for(int i_firing_sequence = 0; i_firing_sequence != s_firing_sequences_count; ++i_firing_sequence)
		{
			for(int i_channel = 0; i_channel != s_channels_count; ++i_channel)
			{
				packet.m_data_blocks[i_data_block].m_firing_sequence[i_firing_sequence].m_distance[i_channel] = s_read.operator()<std::uint16_t>(data, idx);
				packet.m_data_blocks[i_data_block].m_firing_sequence[i_firing_sequence].m_reflexivity[i_channel] = s_read.operator()<std::uint8_t>(data, idx);
			}
		}
	}
	packet.m_timestamp = s_read.operator()<std::uint32_t>(data, idx);
	packet.m_factory.m_return_mode = s_read.operator()<std::uint8_t>(data, idx);
	packet.m_factory.m_product_id = s_read.operator()<std::uint8_t>(data, idx);
	assert(idx == s_packet_size);
	return packet;
}

bool mk::vlp16::verify_single_mode_packet(single_mode_packet_t const& packet)
{
	static constexpr std::uint8_t const s_id = 0x22;
	static constexpr std::uint8_t const s_strongest_mode = 0x37;
	static constexpr std::uint8_t const s_last_mode = 0x38;
	static constexpr std::uint32_t const s_max_timestamp = 3'600'000'000ull;
	static constexpr std::uint16_t const s_flag = 0xEEFF;
	static constexpr std::uint16_t const s_max_azimuth = 36'000;

	static constexpr auto const s_verify_data_block = [](data_block_t const& data_block) -> bool
	{
		return data_block.m_flag == s_flag && data_block.m_azimuth < s_max_azimuth;
	};

	using std::cbegin;
	using std::cend;

	bool ok = true;
	ok = ok && (packet.m_factory.m_product_id == s_id);
	ok = ok && (packet.m_factory.m_return_mode == s_strongest_mode || packet.m_factory.m_return_mode == s_last_mode);
	ok = ok && (packet.m_timestamp < s_max_timestamp);
	ok = ok && (std::all_of(cbegin(packet.m_data_blocks), cend(packet.m_data_blocks), s_verify_data_block));
	return ok;
}
