cbuffer constant_buffer : register(b0)
{
	matrix m_world;
	matrix m_view;
	matrix m_projection;
}


struct my_vertex_format_t
{
	float4 m_position : SV_Position;
	float4 m_color : COLOR0;
};


my_vertex_format_t vertex_shader_main(float4 position : POSITION)
{
	float4 pos = position;
	pos = mul(pos, m_world);
	pos = mul(pos, m_view);
	pos = mul(pos, m_projection);

	float4 col = float4(1.0f, 1.0f, 1.0f, 1.0f);

	my_vertex_format_t output;
	output.m_position = pos;
	output.m_color = col;
	return output;
}
