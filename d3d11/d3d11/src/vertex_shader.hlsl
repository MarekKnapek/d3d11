cbuffer constant_buffer : register(b0)
{
	matrix m_world;
	matrix m_view;
	matrix m_projection;
}


struct my_pixel_format_t
{
	float4 m_position : SV_Position;
	float4 m_color : COLOR0;
};


my_pixel_format_t vertex_shader_main(float4 position : POSITION, float4 color : COLOR)
{
	my_pixel_format_t output = (my_pixel_format_t)0;
	output.m_position = mul(position, m_world);
	output.m_position = mul(output.m_position, m_view);
	output.m_position = mul(output.m_position, m_projection);
	output.m_color = position;
	return output;
}
