#include "vlp16.h"

#include <cassert>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <numbers>
#include <cmath>


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

void mk::vlp16::convert_to_xyza(single_mode_packet_t const& packet, accept_point_fn_t const& accept_point_fn, void* const& ctx)
{
	static constexpr double const s_vertical_angle_cos[] =
	{
		0x1.ee8dd4748bf15p-1 /* cos( -15 deg ) */,
		0x1.ffec097f5af8ap-1 /* cos(  +1 deg ) */,
		0x1.f2e0a214e870fp-1 /* cos( -13 deg ) */,
		0x1.ff4c5ed12e61dp-1 /* cos(  +3 deg ) */,
		0x1.f697d6938b6c2p-1 /* cos( -11 deg ) */,
		0x1.fe0d3b41815a2p-1 /* cos(  +5 deg ) */,
		0x1.f9b24942fe45cp-1 /* cos(  -9 deg ) */,
		0x1.fc2f025a23e8bp-1 /* cos(  +7 deg ) */,
		0x1.fc2f025a23e8bp-1 /* cos(  -7 deg ) */,
		0x1.f9b24942fe45cp-1 /* cos(  +9 deg ) */,
		0x1.fe0d3b41815a2p-1 /* cos(  -5 deg ) */,
		0x1.f697d6938b6c2p-1 /* cos( +11 deg ) */,
		0x1.ff4c5ed12e61dp-1 /* cos(  -3 deg ) */,
		0x1.f2e0a214e870fp-1 /* cos( +13 deg ) */,
		0x1.ffec097f5af8ap-1 /* cos(  -1 deg ) */,
		0x1.ee8dd4748bf15p-1 /* cos( +15 deg ) */,
	};
	static constexpr double const s_vertical_angle_sin[] =
	{
		-0x1.0907dc1930690p-2 /* sin( -15 deg ) */,
		+0x1.1df0b2b89dd1ep-6 /* sin(  +1 deg ) */,
		-0x1.ccb3236cdc675p-3 /* sin( -13 deg ) */,
		+0x1.acbc748efc90ep-5 /* sin(  +3 deg ) */,
		-0x1.86c6ddd76624fp-3 /* sin( -11 deg ) */,
		+0x1.64fd6b8c28102p-4 /* sin(  +5 deg ) */,
		-0x1.4060b67a85375p-3 /* sin(  -9 deg ) */,
		+0x1.f32d44c4f62d3p-4 /* sin(  +7 deg ) */,
		-0x1.f32d44c4f62d3p-4 /* sin(  -7 deg ) */,
		+0x1.4060b67a85375p-3 /* sin(  +9 deg ) */,
		-0x1.64fd6b8c28102p-4 /* sin(  -5 deg ) */,
		+0x1.86c6ddd76624fp-3 /* sin( +11 deg ) */,
		-0x1.acbc748efc90ep-5 /* sin(  -3 deg ) */,
		+0x1.ccb3236cdc675p-3 /* sin( +13 deg ) */,
		-0x1.1df0b2b89dd1ep-6 /* sin(  -1 deg ) */,
		+0x1.0907dc1930690p-2 /* sin( +15 deg ) */,
	};
	static constexpr double const s_vertical_correction_m[] =
	{
		+7.4 / 1'000.0,
		-0.9 / 1'000.0,
		+6.5 / 1'000.0,
		-1.8 / 1'000.0,
		+5.5 / 1'000.0,
		-2.7 / 1'000.0,
		+4.6 / 1'000.0,
		-3.7 / 1'000.0,
		+3.7 / 1'000.0,
		-4.6 / 1'000.0,
		+2.7 / 1'000.0,
		-5.5 / 1'000.0,
		+1.8 / 1'000.0,
		-6.5 / 1'000.0,
		+0.9 / 1'000.0,
		-7.4 / 1'000.0,
	};
	static constexpr double const s_firing_sequence_len_us = 55.296; // us, including recharge time
	static constexpr double const s_firing_delay_us = 2.304; // us

	static constexpr auto const deg_to_rad = [](double const& deg) -> double { return deg * (std::numbers::pi_v<double> / 180.0); };
	static constexpr auto const rad_to_deg = [](double const& rad) -> double { return rad * (180.0 / std::numbers::pi_v<double>); };


	for(int data_block_idx = 0; data_block_idx != s_data_blocks_count; ++data_block_idx)
	{
		auto const& data_block = packet.m_data_blocks[data_block_idx];
		bool const last_block = data_block_idx == s_data_blocks_count - 1;
		std::uint16_t const azimuth_a_uint = !last_block ? packet.m_data_blocks[data_block_idx + 0].m_azimuth : packet.m_data_blocks[data_block_idx - 1].m_azimuth;
		std::uint16_t const azimuth_b_uint = !last_block ? packet.m_data_blocks[data_block_idx + 1].m_azimuth : packet.m_data_blocks[data_block_idx + 0].m_azimuth;
		bool const azimuth_wrap = azimuth_b_uint <= azimuth_a_uint;
		std::uint16_t const azimuth_diff_uint = !azimuth_wrap ? (azimuth_b_uint - azimuth_a_uint) : (360 * 100 - azimuth_a_uint + azimuth_b_uint);
		double const azimuth_diff_deg = static_cast<double>(azimuth_diff_uint) / 100.0;
		double const azimuth_deg = static_cast<double>(data_block.m_azimuth) / 100.0;
		for(int firing_sequence_idx = 0; firing_sequence_idx != s_firing_sequences_count; ++firing_sequence_idx)
		{
			auto const& firing_sequence = data_block.m_firing_sequence[firing_sequence_idx];
			for(int channel_data_idx = 0; channel_data_idx != s_channels_count; ++channel_data_idx)
			{
				double const r = static_cast<double>(firing_sequence.m_distance[channel_data_idx] * 2) / 1'000.0; // distance in 2 milimeters -> distance in meters
				double const azimuth_correction_deg = azimuth_diff_deg * (((firing_sequence_idx == 0 ? 0.0 : s_firing_sequence_len_us) + channel_data_idx * s_firing_delay_us) / (static_cast<double>(s_firing_sequences_count) * s_firing_sequence_len_us));
				double const azimuth_precise_tmp_deg = azimuth_deg + azimuth_correction_deg;
				double const azimuth_precise_deg = azimuth_precise_tmp_deg >= 360.0 ? (azimuth_precise_tmp_deg - 360.0) : azimuth_precise_tmp_deg; // precision azimuth in degrees
				double const azimuth_precise_rad = deg_to_rad(azimuth_precise_deg); // precision azimuth in radians
				double const tmp = r * s_vertical_angle_cos[channel_data_idx];
				double const x = tmp * std::sin(azimuth_precise_rad); // x in meters
				double const y = tmp * std::cos(azimuth_precise_rad); // y in meters
				double const z = r * s_vertical_angle_sin[channel_data_idx] + s_vertical_correction_m[channel_data_idx]; // z in meters
				accept_point_fn(x, y, z, azimuth_precise_deg, ctx);
			}
		}
	}
}
