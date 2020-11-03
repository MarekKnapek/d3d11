#include "scope_exit.h"

#include "mk_bit_utils.h"
#include "mk_counter.h"
#include "mk_utils.h"
#include "ring_buffer.h"
#include "vlp16.h"

#include <algorithm> // std::all_of, std::fill
#include <atomic>
#include <cassert>
#include <charconv> // std::from_chars
#include <chrono>
#include <cmath> // std::sin, std::cos
#include <condition_variable>
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t
#include <cstdio> // std::printf, std::puts
#include <cstdlib> // std::exit, EXIT_SUCCESS, EXIT_FAILURE
#include <cstring> // std::memcpy, std::strcmp, std::strncmp
#include <cwchar> // std::wcslen
#include <deque>
#include <iterator> // std::size, std::cbegin, std::cend, std::begin, std::end
#include <mutex>
#include <numbers> // std::numbers::pi_v
#include <queue>
#include <string> // std::u8string, std::u16string
#include <thread>
#include <vector>

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


static constexpr int const s_points_count = mk::equal_or_next_power_of_two(mk::vlp16::s_points_per_second);


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
};


struct my_constant_buffer_t
{
	XMMATRIX m_world;
	XMMATRIX m_view;
	XMMATRIX m_projection;
};


struct incomming_point_t
{
	double m_x;
	double m_y;
	double m_z;
};


