#if !defined(SHIPPING)

#include "..\Content\ContentLoader.h"
#include "..\Components\Script.h"
#include "..\Platform\PlatformTypes.h" 
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"
#include <thread>

using namespace WAVEENGINE;

namespace {

GRAPHICS::render_surface game_window{};

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch(msg) {
	case WM_DESTROY: {
		if (game_window.window.is_closed()) {
			PostQuitMessage(0);
			return 0;
		}
		break;
	}
	case WM_SYSCHAR: {
		if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
			game_window.window.set_fullscreen(!game_window.window.is_fullscreen());
			return 0;
		}
		break;
	}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

}

bool engine_initialize() {
	if (!WAVEENGINE::CONTENT::load_game())
		return false;
	
	PLATFORM::window_init_info info{
		&win_proc, nullptr, L"Wave Game"
	};
	
	game_window.window = PLATFORM::create_window(&info);
	if (!game_window.window.is_valid())
		return false;
	return true;
}

void engine_update() {
	WAVEENGINE::SCRIPT::update(10.0f);
	std::this_thread::sleep_for(std::chrono::microseconds(10));
}

void engine_shutdown() {
	PLATFORM::remove_window(game_window.window.get_id());
	WAVEENGINE::CONTENT::unload_game();
}

#endif // !defined(SHIPPING)