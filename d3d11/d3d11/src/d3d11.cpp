#include "scope_exit.h"

#include <cassert> // assert
#include <cstdio> // std::printf, std::puts
#include <cstdlib> // std::exit, EXIT_SUCCESS, EXIT_FAILURE
#include <cstring> // std::memcpy

#include <windows.h>
#include <objbase.h>


struct app_state_t
{
	ATOM m_main_window_class;
	HWND m_main_window;
};


static app_state_t* g_app_state;


void check_ret_failed(char const* const& file, int const& line, char const* const& expr);
int seh_filter(unsigned int const code, EXCEPTION_POINTERS* const ep);
bool d3d11_seh(int const argc, char const* const* const argv, int* const& out_exit_code);
bool d3d11(int const argc, char const* const* const argv, int* const& out_exit_code);
bool register_main_window_class(ATOM* const& out_main_window_class);
bool unregister_main_window_class(ATOM const& main_window_class);
LRESULT CALLBACK main_window_proc(_In_ HWND const hwnd, _In_ UINT const msg, _In_ WPARAM const w_param, _In_ LPARAM const l_param);
bool create_main_window(ATOM const& main_window_class, HWND* const& out_main_window);
bool run_main_loop(int* const& out_exit_code);
bool process_message(MSG const& msg);
bool on_idle();


#define CHECK_RET(X, R) do{ if(X){}else{ [[unlikely]] check_ret_failed(__FILE__, __LINE__, #X); return R; } }while(false)
#define CHECK_RET_V(X) do{ if(X){}else{ [[unlikely]] check_ret_failed(__FILE__, __LINE__, #X); std::exit(EXIT_FAILURE); } }while(false)


int main(int const argc, char const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([&](){ std::puts("Oh, no! Someting went wrong!"); });

	int exit_code;
	bool const bussiness = d3d11_seh(argc, argv, &exit_code);
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

bool d3d11_seh(int const argc, char const* const* const argv, int* const& out_exit_code)
{
	__try
	{
		int exit_code;
		bool const bussiness = d3d11(argc, argv, &exit_code);
		CHECK_RET(bussiness, false);
		*out_exit_code = exit_code;
		return true;
	}
	__except(seh_filter(GetExceptionCode(), GetExceptionInformation()))
	{
	}
	return false;
}

bool d3d11(int const argc, char const* const* const argv, int* const& out_exit_code)
{
	(void)argc;
	(void)argv;

	app_state_t* const app_state = new app_state_t{};
	auto const app_state_free = mk::make_scope_exit([&](){ delete app_state; });
	g_app_state = app_state;

	HRESULT const com_initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
	CHECK_RET(com_initialized == S_OK, false);
	auto const com_uninitialize = mk::make_scope_exit([](){ CoUninitialize(); });

	bool const registered = register_main_window_class(&app_state->m_main_window_class);
	CHECK_RET(registered, false);
	auto const unregister = mk::make_scope_exit([&](){ bool const unregistered = unregister_main_window_class(app_state->m_main_window_class); CHECK_RET_V(unregistered); });

	bool const window_created = create_main_window(app_state->m_main_window_class, &app_state->m_main_window);
	CHECK_RET(window_created, false);

	ShowWindow(app_state->m_main_window, SW_SHOW);

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
	return true;
}
