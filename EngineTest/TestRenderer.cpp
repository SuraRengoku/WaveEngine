#include "TestRenderer.h"

#if TEST_RENDERER

////////////////////////////////////////////////////////////////////////////////////////////////
template<bool test, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> { using type = T; };

template<bool test, typename T = void>
using enable_if_t = typename enable_if<test, T>::type;

template<typename T>
using iterator_cat_t = typename std::iterator_traits<T>::iterator_category;

template<typename T>
using void_t = void;

template<typename T, typename=void>
constexpr bool is_iterator_v = false;

template<typename T>
constexpr bool is_iterator_v<T, void_t<iterator_cat_t<T>>> = true;

template<typename T, enable_if_t<is_iterator_v<T>, int> = 0>
void function(T t) {
	// 
}

void function(int t) {
	// 
}
////////////////////////////////////////////////////////////////////////////////////////////////

using namespace WAVEENGINE;

GRAPHICS::render_surface _surfaces[4];

timeIt timer{};

void destroy_render_surface(GRAPHICS::render_surface& surface);

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_DESTROY: {
		bool all_closed{ true };
		for (u32 i{ 0 }; i < _countof(_surfaces); ++i) {
			if (_surfaces[i].window.is_valid()) {
				if (_surfaces[i].window.is_closed()) {
					destroy_render_surface(_surfaces[i]);
				}
				else {
					all_closed = false;
				}
			}
		}
		if (all_closed) {
			PostQuitMessage(0);
			return 0;
		}
		break;
	}
	case WM_SYSCHAR: {
		if(wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
			PLATFORM::window win{ PLATFORM::window_id{(ID::id_type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
			win.set_fullscreen(!win.is_fullscreen());
			return 0;
		}
		break;
	}
	case WM_KEYDOWN: {
		if (wparam == VK_ESCAPE) {
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
	}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void create_render_surface(GRAPHICS::render_surface& surface, PLATFORM::window_init_info& info) {
	surface.window = PLATFORM::create_window(&info); // now we already have window handle
	surface.surface = GRAPHICS::create_surface(surface.window); // create a surface and attach it to window handle
}

void destroy_render_surface(GRAPHICS::render_surface& surface) {
	GRAPHICS::render_surface temp{ surface };
	surface = {};
	if(temp.surface.is_valid())	GRAPHICS::remove_surface(temp.surface.get_id()); 
	if(temp.window.is_valid())	PLATFORM::remove_window(temp.window.get_id());
}

bool engineTest::initialize() {
	while (!compile_shaders()) {
		if (MessageBox(nullptr, L"Failed to compile engine shaders", L"Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY)
			return false;
	}

	if (!GRAPHICS::initialize(GRAPHICS::graphics_platform::Direct3D12)) return false;

	PLATFORM::window_init_info info[]{
		{&win_proc, nullptr, L"Render window 1", 100, 100, 400, 800},
		{&win_proc, nullptr, L"Render window 2", 150, 150, 800, 400},
		{&win_proc, nullptr, L"Render window 3", 200, 200, 400, 400},
		{&win_proc, nullptr, L"Render window 4", 250, 250, 800, 600},
	};
	static_assert(_countof(info) == _countof(_surfaces));

	for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
		create_render_surface(_surfaces[i], info[i]);

	return true;
}


void engineTest::run() {
	timer.begin();

	std::this_thread::sleep_for(std::chrono::milliseconds(10)); // too high frame rate may burn your graphic card!

	for (u32 i{ 0 }; i < _countof(_surfaces); ++i) {
		if (_surfaces[i].surface.is_valid()) {
			_surfaces[i].surface.render();
		}
	}

	timer.end();
}

void engineTest::shutdown() {
	for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
		destroy_render_surface(_surfaces[i]);

	GRAPHICS::shutdown();
}

#endif // TEST_RENDERER