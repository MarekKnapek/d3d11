#include "scope_exit.h"

#include <algorithm> // std::all_of, std::fill
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath> // std::sin, std::cos
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t
#include <cstdio> // std::printf, std::puts
#include <cstdlib> // std::exit, EXIT_SUCCESS, EXIT_FAILURE
#include <cstring> // std::memcpy
#include <cwchar> // std::wcslen
#include <iterator> // std::size, std::cbegin, std::cend, std::begin, std::end
#include <numbers> // std::numbers::pi_v
#include <string> // std::u8string, std::u16string
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <dxgi.h>
#pragma warning(push)
#pragma warning(disable:4005) // Macro redefinition.
#include "c:\\Users\\me\\Downloads\\dx\\dx9\\include\\d3dx11core.h"
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable:4838) // Narowwing conversion.
#include "c:\\Users\\me\\Downloads\\dx\\dx9\\include\\xnamath.h"
#pragma warning(pop)
#include <winsock2.h>


#ifdef NDEBUG
	#include "vertex_shader_release.h"
	#include "pixel_shader_release.h"
#else
	#include "vertex_shader_debug.h"
	#include "pixel_shader_debug.h"
#endif


#ifdef _M_IX86
	#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x86\\dxgi.lib")
	#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x86\\dxguid.lib")
#else
	#ifdef _M_X64
		#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x64\\dxgi.lib")
		#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x64\\dxguid.lib")
	#else
		#error Unknown architecture.
	#endif
#endif


#ifdef _M_IX86
	#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x86\\d3d11.lib")
	#ifdef NDEBUG
		#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x86\\d3dx11.lib")
	#else
		#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x86\\d3dx11d.lib")
	#endif
#else
	#ifdef _M_X64
		#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x64\\d3d11.lib")
		#ifdef NDEBUG
			#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x64\\d3dx11.lib")
		#else
			#pragma comment(lib, "c:\\Users\\me\\Downloads\\dx\\dx9\\Lib\\x64\\d3dx11d.lib")
		#endif
	#else
		#error Unknown architecture.
	#endif
#endif


#pragma comment(lib, "ws2_32.lib")


struct float3_t
{
	float m_x;
	float m_y;
	float m_z;
};

struct float4_t
{
	float m_x;
	float m_y;
	float m_z;
	float m_w;
};

struct my_vertex_t
{
	float3_t m_position;
	float4_t m_color;
};


struct my_constant_buffer_t
{
	XMMATRIX m_world;
	XMMATRIX m_view;
	XMMATRIX m_projection;
};


struct incomming_point_t
{
	float m_azimuth;
	float m_x;
	float m_y;
	float m_z;
};


struct app_state_t
{
	ATOM m_main_window_class;
	HWND m_main_window;
	int m_width;
	int m_height;
	ID3D11Device* m_d3d11_device;
	ID3D11DeviceContext* m_d3d11_immediate_context;
	IDXGISwapChain* m_d3d11_swap_chain;
	ID3D11Texture2D* m_d3d11_back_buffer;
	ID3D11RenderTargetView* m_d3d11_render_target_view;
	ID3D11VertexShader* m_d3d11_vertex_shader;
	ID3D11PixelShader* m_d3d11_pixel_shader;
	ID3D11Buffer* m_d3d11_vertex_buffer;
	ID3D11Buffer* m_d3d11_index_buffer;
	ID3D11Buffer* m_d3d11_constant_buffer;
	float m_rotation;
	my_constant_buffer_t m_d3d11_transformations;
	std::chrono::high_resolution_clock::time_point m_prev_time;
	std::chrono::high_resolution_clock::time_point m_fps_time;
	int m_fps_count;
	std::atomic<bool> m_thread_end_requested;
	incomming_point_t m_incomming_points[64 * 1024];
	unsigned m_incomming_points_idx;
};


static app_state_t* g_app_state;


void check_ret_failed(char const* const& file, int const& line, char const* const& expr);
int seh_filter(unsigned int const code, EXCEPTION_POINTERS* const ep);
bool d3d11_app_seh(int const argc, char const* const* const argv, int* const& out_exit_code);
bool d3d11_app(int const argc, char const* const* const argv, int* const& out_exit_code);
bool register_main_window_class(ATOM* const& out_main_window_class);
bool unregister_main_window_class(ATOM const& main_window_class);
LRESULT CALLBACK main_window_proc(_In_ HWND const hwnd, _In_ UINT const msg, _In_ WPARAM const w_param, _In_ LPARAM const l_param);
bool create_main_window(ATOM const& main_window_class, HWND* const& out_main_window);
bool refresh_window_size(HWND const& whnd, int* const& out_width, int* const& out_height);
bool run_main_loop(int* const& out_exit_code);
bool process_message(MSG const& msg);
bool on_idle();
std::u8string utf16_to_utf8(std::u16string const& u16str);
bool render();
void network_thread_proc();
void process_data(unsigned char const* const& data, int const& data_len);


#define CHECK_RET(X, R) do{ if(X){}else{ [[unlikely]] check_ret_failed(__FILE__, __LINE__, #X); return R; } }while(false)
#define CHECK_RET_V(X) do{ if(X){}else{ [[unlikely]] check_ret_failed(__FILE__, __LINE__, #X); std::exit(EXIT_FAILURE); } }while(false)


int main(int const argc, char const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([&](){ std::puts("Oh, no! Someting went wrong!"); });

	int exit_code;
	bool const bussiness = d3d11_app_seh(argc, argv, &exit_code);
	CHECK_RET(bussiness, EXIT_FAILURE);

	something_wrong.reset();
	std::puts("We didn't crash! Great Success!");
	return exit_code;
}


void check_ret_failed(char const* const& file, int const& line, char const* const& expr)
{
	std::printf("Failed in file `%s` at line %d with `%s`.\n", file, line, expr);
}

