struct my_vertex_format_t
{
	float4 m_position : SV_Position;
	float4 m_color : COLOR0;
};


float4 pixel_shader_main(my_vertex_format_t input) : SV_Target
{
	return input.m_color;
}
