#include "ouster64.h"

#include "mk_utils.h"

#include <cassert>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <numbers>
#include <cmath>

#include <gcem.hpp>


mk::ouster64::packet_t mk::ouster64::raw_data_to_packet(void const* const& data, int const& len)
{
	static constexpr auto const s_read = []<typename t>(void const* const& data, int& idx) -> t
	{
		t ret;
		std::memcpy(&ret, static_cast<unsigned char const*>(data) + idx, sizeof(ret));
		idx += sizeof(ret);
		return ret;
	};

	assert(len == s_bytes_per_packet); (void)len;
	packet_t packet;
	int idx = 0;
	for(int i_azimuth_block = 0; i_azimuth_block != s_azimuth_blocks_per_packet; ++i_azimuth_block)
	{
		packet.m_azimuth_blocks[i_azimuth_block].m_timestamp = s_read.operator()<std::uint64_t>(data, idx);
		packet.m_azimuth_blocks[i_azimuth_block].m_measurement_id = s_read.operator()<std::uint16_t>(data, idx);
		packet.m_azimuth_blocks[i_azimuth_block].m_frame_id = s_read.operator()<std::uint16_t>(data, idx);
		packet.m_azimuth_blocks[i_azimuth_block].m_encoder_count = s_read.operator()<std::uint32_t>(data, idx);
		for(int i_data_block = 0; i_data_block != s_data_blocks_per_azimuth_block; ++i_data_block)
		{
			std::uint32_t const range = s_read.operator()<std::uint32_t>(data, idx);
			packet.m_azimuth_blocks[i_azimuth_block].m_data_blocks[i_data_block].m_range = range & ((1ull << 20) - 1);
			packet.m_azimuth_blocks[i_azimuth_block].m_data_blocks[i_data_block].m_reflectivity = s_read.operator()<std::uint16_t>(data, idx);
			packet.m_azimuth_blocks[i_azimuth_block].m_data_blocks[i_data_block].m_signal_photons = s_read.operator()<std::uint16_t>(data, idx);
			packet.m_azimuth_blocks[i_azimuth_block].m_data_blocks[i_data_block].m_noise_photons = s_read.operator()<std::uint16_t>(data, idx);
			std::uint16_t const unused = s_read.operator()<std::uint16_t>(data, idx); (void)unused;
		}
		packet.m_azimuth_blocks[i_azimuth_block].m_status = s_read.operator()<std::uint32_t>(data, idx);
	}
	assert(idx == s_bytes_per_packet);
	return packet;
}

bool mk::ouster64::verify_packet(packet_t const& packet)
{
	static constexpr std::uint16_t const s_measurement_id_max_1 = 512;
	static constexpr std::uint16_t const s_measurement_id_max_2 = 1024;
	static constexpr std::uint16_t const s_measurement_id_max_3 = 2048;
	static constexpr std::uint32_t const s_encoder_count_max = 90112;
	static constexpr std::uint32_t const s_status_gud = 0xFFFF'FFFFull;
	static constexpr std::uint32_t const s_status_bad = 0x0000'0000ull;

	static constexpr auto const s_verify_azimuth_block = [](azimuth_block_t const& azimuth_block) -> bool
	{
		bool ok = true;
		ok = ok && (azimuth_block.m_measurement_id < s_measurement_id_max_2);
		ok = ok && (azimuth_block.m_encoder_count < s_encoder_count_max);
		ok = ok && (azimuth_block.m_status == s_status_gud || azimuth_block.m_status == s_status_bad);
		return ok;
	};

	using std::cbegin;
	using std::cend;

	bool ok = true;
	ok = ok && (std::all_of(cbegin(packet.m_azimuth_blocks), cend(packet.m_azimuth_blocks), s_verify_azimuth_block));
	return ok;
}

template<typename t, int n>
struct my_array
{
	t const& operator[](int const& idx) const { return m_data[idx]; }
	t m_data[n];
};