int seh_filter(unsigned int const code, EXCEPTION_POINTERS* const ep)
{
	(void)ep;
	static_assert(sizeof(DWORD) == sizeof(unsigned int));
	int code_int;
	std::memcpy(&code_int, &code, sizeof(code));
	std::printf("SEH exception %d (0x%8X).\n", code_int, code);
	std::exit(EXIT_FAILURE);
	// return EXCEPTION_CONTINUE_SEARCH;
}

bool d3d11_app_seh(int const argc, char const* const* const argv, int* const& out_exit_code)
{
	__try
	{
		int exit_code;
		bool const bussiness = d3d11_app(argc, argv, &exit_code);
		CHECK_RET(bussiness, false);
		*out_exit_code = exit_code;
		return true;
	}
	__except(seh_filter(GetExceptionCode(), GetExceptionInformation()))
	{
	}
	return false;
}

bool d3d11_app(int const argc, char const* const* const argv, int* const& out_exit_code)
{
	(void)argc;
	(void)argv;

	app_state_t* const app_state = new app_state_t{};
	auto const app_state_free = mk::make_scope_exit([&](){ delete g_app_state; });
	g_app_state = app_state;

	HRESULT const com_initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
	CHECK_RET(com_initialized == S_OK, false);
	auto const com_uninitialize = mk::make_scope_exit([](){ CoUninitialize(); });

	bool const registered = register_main_window_class(&g_app_state->m_main_window_class);
	CHECK_RET(registered, false);
	auto const unregister = mk::make_scope_exit([&](){ bool const unregistered = unregister_main_window_class(g_app_state->m_main_window_class); CHECK_RET_V(unregistered); });

	bool const window_created = create_main_window(g_app_state->m_main_window_class, &g_app_state->m_main_window);
	CHECK_RET(window_created, false);

	bool const window_size_refreshed = refresh_window_size(g_app_state->m_main_window, &g_app_state->m_width, &g_app_state->m_height);
	CHECK_RET(window_size_refreshed, false);

	/* D3D11 */
	bool const d3d11_version_check = !FAILED(D3DX11CheckVersion(D3D11_SDK_VERSION, D3DX11_SDK_VERSION));
	CHECK_RET(d3d11_version_check, false);

	IDXGIFactory1* d3d11_factory;
	HRESULT const d311_factory_created =  CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void**>(&d3d11_factory));
	CHECK_RET(d311_factory_created == S_OK, false);
	auto const d3d11_factory_free = mk::make_scope_exit([&](){ d3d11_factory->Release(); });

	for(UINT d3d11_adapter_idx = 0;; ++d3d11_adapter_idx)
	{
		IDXGIAdapter1* d3d11_adapter;
		HRESULT const d3d11_adapters_enumed = d3d11_factory->EnumAdapters1(d3d11_adapter_idx, &d3d11_adapter);
		if(d3d11_adapters_enumed == DXGI_ERROR_NOT_FOUND) break;
		CHECK_RET(d3d11_adapters_enumed == S_OK, false);
		auto const d3d11_adapter_free = mk::make_scope_exit([&](){ d3d11_adapter->Release(); });

		DXGI_ADAPTER_DESC1 d3d11_adapter_description;
		HRESULT const d3d11_adapter_got_description = d3d11_adapter->GetDesc1(&d3d11_adapter_description);
		CHECK_RET(d3d11_adapter_got_description == S_OK, false);

		static_assert(sizeof(UINT) == sizeof(unsigned));
		static_assert(sizeof(unsigned) == sizeof(std::uint32_t));
		static_assert(sizeof(SIZE_T) == sizeof(std::size_t));
		static_assert(sizeof(DWORD) == sizeof(unsigned));
		static_assert(sizeof(LONG) == sizeof(int));
		static constexpr char const s_d3d11_DXGI_ADAPTER_FLAG_REMOTE[] = ", DXGI_ADAPTER_FLAG_REMOTE";
		static constexpr char const s_d3d11_DXGI_ADAPTER_FLAG_SOFTWARE[] = ", DXGI_ADAPTER_FLAG_SOFTWARE";
		std::printf("Description:             `%s'\n", reinterpret_cast<char const*>(utf16_to_utf8(std::u16string{d3d11_adapter_description.Description, d3d11_adapter_description.Description + std::wcslen(d3d11_adapter_description.Description)}).c_str()));
		std::printf("Vendor:                  0x%08X (%u)\n", static_cast<unsigned>(d3d11_adapter_description.VendorId), static_cast<unsigned>(d3d11_adapter_description.VendorId));
		std::printf("Device:                  0x%08X (%u)\n", static_cast<unsigned>(d3d11_adapter_description.DeviceId), static_cast<unsigned>(d3d11_adapter_description.DeviceId));
		std::printf("Sub system:              0x%08X (%u)\n", static_cast<unsigned>(d3d11_adapter_description.SubSysId), static_cast<unsigned>(d3d11_adapter_description.SubSysId));
		std::printf("Revision:                0x%08X (%u)\n", static_cast<unsigned>(d3d11_adapter_description.Revision), static_cast<unsigned>(d3d11_adapter_description.Revision));
		std::printf("Dedicated video memory:  0x%zX (%zu B, %.1f kB, %.1f MB, %.1f GB)\n", static_cast<std::size_t>(d3d11_adapter_description.DedicatedVideoMemory), static_cast<std::size_t>(d3d11_adapter_description.DedicatedVideoMemory), static_cast<double>(d3d11_adapter_description.DedicatedVideoMemory) / 1024.0, static_cast<double>(d3d11_adapter_description.DedicatedVideoMemory) / 1024.0 / 1024.0, static_cast<double>(d3d11_adapter_description.DedicatedVideoMemory) / 1024.0 / 1024.0 / 1024.0);
		std::printf("Dedicated system memory: 0x%zX (%zu B, %.1f kB, %.1f MB, %.1f GB)\n", static_cast<std::size_t>(d3d11_adapter_description.DedicatedSystemMemory), static_cast<std::size_t>(d3d11_adapter_description.DedicatedSystemMemory), static_cast<double>(d3d11_adapter_description.DedicatedSystemMemory) / 1024.0, static_cast<double>(d3d11_adapter_description.DedicatedSystemMemory) / 1024.0 / 1024.0, static_cast<double>(d3d11_adapter_description.DedicatedSystemMemory) / 1024.0 / 1024.0 / 1024.0);
		std::printf("Shared system memory:    0x%zX (%zu B, %.1f kB, %.1f MB, %.1f GB)\n", static_cast<std::size_t>(d3d11_adapter_description.SharedSystemMemory), static_cast<std::size_t>(d3d11_adapter_description.SharedSystemMemory), static_cast<double>(d3d11_adapter_description.SharedSystemMemory) / 1024.0, static_cast<double>(d3d11_adapter_description.SharedSystemMemory) / 1024.0 / 1024.0, static_cast<double>(d3d11_adapter_description.SharedSystemMemory) / 1024.0 / 1024.0 / 1024.0);
		std::printf("LUID:                    {%u, %d}\n", static_cast<unsigned>(d3d11_adapter_description.AdapterLuid.LowPart), static_cast<int>(d3d11_adapter_description.AdapterLuid.HighPart));
		std::printf("Flags:                   0x%08X (%u%s%s)\n\n", static_cast<unsigned>(d3d11_adapter_description.Flags), static_cast<unsigned>(d3d11_adapter_description.Flags), (d3d11_adapter_description.Flags & DXGI_ADAPTER_FLAG_REMOTE) != 0 ? static_cast<char const*>(s_d3d11_DXGI_ADAPTER_FLAG_REMOTE) : static_cast<char const*>(""), (d3d11_adapter_description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 ? static_cast<char const*>(s_d3d11_DXGI_ADAPTER_FLAG_SOFTWARE) : static_cast<char const*>(""));
	};

	ID3D11Device* d3d11_device;
	D3D_FEATURE_LEVEL d3d11_feature_level;
	ID3D11DeviceContext* d3d11_immediate_context;
	UINT d3d11_device_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
	#ifndef NDEBUG
	d3d11_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	HRESULT const d3d11_device_created = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, d3d11_device_flags, nullptr, 0, D3D11_SDK_VERSION, &d3d11_device, &d3d11_feature_level, &d3d11_immediate_context);
	CHECK_RET(d3d11_device_created == S_OK, false);
	auto const d3d11_device_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_device->Release(); });
	auto const d3d11_immediate_context_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_immediate_context->Release(); });
	g_app_state->m_d3d11_device = d3d11_device;
	g_app_state->m_d3d11_immediate_context = d3d11_immediate_context;

	IDXGISwapChain* d3d11_swap_chain;
	DXGI_SWAP_CHAIN_DESC d3d11_swap_chain_description;
	d3d11_swap_chain_description.BufferDesc.Width = g_app_state->m_width;
	d3d11_swap_chain_description.BufferDesc.Height = g_app_state->m_height;
	d3d11_swap_chain_description.BufferDesc.RefreshRate.Numerator = 0;
	d3d11_swap_chain_description.BufferDesc.RefreshRate.Denominator = 0;
	d3d11_swap_chain_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3d11_swap_chain_description.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	d3d11_swap_chain_description.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	d3d11_swap_chain_description.SampleDesc.Count = 1;
	d3d11_swap_chain_description.SampleDesc.Quality = 0;
	d3d11_swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
	d3d11_swap_chain_description.BufferCount = 1;
	d3d11_swap_chain_description.OutputWindow = g_app_state->m_main_window;
	d3d11_swap_chain_description.Windowed = TRUE;
	d3d11_swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	d3d11_swap_chain_description.Flags = 0;
	HRESULT const d3d11_swap_chain_created = d3d11_factory->CreateSwapChain(d3d11_device, &d3d11_swap_chain_description, &d3d11_swap_chain);
	CHECK_RET(d3d11_swap_chain_created == S_OK, false);
	auto const d3d11_swap_chain_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_swap_chain->Release(); });
	g_app_state->m_d3d11_swap_chain = d3d11_swap_chain;

	ID3D11Texture2D* d3d11_back_buffer;
	HRESULT const d3d11_got_back_buffer = d3d11_swap_chain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void**>(&d3d11_back_buffer));
	CHECK_RET(d3d11_got_back_buffer == S_OK, false);
	auto const back_buffer_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_back_buffer->Release(); });
	g_app_state->m_d3d11_back_buffer = d3d11_back_buffer;

	ID3D11RenderTargetView* d3d11_render_target_view;
	HRESULT const d3d11_render_target_view_created = d3d11_device->CreateRenderTargetView(d3d11_back_buffer, nullptr, &d3d11_render_target_view);
	CHECK_RET(d3d11_render_target_view_created == S_OK, false);
	auto const d3d11_render_target_view_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_render_target_view->Release(); });
	g_app_state->m_d3d11_render_target_view = d3d11_render_target_view;
	
	d3d11_immediate_context->OMSetRenderTargets(1, &d3d11_render_target_view, nullptr);

	D3D11_VIEWPORT d3d11_view_port;
	d3d11_view_port.TopLeftX = 0.0f;
	d3d11_view_port.TopLeftY = 0.0f;
	d3d11_view_port.Width = static_cast<float>(g_app_state->m_width);
	d3d11_view_port.Height = static_cast<float>(g_app_state->m_height);
	d3d11_view_port.MinDepth = 0.0f;
	d3d11_view_port.MaxDepth = 1.0f;
	d3d11_immediate_context->RSSetViewports(1, &d3d11_view_port);

	ID3D11VertexShader* d3d11_vertex_shader;
	HRESULT const d3d11_vertex_shader_created = g_app_state->m_d3d11_device->CreateVertexShader(g_vertex_shader_main, std::size(g_vertex_shader_main), nullptr, &d3d11_vertex_shader);
	CHECK_RET(d3d11_vertex_shader_created == S_OK, false);
	auto const d3d11_vertex_shader_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_vertex_shader->Release(); });
	g_app_state->m_d3d11_vertex_shader = d3d11_vertex_shader;

	ID3D11InputLayout* d3d11_input_layout;
	D3D11_INPUT_ELEMENT_DESC d3d11_shader_imput_layout[2];
	d3d11_shader_imput_layout[0].SemanticName = "POSITION";
	d3d11_shader_imput_layout[0].SemanticIndex = 0;
	d3d11_shader_imput_layout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	d3d11_shader_imput_layout[0].InputSlot = 0;
	d3d11_shader_imput_layout[0].AlignedByteOffset = 0;
	d3d11_shader_imput_layout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	d3d11_shader_imput_layout[0].InstanceDataStepRate = 0;
	d3d11_shader_imput_layout[1].SemanticName = "COLOR";
	d3d11_shader_imput_layout[1].SemanticIndex = 0;
	d3d11_shader_imput_layout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	d3d11_shader_imput_layout[1].InputSlot = 0;
	d3d11_shader_imput_layout[1].AlignedByteOffset = 12;
	d3d11_shader_imput_layout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	d3d11_shader_imput_layout[1].InstanceDataStepRate = 0;
	HRESULT const d3d11_input_layout_created = g_app_state->m_d3d11_device->CreateInputLayout(d3d11_shader_imput_layout, static_cast<int>(std::size(d3d11_shader_imput_layout)), g_vertex_shader_main, std::size(g_vertex_shader_main), &d3d11_input_layout);
	CHECK_RET(d3d11_input_layout_created == S_OK, false);
	auto const d3d11_input_layout_free = mk::make_scope_exit([&](){ d3d11_input_layout->Release(); });

	g_app_state->m_d3d11_immediate_context->IASetInputLayout(d3d11_input_layout);

	ID3D11PixelShader* d3d11_pixel_shader;
	HRESULT const d3d11_pixel_shader_created = g_app_state->m_d3d11_device->CreatePixelShader(g_pixel_shader_main, std::size(g_pixel_shader_main), nullptr, &d3d11_pixel_shader);
	CHECK_RET(d3d11_pixel_shader_created == S_OK, false);
	auto const d3d11_pixel_shader_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_pixel_shader->Release(); });
	g_app_state->m_d3d11_pixel_shader = d3d11_pixel_shader;

	static constexpr my_vertex_t const s_cube_vertex_buffer[] =
	{
		{float3_t{-1.0f, +1.0f, -1.0f}, float4_t{0.0f, 0.0f, 1.0f, 1.0f}},
		{float3_t{+1.0f, +1.0f, -1.0f}, float4_t{0.0f, 1.0f, 0.0f, 1.0f}},
		{float3_t{+1.0f, +1.0f, +1.0f}, float4_t{0.0f, 1.0f, 1.0f, 1.0f}},
		{float3_t{-1.0f, +1.0f, +1.0f}, float4_t{1.0f, 0.0f, 0.0f, 1.0f}},
		{float3_t{-1.0f, -1.0f, -1.0f}, float4_t{1.0f, 0.0f, 1.0f, 1.0f}},
		{float3_t{+1.0f, -1.0f, -1.0f}, float4_t{1.0f, 1.0f, 0.0f, 1.0f}},
		{float3_t{+1.0f, -1.0f, +1.0f}, float4_t{1.0f, 1.0f, 1.0f, 1.0f}},
		{float3_t{-1.0f, -1.0f, +1.0f}, float4_t{0.0f, 0.0f, 0.0f, 1.0f}},
	};
	ID3D11Buffer* d3d11_vertex_buffer;
	D3D11_BUFFER_DESC d3d11_vertex_buffer_description;
	d3d11_vertex_buffer_description.ByteWidth = static_cast<int>(std::size(s_cube_vertex_buffer)) * sizeof(my_vertex_t);
	d3d11_vertex_buffer_description.Usage = D3D11_USAGE_IMMUTABLE; // D3D11_USAGE_DYNAMIC
	d3d11_vertex_buffer_description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	d3d11_vertex_buffer_description.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE
	d3d11_vertex_buffer_description.MiscFlags = 0;
	d3d11_vertex_buffer_description.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA d3d11_vertex_buffer_resource_data;
	d3d11_vertex_buffer_resource_data.pSysMem = s_cube_vertex_buffer;
	d3d11_vertex_buffer_resource_data.SysMemPitch = 0;
	d3d11_vertex_buffer_resource_data.SysMemSlicePitch = 0;
	HRESULT const d3d11_vertex_buffer_created = g_app_state->m_d3d11_device->CreateBuffer(&d3d11_vertex_buffer_description, &d3d11_vertex_buffer_resource_data, &d3d11_vertex_buffer);
	CHECK_RET(d3d11_vertex_buffer_created == S_OK, false);
	auto const d3d11_vertex_buffer_free = mk::make_scope_exit([](){ g_app_state->m_d3d11_vertex_buffer->Release(); });
	g_app_state->m_d3d11_vertex_buffer = d3d11_vertex_buffer;

	UINT const d3d11_vertex_buffer_stride = sizeof(my_vertex_t);
	UINT const d3d11_vertex_buffer_offset = 0;
	g_app_state->m_d3d11_immediate_context->IASetVertexBuffers(0, 1, &g_app_state->m_d3d11_vertex_buffer, &d3d11_vertex_buffer_stride, &d3d11_vertex_buffer_offset);

	static constexpr std::uint16_t const s_cube_index_buffer[] =
	{
		3, 1, 0,
		2, 1, 3,
		0, 5, 4,
		1, 5, 0,
		3, 4, 7,
		0, 4, 3,
		1, 6, 5,
		2, 6, 1,
		2, 7, 6,
		3, 7, 2,
		6, 4, 5,
		7, 4, 6,
	};
	ID3D11Buffer* d3d11_index_buffer;
	D3D11_BUFFER_DESC d3d11_index_buffer_description;
	d3d11_index_buffer_description.ByteWidth = static_cast<int>(std::size(s_cube_index_buffer)) * sizeof(std::uint16_t);
	d3d11_index_buffer_description.Usage = D3D11_USAGE_IMMUTABLE; // D3D11_USAGE_DYNAMIC
	d3d11_index_buffer_description.BindFlags = D3D11_BIND_INDEX_BUFFER;
	d3d11_index_buffer_description.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE
	d3d11_index_buffer_description.MiscFlags = 0;
	d3d11_index_buffer_description.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA d3d11_index_buffer_resource_data;
	d3d11_index_buffer_resource_data.pSysMem = s_cube_index_buffer;
	d3d11_index_buffer_resource_data.SysMemPitch = 0;
	d3d11_index_buffer_resource_data.SysMemSlicePitch = 0;
	HRESULT const d3d11_index_buffer_created = g_app_state->m_d3d11_device->CreateBuffer(&d3d11_index_buffer_description, &d3d11_index_buffer_resource_data, &d3d11_index_buffer);
	CHECK_RET(d3d11_index_buffer_created == S_OK, false);
	auto const d3d11_index_buffer_free = mk::make_scope_exit([](){ g_app_state->m_d3d11_index_buffer->Release(); });
	g_app_state->m_d3d11_index_buffer = d3d11_index_buffer;

	g_app_state->m_d3d11_immediate_context->IASetIndexBuffer(g_app_state->m_d3d11_index_buffer, DXGI_FORMAT_R16_UINT, 0);

	g_app_state->m_d3d11_immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11Buffer* d3d11_constant_buffer;
	D3D11_BUFFER_DESC d3d11_constant_buffer_description;
	d3d11_constant_buffer_description.ByteWidth = sizeof(my_constant_buffer_t);
	d3d11_constant_buffer_description.Usage = D3D11_USAGE_DEFAULT;
	d3d11_constant_buffer_description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	d3d11_constant_buffer_description.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE
	d3d11_constant_buffer_description.MiscFlags = 0;
	d3d11_constant_buffer_description.StructureByteStride = 0;
	HRESULT const d3d11_constant_buffer_created = g_app_state->m_d3d11_device->CreateBuffer(&d3d11_constant_buffer_description, nullptr, &d3d11_constant_buffer);
	CHECK_RET(d3d11_constant_buffer_created == S_OK, false);
	auto const d3d11_constant_buffer_free = mk::make_scope_exit([](){ g_app_state->m_d3d11_constant_buffer->Release(); });
	g_app_state->m_d3d11_constant_buffer = d3d11_constant_buffer;

	g_app_state->m_rotation = 0.0f;

	g_app_state->m_d3d11_transformations.m_world = XMMatrixIdentity();

	XMVECTOR const d3d11_eye = {0.0f, 1.0f, -5.0f, 0.0f};
	XMVECTOR const d3d11_at = {0.0f, 1.0f, 0.0f, 0.0f};
	XMVECTOR const d3d11_up = {0.0f, 1.0f, 0.0f, 0.0f};
	g_app_state->m_d3d11_transformations.m_view = XMMatrixLookAtLH(d3d11_eye, d3d11_at, d3d11_up);

	g_app_state->m_d3d11_transformations.m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(g_app_state->m_width) / static_cast<float>(g_app_state->m_height), 0.01f, 100.0f);
	/* D3D11 */

	g_app_state->m_thread_end_requested.store(false);
	std::thread network_thread{&network_thread_proc};
	auto const network_thread_free = mk::make_scope_exit([&](){ g_app_state->m_thread_end_requested.store(true); network_thread.join(); });

	g_app_state->m_prev_time = std::chrono::high_resolution_clock::now();
	g_app_state->m_fps_time = g_app_state->m_prev_time;

	ShowWindow(g_app_state->m_main_window, SW_SHOW);

	int exit_code;
	bool const main_loop_ran = run_main_loop(&exit_code);
	CHECK_RET(main_loop_ran, false);

	*out_exit_code = exit_code;
	return true;
}