#pragma warning(push)
#pragma warning(disable:4324) // warning C4324: 'frame_t': structure was padded due to alignment specifier
static constexpr int const s_frames_count = 3;
static constexpr int const s_vertices_per_cube = 8;
static constexpr int const s_indices_per_cube = 2 * 3 * 6;
struct alignas(256) frame_t
{
	float3_t m_vertices[s_points_count];
	unsigned m_count;
};
struct frames_t
{
	std::queue<std::unique_ptr<frame_t>> m_empty_frames;
	std::queue<std::unique_ptr<frame_t>> m_ready_frames;
	std::condition_variable m_cv;
	std::mutex m_mtx;
	std::atomic<bool> m_stop_requested;
};
frames_t g_frames;
void frames_thread_proc();
#pragma warning(pop)


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
	ID3D11Texture2D* m_d3d11_depth_buffer;
	ID3D11DepthStencilView* m_d3d11_stencil_view;
	ID3D11VertexShader* m_d3d11_vertex_shader;
	ID3D11PixelShader* m_d3d11_pixel_shader;
	ID3D11Buffer* m_d3d11_constant_buffer;
	float m_time;
	XMMATRIX m_world;
	XMMATRIX m_view;
	XMMATRIX m_projection;
	std::chrono::high_resolution_clock::time_point m_prev_time;
	mk::counter_t m_frames_counter;
	ID3D11Buffer* m_d3d11_vlp_vertex_buffer;
	ID3D11Buffer* m_d3d11_vlp_instance_buffer;
	ID3D11Buffer* m_d3d11_vlp_index_buffer;
	std::atomic<bool> m_thread_end_requested;
	std::mutex m_points_mutex;
	mk::counter_t m_packet_coutner;
	std::atomic<int> m_incomming_stuff_count;
	mk::ring_buffer_t<double, mk::equal_or_next_power_of_two(mk::vlp16::s_points_per_second)> m_incomming_azimuths;
	mk::ring_buffer_t<incomming_point_t, mk::equal_or_next_power_of_two(mk::vlp16::s_points_per_second)> m_incomming_points;
	std::unique_ptr<frame_t> m_last_frame;
	bool m_move_forward;
	bool m_move_backward;
	bool m_move_left;
	bool m_move_right;
	bool m_move_up;
	bool m_move_down;
	bool m_rotate_yaw_left;
	bool m_rotate_yaw_right;
	bool m_rotate_pitch_up;
	bool m_rotate_pitch_down;
	bool m_rotate_roll_left;
	bool m_rotate_roll_right;
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
	app_state_t* const app_state = new app_state_t{};
	auto const app_state_free = mk::make_scope_exit([&](){ delete g_app_state; });
	g_app_state = app_state;

	static constexpr char const s_param_help[] = "/?";
	static constexpr char const s_adapter[] = "/adapter";
	static constexpr char const s_param_ukn[] = "/ukn";
	static constexpr char const s_param_hw[] = "/hw";
	static constexpr char const s_param_ref[] = "/ref";
	static constexpr char const s_param_sw[] = "/sw";
	static constexpr char const s_param_warp[] = "/warp";
	static constexpr char const s_help_message[] =
		"Possible command line arguments:\n"
		"First argument:\n"
		"/? Prints this help.\n"
		"/adapter0 Uses default adapter.\n"
		"/adapter1 Uses 1st adapter.\n"
		"/adapter2 Uses 2nd adapter.\n"
		"/adapter3 Uses 3rd adapter. And so on.\n"
		"Second argument:\n"
		"/ukn Uses D3D_DRIVER_TYPE_UNKNOWN.\n"
		"/hw Uses D3D_DRIVER_TYPE_HARDWARE.\n"
		"/ref Uses D3D_DRIVER_TYPE_REFERENCE.\n"
		"/sw Uses D3D_DRIVER_TYPE_SOFTWARE.\n"
		"/warp Uses D3D_DRIVER_TYPE_WARP.\n"
		"Default is /adapter0 /hw.";
	enum class driver_type_e
	{
		ukn,
		hw,
		ref,
		sw,
		warp,
	};
	static constexpr auto const s_driver_type_to_win_enum = [](driver_type_e const& driver_type) -> D3D_DRIVER_TYPE
	{
		switch(driver_type)
		{
			case driver_type_e::ukn: return D3D_DRIVER_TYPE_UNKNOWN;
			case driver_type_e::hw: return D3D_DRIVER_TYPE_HARDWARE;
			case driver_type_e::ref: return D3D_DRIVER_TYPE_REFERENCE;
			case driver_type_e::sw: return D3D_DRIVER_TYPE_SOFTWARE;
			case driver_type_e::warp: return D3D_DRIVER_TYPE_WARP;
		}
		CHECK_RET_V(false);
	};

	CHECK_RET(argc >= 1 && argc <= 3, false);
	int adapter_idx = 0;
	driver_type_e driver_type = driver_type_e::hw;
	if(argc == 2 && std::strcmp(argv[1], s_param_help) == 0)
	{
		std::puts(s_help_message);
		return true;
	}
	if(argc >= 2)
	{
		if(std::strncmp(argv[1], s_adapter, std::size(s_adapter) - 1) == 0)
		{
			int const len =  static_cast<int>(std::strlen(argv[1]));
			auto const parsed = std::from_chars(argv[1] +  std::size(s_adapter) - 1, argv[1] + len, adapter_idx, 10);
			CHECK_RET(parsed.ec == std::errc{}, false);
			CHECK_RET(parsed.ptr == argv[1] + len, false);
		}
		else
		{
			CHECK_RET(false, false);
		}
	}
	if(argc == 3)
	{
		if(std::strcmp(argv[2], s_param_ukn) == 0)
		{
			driver_type = driver_type_e::ukn;
		}
		else if(std::strcmp(argv[2], s_param_hw) == 0)
		{
			driver_type = driver_type_e::hw;
		}
		else if(std::strcmp(argv[2], s_param_ref) == 0)
		{
			driver_type = driver_type_e::ref;
		}
		else if(std::strcmp(argv[2], s_param_sw) == 0)
		{
			driver_type = driver_type_e::sw;
		}
		else if(std::strcmp(argv[2], s_param_warp) == 0)
		{
			driver_type = driver_type_e::warp;
		}
		else
		{
			CHECK_RET(false, false);
		}
	}

	HRESULT const com_initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
	CHECK_RET(com_initialized == S_OK, false);
	auto const com_uninitialize = mk::make_scope_exit([](){ CoUninitialize(); });

	bool const registered = register_main_window_class(&g_app_state->m_main_window_class);
	CHECK_RET(registered, false);
	auto const unregister = mk::make_scope_exit([&](){ bool const unregistered = unregister_main_window_class(g_app_state->m_main_window_class); CHECK_RET_V(unregistered); });

	bool const window_created = create_main_window(g_app_state->m_main_window_class, &g_app_state->m_main_window);
	CHECK_RET(window_created, false);
	auto const window_free = mk::make_scope_exit([&](){ if(g_app_state->m_main_window != nullptr){ BOOL const destroyed = DestroyWindow(g_app_state->m_main_window); CHECK_RET_V(destroyed != 0); } });

	bool const window_size_refreshed = refresh_window_size(g_app_state->m_main_window, &g_app_state->m_width, &g_app_state->m_height);
	CHECK_RET(window_size_refreshed, false);

	/* D3D11 */
	bool const d3d11_version_check = !FAILED(D3DX11CheckVersion(D3D11_SDK_VERSION, D3DX11_SDK_VERSION));
	CHECK_RET(d3d11_version_check, false);

	IDXGIFactory1* d3d11_factory;
	HRESULT const d311_factory_created =  CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void**>(&d3d11_factory));
	CHECK_RET(d311_factory_created == S_OK, false);
	auto const d3d11_factory_free = mk::make_scope_exit([&](){ d3d11_factory->Release(); });

	IDXGIAdapter* d3d11_selected_adapter = nullptr;
	auto const d3d11_selected_adapter_free = mk::make_scope_exit([&](){ if(d3d11_selected_adapter){ d3d11_selected_adapter->Release(); } });
	for(int d3d11_adapter_idx = 0;; ++d3d11_adapter_idx)
	{
		IDXGIAdapter1* d3d11_adapter;
		HRESULT const d3d11_adapters_enumed = d3d11_factory->EnumAdapters1(static_cast<UINT>(d3d11_adapter_idx), &d3d11_adapter);
		if(d3d11_adapters_enumed == DXGI_ERROR_NOT_FOUND) break;
		CHECK_RET(d3d11_adapters_enumed == S_OK, false);
		auto const d3d11_adapter_free = mk::make_scope_exit([&](){ d3d11_adapter->Release(); });

		if(adapter_idx != 0 && adapter_idx - 1 == d3d11_adapter_idx)
		{
			d3d11_selected_adapter = d3d11_adapter;
			d3d11_selected_adapter->AddRef();
		}

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
	CHECK_RET((adapter_idx != 0 && d3d11_selected_adapter != nullptr) ||(adapter_idx == 0), false);

	ID3D11Device* d3d11_device;
	D3D_FEATURE_LEVEL d3d11_feature_level;
	ID3D11DeviceContext* d3d11_immediate_context;
	UINT d3d11_device_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
	#ifndef NDEBUG
	d3d11_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	HRESULT const d3d11_device_created = D3D11CreateDevice(d3d11_selected_adapter, s_driver_type_to_win_enum(driver_type), nullptr, d3d11_device_flags, nullptr, 0, D3D11_SDK_VERSION, &d3d11_device, &d3d11_feature_level, &d3d11_immediate_context);
	CHECK_RET(d3d11_device_created == S_OK, false);
	auto const d3d11_device_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_device->Release(); g_app_state->m_d3d11_device = nullptr; });
	#ifndef NDEBUG
	auto const d3d11_debug = mk::make_scope_exit([&]()
	{
		ID3D11Debug* d3d11_debug;
		HRESULT const d3d11_debug_casted = d3d11_device->QueryInterface(IID_ID3D11Debug, reinterpret_cast<void**>(&d3d11_debug));
		CHECK_RET_V(d3d11_debug_casted == S_OK);
		auto const d3d11_debug_free = mk::make_scope_exit([&](){ d3d11_debug->Release(); });
		HRESULT const d3d11_debug_live_reported = d3d11_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		CHECK_RET_V(d3d11_debug_live_reported == S_OK);
	});
	#endif
	auto const d3d11_immediate_context_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_immediate_context->ClearState(); g_app_state->m_d3d11_immediate_context->Flush(); g_app_state->m_d3d11_immediate_context->Release(); g_app_state->m_d3d11_immediate_context = nullptr; });
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
	auto const d3d11_swap_chain_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_swap_chain->Release(); g_app_state->m_d3d11_swap_chain = nullptr; });
	g_app_state->m_d3d11_swap_chain = d3d11_swap_chain;

	ID3D11Texture2D* d3d11_back_buffer;
	HRESULT const d3d11_got_back_buffer = d3d11_swap_chain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void**>(&d3d11_back_buffer));
	CHECK_RET(d3d11_got_back_buffer == S_OK, false);
	auto const back_buffer_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_back_buffer->Release(); g_app_state->m_d3d11_back_buffer = nullptr; });
	g_app_state->m_d3d11_back_buffer = d3d11_back_buffer;

	ID3D11RenderTargetView* d3d11_render_target_view;
	HRESULT const d3d11_render_target_view_created = d3d11_device->CreateRenderTargetView(d3d11_back_buffer, nullptr, &d3d11_render_target_view);
	CHECK_RET(d3d11_render_target_view_created == S_OK, false);
	auto const d3d11_render_target_view_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_render_target_view->Release(); g_app_state->m_d3d11_render_target_view = nullptr; });
	g_app_state->m_d3d11_render_target_view = d3d11_render_target_view;

	ID3D11Texture2D* d3d11_depth_buffer;
	D3D11_TEXTURE2D_DESC d3d11_depth_buffer_description;
	d3d11_depth_buffer_description.Width = g_app_state->m_width;
	d3d11_depth_buffer_description.Height = g_app_state->m_height;
	d3d11_depth_buffer_description.MipLevels = 1;
	d3d11_depth_buffer_description.ArraySize = 1;
	d3d11_depth_buffer_description.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3d11_depth_buffer_description.SampleDesc.Count = 1;
	d3d11_depth_buffer_description.SampleDesc.Quality = 0;
	d3d11_depth_buffer_description.Usage = D3D11_USAGE_DEFAULT;
	d3d11_depth_buffer_description.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	d3d11_depth_buffer_description.CPUAccessFlags = 0;
	d3d11_depth_buffer_description.MiscFlags = 0;
	HRESULT const d3d11_depth_buffer_created = g_app_state->m_d3d11_device->CreateTexture2D(&d3d11_depth_buffer_description, nullptr, &d3d11_depth_buffer);
	CHECK_RET(d3d11_depth_buffer_created == S_OK, false);
	auto const d3d11_depth_buffer_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_depth_buffer->Release(); g_app_state->m_d3d11_depth_buffer = nullptr; });
	g_app_state->m_d3d11_depth_buffer = d3d11_depth_buffer;

	ID3D11DepthStencilView* d3d11_stencil_view;
	D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_stencil_view_description;
	d3d11_stencil_view_description.Format = d3d11_depth_buffer_description.Format;
	d3d11_stencil_view_description.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	d3d11_stencil_view_description.Flags = 0;
	d3d11_stencil_view_description.Texture2D.MipSlice = 0;
	HRESULT const d3d11_stencil_view_created = g_app_state->m_d3d11_device->CreateDepthStencilView(g_app_state->m_d3d11_depth_buffer, &d3d11_stencil_view_description, &d3d11_stencil_view);
	CHECK_RET(d3d11_stencil_view_created == S_OK, false);
	auto const d3d11_stencil_view_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_stencil_view->Release(); g_app_state->m_d3d11_stencil_view = nullptr; });
	g_app_state->m_d3d11_stencil_view = d3d11_stencil_view;

	d3d11_immediate_context->OMSetRenderTargets(1, &g_app_state->m_d3d11_render_target_view, g_app_state->m_d3d11_stencil_view);

	ID3D11RasterizerState* d3d11_rasterizer;
	D3D11_RASTERIZER_DESC d3d11_rasterizer_description;
	d3d11_rasterizer_description.FillMode = D3D11_FILL_SOLID;
	d3d11_rasterizer_description.CullMode = D3D11_CULL_BACK;
	d3d11_rasterizer_description.FrontCounterClockwise = FALSE;
	d3d11_rasterizer_description.DepthBias = 0;
	d3d11_rasterizer_description.DepthBiasClamp = 0.0f;
	d3d11_rasterizer_description.SlopeScaledDepthBias = 0.0f;
	d3d11_rasterizer_description.DepthClipEnable = TRUE;
	d3d11_rasterizer_description.ScissorEnable = FALSE;
	d3d11_rasterizer_description.MultisampleEnable = FALSE;
	d3d11_rasterizer_description.AntialiasedLineEnable = FALSE;
	HRESULT const d3d11_rasterizer_created = g_app_state->m_d3d11_device->CreateRasterizerState(&d3d11_rasterizer_description, &d3d11_rasterizer);
	CHECK_RET(d3d11_rasterizer_created == S_OK, false);
	auto const d3d11_rasterizer_free = mk::make_scope_exit([&](){ d3d11_rasterizer->Release(); });

	g_app_state->m_d3d11_immediate_context->RSSetState(d3d11_rasterizer);

	D3D11_VIEWPORT d3d11_view_port;
	d3d11_view_port.TopLeftX = 0.0f;
	d3d11_view_port.TopLeftY = 0.0f;
	d3d11_view_port.Width = static_cast<float>(g_app_state->m_width);
	d3d11_view_port.Height = static_cast<float>(g_app_state->m_height);
	d3d11_view_port.MinDepth = 0.0f;
	d3d11_view_port.MaxDepth = 1.0f;
	g_app_state->m_d3d11_immediate_context->RSSetViewports(1, &d3d11_view_port);

	ID3D11VertexShader* d3d11_vertex_shader;
	HRESULT const d3d11_vertex_shader_created = g_app_state->m_d3d11_device->CreateVertexShader(g_vertex_shader_main, std::size(g_vertex_shader_main), nullptr, &d3d11_vertex_shader);
	CHECK_RET(d3d11_vertex_shader_created == S_OK, false);
	auto const d3d11_vertex_shader_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_vertex_shader->Release(); g_app_state->m_d3d11_vertex_shader = nullptr; });
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
	d3d11_shader_imput_layout[1].SemanticName = "TEXCOORD";
	d3d11_shader_imput_layout[1].SemanticIndex = 0;
	d3d11_shader_imput_layout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	d3d11_shader_imput_layout[1].InputSlot = 1;
	d3d11_shader_imput_layout[1].AlignedByteOffset = 0;
	d3d11_shader_imput_layout[1].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
	d3d11_shader_imput_layout[1].InstanceDataStepRate = 1;
	HRESULT const d3d11_input_layout_created = g_app_state->m_d3d11_device->CreateInputLayout(d3d11_shader_imput_layout, static_cast<int>(std::size(d3d11_shader_imput_layout)), g_vertex_shader_main, std::size(g_vertex_shader_main), &d3d11_input_layout);
	CHECK_RET(d3d11_input_layout_created == S_OK, false);
	auto const d3d11_input_layout_free = mk::make_scope_exit([&](){ d3d11_input_layout->Release(); });

	g_app_state->m_d3d11_immediate_context->IASetInputLayout(d3d11_input_layout);

	ID3D11PixelShader* d3d11_pixel_shader;
	HRESULT const d3d11_pixel_shader_created = g_app_state->m_d3d11_device->CreatePixelShader(g_pixel_shader_main, std::size(g_pixel_shader_main), nullptr, &d3d11_pixel_shader);
	CHECK_RET(d3d11_pixel_shader_created == S_OK, false);
	auto const d3d11_pixel_shader_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_pixel_shader->Release(); g_app_state->m_d3d11_pixel_shader = nullptr; });
	g_app_state->m_d3d11_pixel_shader = d3d11_pixel_shader;

	static constexpr my_vertex_t const s_cube_vertex_pattern[] =
	{
		{-1.0f, +1.0f, +1.0f}, /* right, up, front */
		{-1.0f, -1.0f, +1.0f}, /* right, down, front */
		{+1.0f, -1.0f, +1.0f}, /* left, down, front */
		{+1.0f, +1.0f, +1.0f}, /* left, up, front */
		{-1.0f, +1.0f, -1.0f}, /* right, up, back */
		{-1.0f, -1.0f, -1.0f}, /* right, down, back */
		{+1.0f, -1.0f, -1.0f}, /* left, down, back */
		{+1.0f, +1.0f, -1.0f}, /* left, up, back */
	};
	static_assert(std::size(s_cube_vertex_pattern) == s_vertices_per_cube);

	ID3D11Buffer* d3d11_vlp_vertex_buffer;
	D3D11_BUFFER_DESC d3d11_vlp_vertex_buffer_description;
	d3d11_vlp_vertex_buffer_description.ByteWidth = s_vertices_per_cube * sizeof(my_vertex_t);
	d3d11_vlp_vertex_buffer_description.Usage = D3D11_USAGE_IMMUTABLE;
	d3d11_vlp_vertex_buffer_description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	d3d11_vlp_vertex_buffer_description.CPUAccessFlags = 0;
	d3d11_vlp_vertex_buffer_description.MiscFlags = 0;
	d3d11_vlp_vertex_buffer_description.StructureByteStride = 0;
	my_vertex_t cube_vertices[std::size(s_cube_vertex_pattern)];
	std::memcpy(cube_vertices, s_cube_vertex_pattern, sizeof(s_cube_vertex_pattern));
	static constexpr float const s_cube_size = 0.005f;
	for(auto& my_vertex : cube_vertices){ my_vertex.m_position.m_x *= s_cube_size; my_vertex.m_position.m_y *= s_cube_size; my_vertex.m_position.m_z *= s_cube_size; }
	D3D11_SUBRESOURCE_DATA d3d11_vlp_vertex_buffer_resource_data;
	d3d11_vlp_vertex_buffer_resource_data.pSysMem = cube_vertices;
	d3d11_vlp_vertex_buffer_resource_data.SysMemPitch = 0;
	d3d11_vlp_vertex_buffer_resource_data.SysMemSlicePitch = 0;
	HRESULT const d3d11_vlp_vertex_buffer_created = g_app_state->m_d3d11_device->CreateBuffer(&d3d11_vlp_vertex_buffer_description, &d3d11_vlp_vertex_buffer_resource_data, &d3d11_vlp_vertex_buffer);
	CHECK_RET(d3d11_vlp_vertex_buffer_created == S_OK, false);
	g_app_state->m_d3d11_vlp_vertex_buffer = d3d11_vlp_vertex_buffer;
	auto const d3d11_vlp_vertex_buffer_free = mk::make_scope_exit([](){ g_app_state->m_d3d11_vlp_vertex_buffer->Release(); g_app_state->m_d3d11_vlp_vertex_buffer = nullptr; });

	ID3D11Buffer* d3d11_vlp_instance_buffer;
	D3D11_BUFFER_DESC d3d11_vlp_instance_buffer_description;
	d3d11_vlp_instance_buffer_description.ByteWidth = s_points_count * sizeof(float3_t);
	d3d11_vlp_instance_buffer_description.Usage = D3D11_USAGE_DYNAMIC;
	d3d11_vlp_instance_buffer_description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	d3d11_vlp_instance_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	d3d11_vlp_instance_buffer_description.MiscFlags = 0;
	d3d11_vlp_instance_buffer_description.StructureByteStride = 0;
	HRESULT const d3d11_vlp_instance_buffer_created = g_app_state->m_d3d11_device->CreateBuffer(&d3d11_vlp_instance_buffer_description, nullptr, &d3d11_vlp_instance_buffer);
	CHECK_RET(d3d11_vlp_instance_buffer_created == S_OK, false);
	g_app_state->m_d3d11_vlp_instance_buffer = d3d11_vlp_instance_buffer;
	auto const d3d11_vlp_instance_buffer_free = mk::make_scope_exit([](){ g_app_state->m_d3d11_vlp_instance_buffer->Release(); g_app_state->m_d3d11_vlp_instance_buffer = nullptr; });

	UINT const start_slot = 0;
	ID3D11Buffer* d3d11_vertex_buffers[] = {g_app_state->m_d3d11_vlp_vertex_buffer, g_app_state->m_d3d11_vlp_instance_buffer};
	UINT const num_buffers = static_cast<int>(std::size(d3d11_vertex_buffers));
	UINT const d3d11_vertex_buffer_strides[] = {sizeof(my_vertex_t), sizeof(float3_t)};
	static_assert(std::size(d3d11_vertex_buffer_strides) == std::size(d3d11_vertex_buffers));
	UINT const d3d11_vertex_buffer_offsets[] = {0, 0};
	static_assert(std::size(d3d11_vertex_buffer_offsets) == std::size(d3d11_vertex_buffers));
	g_app_state->m_d3d11_immediate_context->IASetVertexBuffers(start_slot, num_buffers, d3d11_vertex_buffers, d3d11_vertex_buffer_strides, d3d11_vertex_buffer_offsets);

	static constexpr std::uint16_t const s_cube_index_pattern[] =
	{
		// front
		0, 1, 3,
		3, 1, 2,
		// back
		4, 5, 7,
		7, 5, 6,
		// left
		3, 2, 4,
		4, 2, 5,
		// right
		7, 6, 0,
		0, 6, 1,
		// top
		7, 0, 4,
		4, 0, 3,
		// bottom
		1, 6, 2,
		2, 6, 5,
	};
	static_assert(std::size(s_cube_index_pattern) == s_indices_per_cube);

	ID3D11Buffer* d3d11_vlp_index_buffer;
	D3D11_BUFFER_DESC d3d11_vlp_index_buffer_description;
	d3d11_vlp_index_buffer_description.ByteWidth = s_indices_per_cube * sizeof(std::uint16_t);
	d3d11_vlp_index_buffer_description.Usage = D3D11_USAGE_IMMUTABLE;
	d3d11_vlp_index_buffer_description.BindFlags = D3D11_BIND_INDEX_BUFFER;
	d3d11_vlp_index_buffer_description.CPUAccessFlags = 0;
	d3d11_vlp_index_buffer_description.MiscFlags = 0;
	d3d11_vlp_index_buffer_description.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA d3d11_vlp_index_buffer_resource_data;
	d3d11_vlp_index_buffer_resource_data.pSysMem = s_cube_index_pattern;
	d3d11_vlp_index_buffer_resource_data.SysMemPitch = 0;
	d3d11_vlp_index_buffer_resource_data.SysMemSlicePitch = 0;
	HRESULT const d3d11_vlp_index_buffer_created = g_app_state->m_d3d11_device->CreateBuffer(&d3d11_vlp_index_buffer_description, &d3d11_vlp_index_buffer_resource_data, &d3d11_vlp_index_buffer);
	CHECK_RET(d3d11_vlp_index_buffer_created == S_OK, false);
	g_app_state->m_d3d11_vlp_index_buffer = d3d11_vlp_index_buffer;
	auto const d3d11_vlp_index_buffer_free = mk::make_scope_exit([](){ g_app_state->m_d3d11_vlp_index_buffer->Release(); g_app_state->m_d3d11_vlp_index_buffer = nullptr; });

	g_app_state->m_d3d11_immediate_context->IASetIndexBuffer(g_app_state->m_d3d11_vlp_index_buffer, DXGI_FORMAT_R16_UINT, 0);

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
	auto const d3d11_constant_buffer_free = mk::make_scope_exit([](){ g_app_state->m_d3d11_constant_buffer->Release(); g_app_state->m_d3d11_constant_buffer = nullptr; });
	g_app_state->m_d3d11_constant_buffer = d3d11_constant_buffer;

	g_app_state->m_time = 0.0f;

	g_app_state->m_world = XMMatrixIdentity();

	XMVECTOR const d3d11_eye = {0.0f, 1.0f, -5.0f, 0.0f};
	XMVECTOR const d3d11_at = {0.0f, 1.0f, 0.0f, 0.0f};
	XMVECTOR const d3d11_up = {0.0f, 1.0f, 0.0f, 0.0f};
	g_app_state->m_view = XMMatrixLookAtLH(d3d11_eye, d3d11_at, d3d11_up);

	g_app_state->m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(g_app_state->m_width) / static_cast<float>(g_app_state->m_height), 0.01f, 100.0f);

	g_app_state->m_world = XMMatrixIdentity();

	my_constant_buffer_t my_constant_buffer;
	my_constant_buffer.m_world = XMMatrixTranspose(g_app_state->m_world);
	my_constant_buffer.m_view = XMMatrixTranspose(g_app_state->m_view);
	my_constant_buffer.m_projection = XMMatrixTranspose(g_app_state->m_projection);
	g_app_state->m_d3d11_immediate_context->UpdateSubresource(g_app_state->m_d3d11_constant_buffer, 0, nullptr, &my_constant_buffer, 0, 0);

	g_app_state->m_d3d11_immediate_context->VSSetShader(g_app_state->m_d3d11_vertex_shader, nullptr, 0);
	g_app_state->m_d3d11_immediate_context->VSSetConstantBuffers(0, 1, &g_app_state->m_d3d11_constant_buffer);
	g_app_state->m_d3d11_immediate_context->PSSetShader(g_app_state->m_d3d11_pixel_shader, nullptr, 0);

	for(int i = 0; i != s_frames_count; ++i)
	{
		g_frames.m_empty_frames.push(std::make_unique<frame_t>());
	}
	g_frames.m_stop_requested.store(false);
	std::thread frames_thread{&frames_thread_proc};
	auto const frames_thread_free = mk::make_scope_exit([&]()
	{
		g_frames.m_stop_requested.store(true);
		g_frames.m_cv.notify_one();
		frames_thread.join();
	});
	/* D3D11 */

	g_app_state->m_thread_end_requested.store(false);
	std::thread network_thread{&network_thread_proc};
	auto const network_thread_free = mk::make_scope_exit([&]()
	{
		g_app_state->m_thread_end_requested.store(true);
		network_thread.join();
	});

	g_app_state->m_prev_time = std::chrono::high_resolution_clock::now();
	g_app_state->m_frames_counter.rename("frames");
	g_app_state->m_packet_coutner.rename("packets");

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
			g_app_state->m_main_window = nullptr;
			PostQuitMessage(EXIT_SUCCESS);
		}
		break;
		case WM_SIZE:
		{
			bool const window_size_refreshed = refresh_window_size(g_app_state->m_main_window, &g_app_state->m_width, &g_app_state->m_height);
			CHECK_RET_V(window_size_refreshed);
			if(g_app_state->m_width != 0 && g_app_state->m_height != 0)
			{
				g_app_state->m_d3d11_immediate_context->OMSetRenderTargets(0, nullptr, nullptr);
				g_app_state->m_d3d11_stencil_view->Release(); g_app_state->m_d3d11_stencil_view = nullptr;
				g_app_state->m_d3d11_depth_buffer->Release(); g_app_state->m_d3d11_depth_buffer = nullptr;
				g_app_state->m_d3d11_render_target_view->Release(); g_app_state->m_d3d11_render_target_view = nullptr;
				g_app_state->m_d3d11_back_buffer->Release(); g_app_state->m_d3d11_back_buffer = nullptr;

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

				ID3D11Texture2D* d3d11_depth_buffer;
				D3D11_TEXTURE2D_DESC d3d11_depth_buffer_description;
				d3d11_depth_buffer_description.Width = g_app_state->m_width;
				d3d11_depth_buffer_description.Height = g_app_state->m_height;
				d3d11_depth_buffer_description.MipLevels = 1;
				d3d11_depth_buffer_description.ArraySize = 1;
				d3d11_depth_buffer_description.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				d3d11_depth_buffer_description.SampleDesc.Count = 1;
				d3d11_depth_buffer_description.SampleDesc.Quality = 0;
				d3d11_depth_buffer_description.Usage = D3D11_USAGE_DEFAULT;
				d3d11_depth_buffer_description.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				d3d11_depth_buffer_description.CPUAccessFlags = 0;
				d3d11_depth_buffer_description.MiscFlags = 0;
				HRESULT const d3d11_depth_buffer_created = g_app_state->m_d3d11_device->CreateTexture2D(&d3d11_depth_buffer_description, nullptr, &d3d11_depth_buffer);
				CHECK_RET_V(d3d11_depth_buffer_created == S_OK);
				g_app_state->m_d3d11_depth_buffer = d3d11_depth_buffer;

				ID3D11DepthStencilView* d3d11_stencil_view;
				D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_stencil_view_description;
				d3d11_stencil_view_description.Format = d3d11_depth_buffer_description.Format;
				d3d11_stencil_view_description.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				d3d11_stencil_view_description.Flags = 0;
				d3d11_stencil_view_description.Texture2D.MipSlice = 0;
				HRESULT const d3d11_stencil_view_created = g_app_state->m_d3d11_device->CreateDepthStencilView(g_app_state->m_d3d11_depth_buffer, &d3d11_stencil_view_description, &d3d11_stencil_view);
				CHECK_RET_V(d3d11_stencil_view_created == S_OK);
				g_app_state->m_d3d11_stencil_view = d3d11_stencil_view;

				g_app_state->m_d3d11_immediate_context->OMSetRenderTargets(1, &g_app_state->m_d3d11_render_target_view, g_app_state->m_d3d11_stencil_view);

				D3D11_VIEWPORT d3d11_view_port;
				d3d11_view_port.TopLeftX = 0.0f;
				d3d11_view_port.TopLeftY = 0.0f;
				d3d11_view_port.Width = static_cast<float>(g_app_state->m_width);
				d3d11_view_port.Height = static_cast<float>(g_app_state->m_height);
				d3d11_view_port.MinDepth = 0.0f;
				d3d11_view_port.MaxDepth = 1.0f;
				g_app_state->m_d3d11_immediate_context->RSSetViewports(1, &d3d11_view_port);

				g_app_state->m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(g_app_state->m_width) / static_cast<float>(g_app_state->m_height), 0.01f, 100.0f);
			}
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
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			bool const is_pressed = msg == WM_KEYDOWN;
			static constexpr WPARAM const s_move_forward[] = {'W'};
			static constexpr WPARAM const s_move_backward[] = {'S'};
			static constexpr WPARAM const s_move_left[] = {'A'};
			static constexpr WPARAM const s_move_right[] = {'D'};
			static constexpr WPARAM const s_move_up[] = {'E'};
			static constexpr WPARAM const s_move_down[] = {'Q'};
			static constexpr WPARAM const s_rotate_yaw_left[] = {'J'};
			static constexpr WPARAM const s_rotate_yaw_right[] = {'L'};
			static constexpr WPARAM const s_rotate_pitch_up[] = {'I'};
			static constexpr WPARAM const s_rotate_pitch_down[] = {'K'};
			static constexpr WPARAM const s_rotate_roll_left[] = {'U'};
			static constexpr WPARAM const s_rotate_roll_right[] = {'O'};
			static constexpr WPARAM const s_reset[] = {'R'};
			app_state_t& app_state = *g_app_state;
			if(std::find(std::cbegin(s_move_forward), std::cend(s_move_forward), w_param) != std::cend(s_move_forward)) app_state.m_move_forward = is_pressed;
			if(std::find(std::cbegin(s_move_backward), std::cend(s_move_backward), w_param) != std::cend(s_move_backward)) app_state.m_move_backward = is_pressed;
			if(std::find(std::cbegin(s_move_left), std::cend(s_move_left), w_param) != std::cend(s_move_left)) app_state.m_move_left = is_pressed;
			if(std::find(std::cbegin(s_move_right), std::cend(s_move_right), w_param) != std::cend(s_move_right)) app_state.m_move_right = is_pressed;
			if(std::find(std::cbegin(s_move_up), std::cend(s_move_up), w_param) != std::cend(s_move_up)) app_state.m_move_up = is_pressed;
			if(std::find(std::cbegin(s_move_down), std::cend(s_move_down), w_param) != std::cend(s_move_down)) app_state.m_move_down = is_pressed;
			if(std::find(std::cbegin(s_rotate_yaw_left), std::cend(s_rotate_yaw_left), w_param) != std::cend(s_rotate_yaw_left)) app_state.m_rotate_yaw_left = is_pressed;
			if(std::find(std::cbegin(s_rotate_yaw_right), std::cend(s_rotate_yaw_right), w_param) != std::cend(s_rotate_yaw_right)) app_state.m_rotate_yaw_right = is_pressed;
			if(std::find(std::cbegin(s_rotate_pitch_up), std::cend(s_rotate_pitch_up), w_param) != std::cend(s_rotate_pitch_up)) app_state.m_rotate_pitch_up = is_pressed;
			if(std::find(std::cbegin(s_rotate_pitch_down), std::cend(s_rotate_pitch_down), w_param) != std::cend(s_rotate_pitch_down)) app_state.m_rotate_pitch_down = is_pressed;
			if(std::find(std::cbegin(s_rotate_roll_left), std::cend(s_rotate_roll_left), w_param) != std::cend(s_rotate_roll_left)) app_state.m_rotate_roll_left = is_pressed;
			if(std::find(std::cbegin(s_rotate_roll_right), std::cend(s_rotate_roll_right), w_param) != std::cend(s_rotate_roll_right)) app_state.m_rotate_roll_right = is_pressed;
			if(is_pressed && std::find(std::cbegin(s_reset), std::cend(s_reset), w_param) != std::cend(s_reset))
			{
				XMVECTOR const d3d11_eye = {0.0f, 1.0f, -5.0f, 0.0f};
				XMVECTOR const d3d11_at = {0.0f, 1.0f, 0.0f, 0.0f};
				XMVECTOR const d3d11_up = {0.0f, 1.0f, 0.0f, 0.0f};
				app_state.m_view = XMMatrixLookAtLH(d3d11_eye, d3d11_at, d3d11_up);
			}
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
	static constexpr float const s_speed = 0.001f;
	static constexpr float const s_two_pi = 2.0f * std::numbers::pi_v<float>;

	auto const now = std::chrono::high_resolution_clock::now();
	auto const diff = now - g_app_state->m_prev_time;
	float const diff_float_ms = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(diff).count();
	g_app_state->m_prev_time = now;

	g_app_state->m_time += diff_float_ms * s_speed;
	if(g_app_state->m_time >= s_two_pi)
	{
		g_app_state->m_time -= s_two_pi;
	}

	float const move_speed = diff_float_ms / 100.0f;
	float const rotate_speed = diff_float_ms / 500.0f;
	app_state_t& app_state = *g_app_state;
	if(app_state.m_move_forward) app_state.m_view = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, -move_speed), app_state.m_view);
	if(app_state.m_move_backward) app_state.m_view = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, +move_speed), app_state.m_view);
	if(app_state.m_move_left) app_state.m_view = XMMatrixMultiply(XMMatrixTranslation(+move_speed, 0.0f, 0.0f), app_state.m_view);
	if(app_state.m_move_right) app_state.m_view = XMMatrixMultiply(XMMatrixTranslation(-move_speed, 0.0f, 0.0f), app_state.m_view);
	if(app_state.m_move_up) app_state.m_view = XMMatrixMultiply(XMMatrixTranslation(0.0f, -move_speed, 0.0f), app_state.m_view);
	if(app_state.m_move_down) app_state.m_view = XMMatrixMultiply(XMMatrixTranslation(0.0f, +move_speed, 0.0f), app_state.m_view);
	if(app_state.m_rotate_yaw_left) app_state.m_view = XMMatrixMultiply(XMMatrixRotationY(-rotate_speed), app_state.m_view);
	if(app_state.m_rotate_yaw_right) app_state.m_view = XMMatrixMultiply(XMMatrixRotationY(+rotate_speed), app_state.m_view);
	if(app_state.m_rotate_pitch_up) app_state.m_view = XMMatrixMultiply(XMMatrixRotationX(-rotate_speed), app_state.m_view);
	if(app_state.m_rotate_pitch_down) app_state.m_view = XMMatrixMultiply(XMMatrixRotationX(+rotate_speed), app_state.m_view);
	if(app_state.m_rotate_roll_left) app_state.m_view = XMMatrixMultiply(XMMatrixRotationZ(+rotate_speed), app_state.m_view);
	if(app_state.m_rotate_roll_right) app_state.m_view = XMMatrixMultiply(XMMatrixRotationZ(-rotate_speed), app_state.m_view);

	my_constant_buffer_t my_constant_buffer;
	my_constant_buffer.m_world = XMMatrixTranspose(app_state.m_world);
	my_constant_buffer.m_view = XMMatrixTranspose(app_state.m_view);
	my_constant_buffer.m_projection = XMMatrixTranspose(app_state.m_projection);
	g_app_state->m_d3d11_immediate_context->UpdateSubresource(app_state.m_d3d11_constant_buffer, 0, nullptr, &my_constant_buffer, 0, 0);
	g_app_state->m_d3d11_immediate_context->VSSetConstantBuffers(0, 1, &g_app_state->m_d3d11_constant_buffer);

	static constexpr float const s_background_color[4] = {0.0f, 0.125f, 0.6f, 1.0f};
	g_app_state->m_d3d11_immediate_context->ClearRenderTargetView(g_app_state->m_d3d11_render_target_view, s_background_color);

	g_app_state->m_d3d11_immediate_context->ClearDepthStencilView(g_app_state->m_d3d11_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);

	do
	{
		std::unique_ptr<frame_t> frame;
		{
			std::lock_guard<std::mutex> const lck{g_frames.m_mtx};
			if(!g_frames.m_ready_frames.empty())
			{
				frame = std::move(g_frames.m_ready_frames.front());
				g_frames.m_ready_frames.pop();
			}
		}
		auto const return_frame = mk::make_scope_exit([&]()
		{
			using std::swap;
			if(frame)
			{
				swap(g_app_state->m_last_frame, frame);
			}
			if(frame)
			{
				{
					std::lock_guard<std::mutex> const lck{g_frames.m_mtx};
					g_frames.m_empty_frames.push(std::move(frame));
				}
				g_frames.m_cv.notify_one();
			}
		});
		if(!frame)
		{
			swap(frame, g_app_state->m_last_frame);
			std::printf("Could not compute frame in time for next present!\n");
		}
		if(frame && frame->m_count != 0)
		{
			D3D11_MAPPED_SUBRESOURCE d3d11_mapped_sub_resource;
			HRESULT const mapped = g_app_state->m_d3d11_immediate_context->Map(g_app_state->m_d3d11_vlp_instance_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &d3d11_mapped_sub_resource);
			CHECK_RET_V(mapped == S_OK);
			std::memcpy(d3d11_mapped_sub_resource.pData, frame->m_vertices, sizeof(float3_t) * frame->m_count);
			g_app_state->m_d3d11_immediate_context->Unmap(g_app_state->m_d3d11_vlp_instance_buffer, 0);
			g_app_state->m_d3d11_immediate_context->DrawIndexedInstanced(s_indices_per_cube, frame->m_count, 0, 0, 0);
			g_app_state->m_frames_counter.count();
		}
	}while(false);

	HRESULT const presented = g_app_state->m_d3d11_swap_chain->Present(1, 0);
	CHECK_RET(presented == S_OK || presented == DXGI_STATUS_OCCLUDED, false);

	return true;
}

