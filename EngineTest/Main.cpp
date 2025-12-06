#pragma comment(lib, "waveengine.lib")
#include "Test.h"

#if TEST_ENTITY_COMPONENTS

#include "TestEntityComponents.h"

#elif TEST_WINDOW

#include "TestWindow.h"

#elif TEST_RENDERER

#include "TestRenderer.h"

#else
#error One of the tests need to be enabled
#endif

#ifdef _WIN64

#include <windows.h>
#include <filesystem>

std::filesystem::path set_current_directory_to_executable_path() {
	wchar_t path[MAX_PATH]{};
	const u32 length{ GetModuleFileName(0, &path[0], MAX_PATH) };
	if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		return {};

	std::filesystem::path p{ path };
	std::filesystem::current_path(p.parent_path());
	return std::filesystem::current_path();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // detect memory leaks
#endif

	set_current_directory_to_executable_path();

	engineTest test{};

	if (test.initialize()) {
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

			test.run();
		}
	}
	test.shutdown();
	return 0;

}

#else

int main() {
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // detect memory leaks
#endif
	
	engineTest test{};
	
	if (test.initialize()) {
		test.run();
	}

	test.shutdown();
}

#endif // _WIN64