bool register_main_window_class(ATOM* const& out_main_window_class)
{
	assert(out_main_window_class);

	HMODULE const self = GetModuleHandleW(nullptr);
	CHECK_RET(self != nullptr, false);

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &main_window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = self;
	wc.hIcon = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));
	wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<wchar_t const*>(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"main_window";
	wc.hIconSm = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));

	ATOM const main_window_class = RegisterClassExW(&wc);
	CHECK_RET(main_window_class != 0, false);

	*out_main_window_class = main_window_class;
	return true;
}

bool unregister_main_window_class(ATOM const& main_window_class)
{
	assert(main_window_class != 0);

	HMODULE const self = GetModuleHandleW(nullptr);
	CHECK_RET(self != nullptr, false);

	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(main_window_class), self);
	CHECK_RET(unregistered != 0, false);

	return true;
}

LRESULT CALLBACK main_window_proc(_In_ HWND const hwnd, _In_ UINT const msg, _In_ WPARAM const w_param, _In_ LPARAM const l_param)
{
	switch(msg)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(EXIT_SUCCESS);
		}
		break;
		case WM_SIZE:
		{
			g_app_state->m_d3d11_back_buffer->Release();

			g_app_state->m_d3d11_immediate_context->OMSetRenderTargets(0, nullptr, nullptr);
			g_app_state->m_d3d11_render_target_view->Release();

			bool const window_size_refreshed = refresh_window_size(g_app_state->m_main_window, &g_app_state->m_width, &g_app_state->m_height);
			CHECK_RET_V(window_size_refreshed);

			HRESULT const resized = g_app_state->m_d3d11_swap_chain->ResizeBuffers(1, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
			CHECK_RET_V(resized == S_OK);

			ID3D11Texture2D* d3d11_back_buffer;
			HRESULT const d3d11_got_back_buffer = g_app_state->m_d3d11_swap_chain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void**>(&d3d11_back_buffer));
			CHECK_RET(d3d11_got_back_buffer == S_OK, false);
			g_app_state->m_d3d11_back_buffer = d3d11_back_buffer;

			ID3D11RenderTargetView* d3d11_render_target_view;
			HRESULT const d3d11_render_target_view_created = g_app_state->m_d3d11_device->CreateRenderTargetView(g_app_state->m_d3d11_back_buffer, nullptr, &d3d11_render_target_view);
			CHECK_RET(d3d11_render_target_view_created == S_OK, false);
			g_app_state->m_d3d11_render_target_view = d3d11_render_target_view;
	
			g_app_state->m_d3d11_immediate_context->OMSetRenderTargets(1, &d3d11_render_target_view, nullptr);

			D3D11_VIEWPORT d3d11_view_port;
			d3d11_view_port.TopLeftX = 0.0f;
			d3d11_view_port.TopLeftY = 0.0f;
			d3d11_view_port.Width = static_cast<float>(g_app_state->m_width);
			d3d11_view_port.Height = static_cast<float>(g_app_state->m_height);
			d3d11_view_port.MinDepth = 0.0f;
			d3d11_view_port.MaxDepth = 1.0f;
			g_app_state->m_d3d11_immediate_context->RSSetViewports(1, &d3d11_view_port);

			g_app_state->m_d3d11_transformations.m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(g_app_state->m_width) / static_cast<float>(g_app_state->m_height), 0.01f, 100.0f);
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC const dc = BeginPaint(g_app_state->m_main_window, &ps);
			CHECK_RET_V(dc != nullptr);
			BOOL paint_ended = EndPaint(g_app_state->m_main_window, &ps);
			CHECK_RET_V(paint_ended != 0);
			bool const rendered = render();
			CHECK_RET_V(rendered);
		}
		break;
	}
	LRESULT const ret = DefWindowProcW(hwnd, msg, w_param, l_param);
	return ret;
}

