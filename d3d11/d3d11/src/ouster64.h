#pragma once


#include <cstdint>


namespace mk
{
	namespace ouster64
	{

		static constexpr int const s_azimuth_blocks_per_packet = 16;
		static constexpr int const s_data_blocks_per_azimuth_block = 64;

		static constexpr int const s_bytes_per_packet = 12608;
		static constexpr int const s_points_per_second = 1'310'720;
		static constexpr int const s_min_rotations_per_second = 10;
		static constexpr int const s_max_rotations_per_second = 20;
		static constexpr int const s_points_per_packet = s_azimuth_blocks_per_packet * s_data_blocks_per_azimuth_block;
		static constexpr int const s_packets_per_second = s_points_per_second / s_points_per_packet;
		static constexpr int const s_bytes_per_second = s_bytes_per_packet * s_packets_per_second;
		static constexpr int const s_min_points_per_rotation = s_points_per_second / s_max_rotations_per_second;
		static constexpr int const s_max_points_per_rotation = s_points_per_second / s_min_rotations_per_second;

		struct data_block_t
		{
			std::uint32_t m_range;
			std::uint16_t m_reflectivity;
			std::uint16_t m_signal_photons;
			std::uint16_t m_noise_photons;
		};

		struct azimuth_block_t
		{
			std::uint64_t m_timestamp;
			std::uint16_t m_measurement_id;
			std::uint16_t m_frame_id;
			std::uint32_t m_encoder_count;
			data_block_t m_data_blocks[s_data_blocks_per_azimuth_block];
			std::uint32_t m_status;
		};

		struct packet_t
		{
			azimuth_block_t m_azimuth_blocks[s_azimuth_blocks_per_packet];
		};

		typedef void(* const& accept_point_fn_t)(double const& x, double const& y, double const& z, double const& a, void* const& ctx);


		packet_t raw_data_to_packet(void const* const& data, int const& len);
		bool verify_packet(packet_t const& packet);
		void convert_to_xyza(packet_t const& packet, accept_point_fn_t const& accept_point_fn, void* const& ctx);


	}
}
