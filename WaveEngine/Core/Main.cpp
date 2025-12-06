/*	
	Things to do to create a game project:

	1) Generate an MSVC solution/project
	2) Add files that contain the script
	3) Set include and library directories
	4) Set force include file (GameEntity.h)
	5) Set c++ language version and calling convension
*/

#include "CommonHeaders.h"
#include <filesystem>

#ifdef _WIN64
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <crtdbg.h>

namespace {

std::filesystem::path set_current_directory_to_executable_path() {
	wchar_t path[MAX_PATH]{};
	const u32 length{ GetModuleFileName(0, &path[0], MAX_PATH) };
	if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		return {};

	std::filesystem::path p{ path };
	std::filesystem::current_path(p.parent_path());
	return std::filesystem::current_path();
}

}

#ifndef USE_WITH_EDITOR

extern bool engine_initialize();
extern void engine_update();
extern void engine_shutdown();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // detect memory leaks
#endif

	set_current_directory_to_executable_path();

	if (engine_initialize()) {
		MSG msg{};
		bool is_running{ true };
		// NOTE: call engine's update function
		while (is_running) {
			
			// NOTE: read, removes and dispatches messages form the message queue,
			//		 until there are no messages left to process
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
				is_running &= (msg.message != WM_QUIT);
			}

			engine_update();
		}
	}
	engine_shutdown();
	return 0;

}

#endif // USE_WITH_EDITOR
#endif // _WIN64