bool create_main_window(ATOM const& main_window_class, HWND* const& out_main_window)
{
	HMODULE const self = GetModuleHandleW(nullptr);
	CHECK_RET(self != nullptr, false);

	HWND const main_window = CreateWindowExW
	(
		0,
		reinterpret_cast<wchar_t const*>(main_window_class),
		L"Main Window",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		nullptr,
		nullptr,
		self,
		nullptr
	);
	CHECK_RET(main_window != nullptr, false);

	*out_main_window = main_window;
	return true;
}

bool refresh_window_size(HWND const& whnd, int* const& out_width, int* const& out_height)
{
	RECT r;
	BOOL const got_client_rect = GetClientRect(whnd, &r);
	CHECK_RET(got_client_rect != 0, false);

	*out_width = static_cast<int>(r.right) - static_cast<int>(r.left);
	*out_height = static_cast<int>(r.bottom) - static_cast<int>(r.top);

	return true;
}

bool run_main_loop(int* const& out_exit_code)
{
	MSG msg;
	BOOL got;
	for(;;)
	{
		while((got = PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) != 0)
		{
			if(msg.message != WM_QUIT) [[likely]]
			{
				bool processed = process_message(msg);
				CHECK_RET(processed, false);
			}
			else
			{
				CHECK_RET(msg.hwnd == nullptr, false);
				int const exit_code = static_cast<int>(msg.wParam);
				*out_exit_code = exit_code;
				return true;
			}
		}
		bool const idled = on_idle();
		CHECK_RET(idled, false);
		got = GetMessageW(&msg, nullptr, 0, 0);
		if(got != 0) [[likely]]
		{
			bool processed = process_message(msg);
			CHECK_RET(processed, false);
		}
		else
		{
			CHECK_RET(got != -1, false);
			CHECK_RET(msg.hwnd == nullptr, false);
			int const exit_code = static_cast<int>(msg.wParam);
			*out_exit_code = exit_code;
			return true;
		}
	}
}