void network_thread_proc()
{
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

		g_app_state->m_packet_coutner.count();
	}
}

void process_data(unsigned char const* const& data, int const& data_len)
{
	#pragma push_macro("CHECK_RET")
	#undef CHECK_RET
	#define CHECK_RET(X) do{ if(X){}else{ [[unlikely]] return; } }while(false)

	static constexpr auto const s_accept_point = [](double const& x, double const& y, double const& z, double const& a, void* const& ctx)
	{
		app_state_t& app_state = *static_cast<app_state_t*>(ctx);
		assert(!app_state.m_incomming_azimuths.is_full());
		assert(!app_state.m_incomming_points.is_full());
		app_state.m_incomming_azimuths.push(a);
		app_state.m_incomming_points.push(incomming_point_t{x, y, z});
	};

	CHECK_RET(data_len == mk::vlp16::s_packet_size);
	auto const& packet = mk::vlp16::raw_data_to_single_mode_packet(data, data_len);
	CHECK_RET(mk::vlp16::verify_single_mode_packet(packet));
	int const incomming_stuff_count = g_app_state->m_incomming_stuff_count.load();
	if(g_app_state->m_incomming_azimuths.s_capacity_v - incomming_stuff_count < mk::vlp16::s_points_per_packet)
	{
		std::printf("Not enough free space in ring buffer, dropping incomming packet!\n");
		return;
	}
	mk::vlp16::convert_to_xyza(packet, s_accept_point, g_app_state);
	g_app_state->m_incomming_stuff_count.fetch_add(mk::vlp16::s_points_per_packet);

	#pragma pop_macro("CHECK_RET")
}

