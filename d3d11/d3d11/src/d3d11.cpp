#include "scope_exit.h"

#include <cassert> // assert
#include <cstdio> // std::printf, std::puts
#include <cstdlib> // std::exit, EXIT_SUCCESS, EXIT_FAILURE
#include <cstring> // std::memcpy
#include <cwchar> // std::wcslen
#include <string> // std::u8string, std::u16string


#include <windows.h>
#include <objbase.h>
#include <dxgi.h>
#include <c:\\Users\\me\\Downloads\\dx\\dx9\\include\\d3dx11core.h>


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


struct app_state_t
{
	ATOM m_main_window_class;
	HWND m_main_window;
	ID3D11Device* m_d3d11_device;
	ID3D11DeviceContext* m_d3d11_immediate_context;
	IDXGISwapChain* m_d3d11_swap_chain;
	ID3D11RenderTargetView* m_d3d11_render_target_view;
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
bool run_main_loop(int* const& out_exit_code);
bool process_message(MSG const& msg);
bool on_idle();
std::u8string utf16_to_utf8(std::u16string const& u16str);
bool render();


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
	HRESULT const d3d11_device_created = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0, D3D11_SDK_VERSION, &d3d11_device, &d3d11_feature_level, &d3d11_immediate_context);
	CHECK_RET(d3d11_device_created == S_OK, false);
	auto const d3d11_device_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_device->Release(); });
	auto const d3d11_immediate_context_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_immediate_context->Release(); });
	g_app_state->m_d3d11_device = d3d11_device;
	g_app_state->m_d3d11_immediate_context = d3d11_immediate_context;

	IDXGISwapChain* d3d11_swap_chain;
	DXGI_SWAP_CHAIN_DESC d3d11_swap_chain_description;
	d3d11_swap_chain_description.BufferDesc.Width = 1920;
	d3d11_swap_chain_description.BufferDesc.Height = 1080;
	d3d11_swap_chain_description.BufferDesc.RefreshRate.Numerator = 60;
	d3d11_swap_chain_description.BufferDesc.RefreshRate.Denominator = 1;
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

	ID3D11Texture2D* back_buffer;
	HRESULT const d3d11_got_back_buffer = d3d11_swap_chain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void**>(&back_buffer));
	CHECK_RET(d3d11_got_back_buffer == S_OK, false);
	auto const back_buffer_free = mk::make_scope_exit([&](){ back_buffer->Release(); });

	ID3D11RenderTargetView* d3d11_render_target_view;
	HRESULT const d3d11_render_target_view_created = d3d11_device->CreateRenderTargetView(back_buffer, nullptr, &d3d11_render_target_view);
	CHECK_RET(d3d11_render_target_view_created == S_OK, false);
	auto const d3d11_render_target_view_free = mk::make_scope_exit([&](){ g_app_state->m_d3d11_render_target_view->Release(); });
	g_app_state->m_d3d11_render_target_view = d3d11_render_target_view;

	d3d11_immediate_context->OMSetRenderTargets(1, &d3d11_render_target_view, nullptr);

	D3D11_VIEWPORT d3d11_view_port;
	d3d11_view_port.TopLeftX = 0.0f;
	d3d11_view_port.TopLeftY = 0.0f;
	d3d11_view_port.Width = 1920.0f;
	d3d11_view_port.Height = 1080.0f;
	d3d11_view_port.MinDepth = 0.0f;
	d3d11_view_port.MaxDepth = 1.0f;
	d3d11_immediate_context->RSSetViewports(1, &d3d11_view_port);
	/* D3D11 */

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
		case WM_PAINT:
		{
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
		static constexpr float const s_background_color[4] = {0.0f, 0.125f, 0.6f, 1.0f};
		g_app_state->m_d3d11_immediate_context->ClearRenderTargetView(g_app_state->m_d3d11_render_target_view, s_background_color);

		HRESULT const presented = g_app_state->m_d3d11_swap_chain->Present(1, 0);
		CHECK_RET(presented == S_OK || presented == DXGI_STATUS_OCCLUDED, false);

	return true;
}