bool process_message(MSG const& msg)
{
	BOOL const translated = TranslateMessage(&msg);
	LRESULT const dispatched = DispatchMessageW(&msg);

	return true;
}

bool on_idle()
{
	BOOL const invalidated = InvalidateRect(g_app_state->m_main_window, nullptr, FALSE);
	CHECK_RET(invalidated != 0, false);

	return true;
}

std::u8string utf16_to_utf8(std::u16string const& u16str)
{
	int const len_bytes_1 = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, reinterpret_cast<wchar_t const*>(u16str.data()), static_cast<int>(u16str.size()), nullptr, 0, nullptr, nullptr);
	CHECK_RET_V(len_bytes_1 != 0);
	std::u8string u8str;
	u8str.resize(len_bytes_1);
	int const len_bytes_2 = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, reinterpret_cast<wchar_t const*>(u16str.data()), static_cast<int>(u16str.size()), reinterpret_cast<char*>(u8str.data()), len_bytes_1, nullptr, nullptr);
	CHECK_RET_V(len_bytes_2 == len_bytes_1);
	return u8str;
}

bool render()
{
	static constexpr float const s_rotation_speed = 0.001f;
	static constexpr float const s_two_pi = 2.0f * std::numbers::pi_v<float>;

	auto const now = std::chrono::high_resolution_clock::now();
	auto const diff = now - g_app_state->m_prev_time;
	float const diff_float_ms = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(diff).count();
	g_app_state->m_prev_time = now;

	g_app_state->m_rotation += diff_float_ms * s_rotation_speed;
	if(g_app_state->m_rotation >= s_two_pi)
	{
		g_app_state->m_rotation -= s_two_pi;
	}
	g_app_state->m_d3d11_transformations.m_world =XMMatrixRotationY(g_app_state->m_rotation);

	static constexpr float const s_background_color[4] = {0.0f, 0.125f, 0.6f, 1.0f};
	g_app_state->m_d3d11_immediate_context->ClearRenderTargetView(g_app_state->m_d3d11_render_target_view, s_background_color);

	my_constant_buffer_t my_constant_buffer;
	my_constant_buffer.m_world = XMMatrixTranspose(g_app_state->m_d3d11_transformations.m_world);
	my_constant_buffer.m_view = XMMatrixTranspose(g_app_state->m_d3d11_transformations.m_view);
	my_constant_buffer.m_projection = XMMatrixTranspose(g_app_state->m_d3d11_transformations.m_projection);
	g_app_state->m_d3d11_immediate_context->UpdateSubresource(g_app_state->m_d3d11_constant_buffer, 0, nullptr, &my_constant_buffer, 0, 0);

	g_app_state->m_d3d11_immediate_context->VSSetShader(g_app_state->m_d3d11_vertex_shader, nullptr, 0);
	g_app_state->m_d3d11_immediate_context->VSSetConstantBuffers(0, 1, &g_app_state->m_d3d11_constant_buffer);
	g_app_state->m_d3d11_immediate_context->PSSetShader(g_app_state->m_d3d11_pixel_shader, nullptr, 0);

	g_app_state->m_d3d11_immediate_context->DrawIndexed(36, 0, 0);

	HRESULT const presented = g_app_state->m_d3d11_swap_chain->Present(1, 0);
	CHECK_RET(presented == S_OK || presented == DXGI_STATUS_OCCLUDED, false);

	++g_app_state->m_fps_count;
	auto const long_term_diff = now - g_app_state->m_fps_time;
	if(long_term_diff >= std::chrono::seconds{5})
	{
		float const long_term_diff_float_s = std::chrono::duration_cast<std::chrono::duration<float>>(long_term_diff).count();
		std::printf("%f FPS. Presented %d frames in %f seconds.\n", static_cast<float>(g_app_state->m_fps_count) / long_term_diff_float_s, g_app_state->m_fps_count, long_term_diff_float_s);
		g_app_state->m_fps_time = now;
		g_app_state->m_fps_count = 0;
	}

	return true;
}