void frames_thread_proc()
{
	for(;;)
	{
		std::unique_ptr<frame_t> frame;
		{
			std::unique_lock<std::mutex> lck{g_frames.m_mtx};
			for(;;)
			{
				if(!g_frames.m_empty_frames.empty())
				{
					frame = std::move(g_frames.m_empty_frames.front());
					g_frames.m_empty_frames.pop();
					break;
				}
				else
				{
					if(g_frames.m_stop_requested.load() == true)
					{
						return;
					}
					g_frames.m_cv.wait(lck);
				}
			}
		}
		auto const return_frame = mk::make_scope_exit([&]()
		{
			std::lock_guard<std::mutex> const lck{g_frames.m_mtx};
			g_frames.m_ready_frames.push(std::move(frame));
		});

		do
		{
			frame->m_count = 0;
			int const incomming_stuff_count = g_app_state->m_incomming_stuff_count.load();
			auto& azimuths = g_app_state->m_incomming_azimuths;
			auto& incomming_points = g_app_state->m_incomming_points;
			int const n = incomming_stuff_count;
			if(n == 0)
			{
				break;
			}
			auto const& back = azimuths[n - 1];
			auto const fn_find_min = [&]()
			{
				for(int i = n - 1; i != 0; --i)
				{
					if(azimuths[i - 1] > azimuths[i])
					{
						return i;
					}
				}
				return 1;
			};
			int const min_val_i = fn_find_min();
			int target_i = 0;
			for(int i = min_val_i - 1; i != -1; --i)
			{
				if(azimuths[i] <= back)
				{
					target_i = i + 1;
					break;
				}
			}
			int const to_pop = target_i;
			azimuths.pop(to_pop);
			incomming_points.pop(to_pop);
			g_app_state->m_incomming_stuff_count.fetch_sub(to_pop);
			int const new_count = n - to_pop;
			for(int i = 0; i != new_count; ++i)
			{
				auto const& ip = incomming_points[i];
				float3_t const position = float3_t{static_cast<float>(ip.m_x), static_cast<float>(ip.m_y), static_cast<float>(ip.m_z)};
				float3_t& p = frame->m_vertices[i];
				p = position;
			}
			frame->m_count = new_count;
		}while(false);

	}
}
