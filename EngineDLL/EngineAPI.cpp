#include "Common.h"
#include "CommonHeaders.h"
#include "..\WaveEngine\Components\Script.h"
#include "..\Graphics\Renderer.h"
#include "..\Platform\Platform.h"
#include "..\Platform\PlatformTypes.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <atlsafe.h>

using namespace WAVEENGINE;

namespace {
HMODULE game_code_dll{ nullptr };
using _get_script_creator = WAVEENGINE::SCRIPT::DETAIL::script_creator(*)(size_t);
_get_script_creator get_script_creator{ nullptr };

using _get_script_names = LPSAFEARRAY(*)(void);
_get_script_names get_script_names{ nullptr };

UTL::vector<GRAPHICS::render_surface> surfaces;
}

EDITOR_INTERFACE 
u32 LoadGameCodeDll(const char* dll_path) {
	if (game_code_dll) return FALSE;
	game_code_dll = LoadLibraryA(dll_path);
	assert(game_code_dll);
	
	// Get function with name "xxx" by search functions in game_code_dll by calling GetProcAddress
	// do not forget to do type casting
	get_script_creator = (_get_script_creator)GetProcAddress(game_code_dll, "get_script_creator");
	get_script_names = (_get_script_names)GetProcAddress(game_code_dll, "get_script_names");

	return (game_code_dll && get_script_creator && get_script_names) ? TRUE : FALSE;
}

EDITOR_INTERFACE
u32 UnloadGameCodeDll() {
	if (!game_code_dll) return FALSE;
	assert(game_code_dll);
	int result{ FreeLibrary(game_code_dll) };
	assert(result);
	game_code_dll = nullptr;
	return TRUE;
}

EDITOR_INTERFACE
SCRIPT::DETAIL::script_creator GetScriptCreator(const char* name) {
	return (game_code_dll && get_script_creator) ? get_script_creator(SCRIPT::DETAIL::string_hash()(name)) : nullptr;
}

EDITOR_INTERFACE
LPSAFEARRAY GetScriptNames() {
	return (game_code_dll && get_script_names) ? get_script_names() : nullptr;
}

EDITOR_INTERFACE
u32 CreateRenderSurface(HWND host, s32 width, s32 height) {
	assert(host);
	PLATFORM::window_init_info info{ nullptr, host, nullptr, 0, 0, width, height };
	GRAPHICS::render_surface surface{ PLATFORM::create_window(&info), {} };
	assert(surface.window.is_valid());
	surfaces.emplace_back(surface);
	return static_cast<u32>(surfaces.size()) - 1;
}

EDITOR_INTERFACE
void RemoveRenderSurface(u32 id) {
	assert(id < surfaces.size());
	// TODO: remove surface from surfaces
	PLATFORM::remove_window(surfaces[id].window.get_id());
}

EDITOR_INTERFACE
HWND GetWindowHandle(u32 id) {
	assert(id < surfaces.size());
	return (HWND)surfaces[id].window.handle();
}

EDITOR_INTERFACE
void ResizeRenderSurface(u32 id) {
	assert(id < surfaces.size());
	surfaces[id].window.resize(0, 0);
}