void mk::ouster64::convert_to_xyza(packet_t const& packet, accept_point_fn_t const& accept_point_fn, void* const& ctx)
{
	static constexpr auto const s_deg_to_rad = [](double const& deg) -> double { return deg * (std::numbers::pi_v<double> / 180.0); };
	static constexpr auto const s_rad_to_deg = [](double const& rad) -> double { return rad * (180.0 / std::numbers::pi_v<double>); };

	static constexpr auto const s_transform = []<typename t, int n, typename fn_t>(t const(&input)[n], fn_t const& fn) -> auto
	{
		my_array<t, n> output;
		for(int i = 0; i != n; ++i)
		{
			output.m_data[i] = fn(input[i]);
		}
		return output;
	};

	static constexpr double const s_elevation_angles_deg[] =
	{
		16.856,
		16.26,
		15.694,
		15.147,
		14.649,
		14.093,
		13.547,
		12.987,
		12.523,
		11.936,
		11.417,
		10.881,
		10.369,
		9.823,
		9.306,
		8.765,
		8.274,
		7.736,
		7.211,
		6.679,
		6.186,
		5.631,
		5.106,
		4.555,
		4.079,
		3.558,
		3.012,
		2.478,
		2.01,
		1.448,
		0.921,
		0.367,
		-0.109,
		-0.64,
		-1.231,
		-1.723,
		-2.209,
		-2.738,
		-3.281,
		-3.825,
		-4.307,
		-4.845,
		-5.372,
		-5.923,
		-6.398,
		-6.931,
		-7.484,
		-8.015,
		-8.506,
		-9.033,
		-9.567,
		-10.134,
		-10.615,
		-11.142,
		-11.694,
		-12.254,
		-12.74,
		-13.293,
		-13.844,
		-14.401,
		-14.924,
		-15.452,
		-16.009,
		-16.612,
	};
	static constexpr auto const s_elevation_angles_cos = s_transform(s_elevation_angles_deg, [](double const& e){ return gcem::cos(s_deg_to_rad(e)); });
	static constexpr auto const s_elevation_angles_sin = s_transform(s_elevation_angles_deg, [](double const& e){ return gcem::sin(s_deg_to_rad(e)); });

	static constexpr double const s_azimuth_angles_deg[] =
	{
		3.165,
		1.009,
		-1.18,
		-3.287,
		3.139,
		0.99,
		-1.146,
		-3.244,
		3.115,
		0.984,
		-1.106,
		-3.231,
		3.103,
		1.007,
		-1.081,
		-3.18,
		3.103,
		1.004,
		-1.083,
		-3.152,
		3.11,
		1.016,
		-1.071,
		-3.143,
		3.111,
		1.019,
		-1.04,
		-3.123,
		3.126,
		1.049,
		-1.039,
		-3.104,
		3.145,
		1.057,
		-1.054,
		-3.1,
		3.146,
		1.061,
		-1.01,
		-3.09,
		3.165,
		1.077,
		-1.002,
		-3.081,
		3.187,
		1.09,
		-0.993,
		-3.096,
		3.209,
		1.109,
		-0.984,
		-3.088,
		3.246,
		1.136,
		-0.981,
		-3.099,
		3.289,
		1.155,
		-0.969,
		-3.115,
		3.318,
		1.174,
		-0.957,
		-3.144,
	};
	static constexpr auto const s_azimuth_angles_rad = s_transform(s_azimuth_angles_deg, s_deg_to_rad);

	static constexpr std::uint32_t const s_encoder_count_max = 90112;


	for(int i_azimuth_block = 0; i_azimuth_block != s_azimuth_blocks_per_packet; ++i_azimuth_block)
	{
		auto const& encoder_count = packet.m_azimuth_blocks[i_azimuth_block].m_encoder_count;
		double const azimuth_block_azimuth = static_cast<double>(encoder_count) * (360.0 / static_cast<double>(s_encoder_count_max));
		for(int i_data_block = 0; i_data_block != s_data_blocks_per_azimuth_block; ++i_data_block)
		{
			double const r = static_cast<double>(packet.m_azimuth_blocks[i_azimuth_block].m_data_blocks[i_data_block].m_range) / 1'000.0;
			double const azimuth_deg_ = azimuth_block_azimuth + s_azimuth_angles_deg[i_data_block];
			double const azimuth_deg = (std::min)(360.0, (std::max)(0.0, azimuth_deg_));
			double const azimuth_rad = s_deg_to_rad(azimuth_deg);
			double const x = r * std::cos(azimuth_rad) * s_elevation_angles_cos[i_data_block];
			double const y = -r * std::sin(azimuth_rad) * s_elevation_angles_cos[i_data_block];
			double const z = r * s_elevation_angles_sin[i_data_block];
			accept_point_fn(x, y, z, azimuth_block_azimuth, ctx);
		}
	}
}