void network_thread_proc()
{
	static constexpr incomming_point_t const s_incomming_point{};
	std::fill(std::begin(g_app_state->m_incomming_points), std::end(g_app_state->m_incomming_points), s_incomming_point);
	g_app_state->m_incomming_points_idx = 0;

	WORD const wsa_version = MAKEWORD(2, 2);
	WSADATA wsa_data;
	int const wsa_started = WSAStartup(wsa_version, &wsa_data);
	CHECK_RET_V(wsa_started == 0);
	auto const wsa_free = mk::make_scope_exit([&](){ int const wsa_freed = WSACleanup(); CHECK_RET_V(wsa_freed == 0); });

	int const address_family = AF_INET;
	int const sck_type = SOCK_DGRAM;
	int const protocol = IPPROTO_UDP;
	SOCKET const sck = socket(address_family, sck_type, protocol);
	CHECK_RET_V(sck != INVALID_SOCKET);
	auto const sck_free = mk::make_scope_exit([&](){ int const sck_freed = closesocket(sck); CHECK_RET_V(sck_freed == 0); });

	DWORD const receieve_timeout_ms = 10;
	int const option_set = setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char const*>(&receieve_timeout_ms), static_cast<int>(sizeof(receieve_timeout_ms)));
	CHECK_RET_V(option_set == 0);

	static constexpr short const s_port_number = 2368;
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(s_port_number);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	int const bound = bind(sck, reinterpret_cast<sockaddr const*>(&addr), static_cast<int>(sizeof(addr)));
	CHECK_RET_V(bound == 0);

	while(g_app_state->m_thread_end_requested.load() == false)
	{
		unsigned char buff[64 * 1024];
		int const flags = 0;
		sockaddr_in other_addr;
		int other_addr_len = static_cast<int>(sizeof(other_addr));
		int const receieved = recvfrom(sck, reinterpret_cast<char*>(buff), static_cast<int>(std::size(buff)), flags, reinterpret_cast<sockaddr*>(&other_addr), &other_addr_len);
		if(receieved == 0)
		{
			break;
		}
		else if(receieved == SOCKET_ERROR)
		{
			int const err = WSAGetLastError();
			CHECK_RET_V(err == WSAETIMEDOUT);
			continue;
		}
		process_data(buff, receieved);
	}
}

