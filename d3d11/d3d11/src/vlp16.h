#pragma once


#include <cstdint>


namespace mk
{
	namespace vlp16
	{

		static constexpr int const s_channels_count = 16;
		static constexpr int const s_firing_sequences_count = 2;
		static constexpr int const s_data_blocks_count = 12;

		static constexpr int const s_points_per_packet = s_data_blocks_count * s_firing_sequences_count * s_channels_count;
		static constexpr int const s_packets_per_second = 754;
		static constexpr int const s_points_per_second = s_points_per_packet * s_packets_per_second;
		static constexpr int const s_packet_size = 1206;

		struct firing_sequence_t
		{
			std::uint16_t m_distance[s_channels_count];
			std::uint8_t m_reflexivity[s_channels_count];
		};

		struct data_block_t
		{
			std::uint16_t m_flag;
			std::uint16_t m_azimuth;
			firing_sequence_t m_firing_sequence[s_firing_sequences_count];
		};

		struct factory_t
		{
			std::uint8_t m_return_mode;
			std::uint8_t m_product_id;
		};

		struct single_mode_packet_t
		{
			factory_t m_factory;
			std::uint32_t m_timestamp;
			data_block_t m_data_blocks[s_data_blocks_count];
		};

		typedef void(* const& accept_point_fn_t)(double const& x, double const& y, double const& z, double const& a, void* const& ctx);


		single_mode_packet_t raw_data_to_single_mode_packet(void const* const& data, int const& len);
		bool verify_single_mode_packet(single_mode_packet_t const& packet);
		void convert_to_xyza(single_mode_packet_t const& packet, accept_point_fn_t const& accept_point_fn, void* const& ctx);


	}
}
