cbuffer constant_buffer : register(b0)
{
	matrix m_world;
	matrix m_view;
	matrix m_projection;
}

struct my_pixel_output_format_t
{
	float4 m_position : SV_Position;
	float4 m_color : COLOR0;
};


my_pixel_output_format_t vertex_shader_main(float4 position : POSITION, float3 instance_offset : TEXCOORD)
{
	float4 pos = position;
	pos.x += instance_offset.x;
	pos.y += instance_offset.y;
	pos.z += instance_offset.z;

	pos = mul(pos, m_world);
	pos = mul(pos, m_view);
	pos = mul(pos, m_projection);

	float4 col = float4(1.0f, 1.0f, 1.0f, 1.0f);

	my_pixel_output_format_t output;
	output.m_position = pos;
	output.m_color = col;
	return output;
}