void process_data(unsigned char const* const& data, int const& data_len)
{
	#pragma push_macro("CHECK_RET")
	#undef CHECK_RET
	#define CHECK_RET(X) do{ if(X){}else{ [[unlikely]] return; } }while(false)


	static constexpr int const s_lasers_count = 16;
	static constexpr int const s_firing_sequences_per_data_block_count = 2;
	static constexpr int const s_data_blocks_per_packet_count = 12;

	static constexpr std::uint8_t const s_vlp16_id = 0x22;
	static constexpr std::uint8_t const s_strongest_mode = 0x37;
	static constexpr std::uint8_t const s_last_mode = 0x38;
	static constexpr std::uint32_t const s_max_timestamp_incl = 3'599'999'999ull;
	static constexpr std::uint16_t const s_flag = 0xEEFF;
	static constexpr std::uint16_t const s_max_azimuth_incl = 35'999;

	static constexpr double const s_firing_sequence_len_us = 55.296; // us, including recharge time
	static constexpr double const s_firing_delay_us = 2.304; // us


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


	#pragma pack(push, 1)
	struct channel_data_t
	{
		std::uint16_t m_distance;
		std::uint8_t m_reflectivity;
	};
	#pragma pack(pop)
	static_assert(sizeof(channel_data_t) == 3);

	struct firing_sequence_t
	{
		channel_data_t m_channel_data[s_lasers_count];
	};
	static_assert(sizeof(firing_sequence_t) == 48);

	struct data_block_t
	{
		std::uint16_t m_flag;
		std::uint16_t m_azimuth;
		firing_sequence_t m_firing_sequence[s_firing_sequences_per_data_block_count];
	};
	static_assert(sizeof(data_block_t) == 100);

	struct factory_t
	{
		std::uint8_t m_return_mode;
		std::uint8_t m_product_id;
	};
	static_assert(sizeof(factory_t) == 2);

	#pragma pack(push, 1)
	struct single_mode_packet_t
	{
		data_block_t m_data_blocks[s_data_blocks_per_packet_count];
		std::uint32_t m_timestamp;
		factory_t m_factory;
	};
	#pragma pack(pop)
	static_assert(sizeof(single_mode_packet_t) == 1206);


	static constexpr auto const deg_to_rad = [](double const& deg) -> double { return deg * (std::numbers::pi_v<double> / 180.0); };
	static constexpr auto const rad_to_deg = [](double const& rad) -> double { return rad * (180.0 / std::numbers::pi_v<double>); };


	CHECK_RET(data_len == sizeof(single_mode_packet_t));
	single_mode_packet_t const& packet = *reinterpret_cast<single_mode_packet_t const*>(data);
	CHECK_RET(packet.m_factory.m_product_id == s_vlp16_id);
	CHECK_RET(packet.m_factory.m_return_mode == s_strongest_mode || packet.m_factory.m_return_mode == s_last_mode);
	CHECK_RET(packet.m_timestamp <= s_max_timestamp_incl);
	CHECK_RET(std::all_of(std::cbegin(packet.m_data_blocks), std::cend(packet.m_data_blocks), [](data_block_t const& data_block){ return data_block.m_flag == s_flag; }));
	CHECK_RET(std::all_of(std::cbegin(packet.m_data_blocks), std::cend(packet.m_data_blocks), [](data_block_t const& data_block){ return data_block.m_azimuth <= s_max_azimuth_incl; }));
	for(int data_block_idx = 0; data_block_idx != static_cast<int>(std::size(packet.m_data_blocks)); ++data_block_idx)
	{
		data_block_t const& data_block = packet.m_data_blocks[data_block_idx];
		std::uint16_t const azimuth_gap = [&]() -> std::uint16_t
		{
			if(data_block_idx == 0)
			{
				return 0;
			}
			else
			{
				data_block_t const& data_block_prev = packet.m_data_blocks[data_block_idx - 1];
				bool const wrap = data_block.m_azimuth - data_block_prev.m_azimuth < 0;
				std::uint16_t const azimuth_gap = !wrap ? data_block.m_azimuth - data_block_prev.m_azimuth : data_block.m_azimuth - data_block_prev.m_azimuth + 360 * 100;
				return azimuth_gap;
			}
		}();
		for(int firing_sequence_idx = 0; firing_sequence_idx != static_cast<int>(std::size(data_block.m_firing_sequence)); ++firing_sequence_idx)
		{
			firing_sequence_t const& firing_sequence = data_block.m_firing_sequence[firing_sequence_idx];
			for(int channel_data_idx = 0; channel_data_idx != static_cast<int>(std::size(firing_sequence.m_channel_data)); ++channel_data_idx)
			{
				channel_data_t const& channel_data = firing_sequence.m_channel_data[channel_data_idx];
				double const r = static_cast<double>(channel_data.m_distance * 2) / 1'000.0; // distance in meters
				//double const a = deg_to_rad(static_cast<double>(data_block.m_azimuth) / 100.0); // azimuth in radians
				double const azimuth_correction = (static_cast<double>(azimuth_gap) / 100.0) / (2.0 * s_firing_sequence_len_us) * (((firing_sequence_idx == 0) ? 0.0 : s_firing_sequence_len_us) + channel_data_idx * s_firing_delay_us); // azimuth correction in degrees
				double const pa = deg_to_rad(static_cast<double>(data_block.m_azimuth) / 100.0 + azimuth_correction); // precision azimuth in radians
				double const tmp = r * s_vertical_angle_cos[channel_data_idx];
				double const x = tmp * std::sin(pa); // x in meters
				double const y = tmp * std::cos(pa); // y in meters
				double const z = r * s_vertical_angle_sin[channel_data_idx] + s_vertical_correction_m[channel_data_idx]; // z in meters
				g_app_state->m_incomming_points[g_app_state->m_incomming_points_idx].m_azimuth = static_cast<float>(pa);
				g_app_state->m_incomming_points[g_app_state->m_incomming_points_idx].m_x = static_cast<float>(x);
				g_app_state->m_incomming_points[g_app_state->m_incomming_points_idx].m_y = static_cast<float>(y);
				g_app_state->m_incomming_points[g_app_state->m_incomming_points_idx].m_z = static_cast<float>(z);
				++g_app_state->m_incomming_points_idx;
			}
		}
	}

	#pragma pop_macro("CHECK_RET")
}
