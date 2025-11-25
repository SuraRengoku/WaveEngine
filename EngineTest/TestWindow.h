#pragma once
#include "Test.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"

using namespace WAVEENGINE;

PLATFORM::window _windows[4];

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_DESTROY: {
		bool all_closed{ true };
		for (u32 i{ 0 }; i < _countof(_windows); ++i) {
			if (!_windows[i].is_closed()) {
				all_closed = false;
			}
		}
		if (all_closed) {
			PostQuitMessage(0);
			return 0;
		}
		break;
	}
	case WM_SYSCHAR: {
		if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
			PLATFORM::window win{ PLATFORM::window_id{(ID::id_type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
			win.set_fullscreen(!win.is_fullscreen());
			return 0;
		}
		break;
	}
	}
	
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

class engineTest : public test {
public:
	bool initialize() override {
		PLATFORM::window_init_info info[]{
			{&win_proc, nullptr, L"Test window 1", 100, 100, 400, 800},
			{&win_proc, nullptr, L"Test window 2", 150, 150, 800, 400},
			{&win_proc, nullptr, L"Test window 3", 200, 200, 400, 400},
			{&win_proc, nullptr, L"Test window 4", 250, 250, 800, 600},
		};

		for (u32 i{ 0 }; i < _countof(_windows); ++i) {
			_windows[i] = PLATFORM::create_window(&info[i]);
		}
		return true;
	}
	
	void run() override {
		std::this_thread::sleep_for(std::chrono::microseconds(10));
	}

	void shutdown() override {
		for (u32 i{ 0 }; i < _countof(_windows); ++i) {
			PLATFORM::remove_window(_windows[i].get_id());
		}
	}

};