#include "Platform.h"
#include "PlatformTypes.h"

namespace WAVEENGINE::PLATFORM {

#ifdef _WIN64
namespace {

struct window_info { 
	HWND	hwnd{ nullptr };
	RECT	client_area{ 0, 0, 1920, 1080 };
	RECT	fullscreen_area{};
	POINT	top_left{ 0, 0 };
	DWORD	style{ WS_VISIBLE };
	bool	is_fullscreen{ false };
	bool	is_closed{ false };

	//~window_info() { assert(!is_fullscreen); }
};

UTL::freeList<window_info> windows; // window_info is a trivially destructible data type 

////////////////////////////////////////////////////////////////////////////////
// TODO: this part will be handled by a free-list container later

//UTL::vector<u32> available_slots;
//
//u32 add_to_windows(window_info info) {
//	u32 id{ u32_invalid_id };
//	if (available_slots.empty()) {
//		id = (u32)windows.size();
//		windows.emplace_back(info);
//	}
//	else {
//		id = available_slots.back();
//		available_slots.pop_back();
//		assert(id != u32_invalid_id);
//		windows[id] = info;
//	}
//	return id;
//}
//
//void remove_from_windows(u32 id) {
//	assert(id < windows.size());
//	available_slots.emplace_back(id);
//}

////////////////////////////////////////////////////////////////////////////////

window_info& get_from_id(window_id id) {
	assert(id < windows.size());
	assert(windows[id].hwnd);
	return windows[id];
}

window_info& get_from_handle(window_handle handle) {
	const window_id id{ (ID::id_type)GetWindowLongPtr(handle, GWLP_USERDATA) };
	return get_from_id(id);
}

/**
 * @brief get defined function pointer of window procedure and return it to self-defined processing function or default processing function
 * @param hwnd window handle
 * @param msg message type(WM_PAINT, WM_CLOSE...)
 * @param additional parameters
 */
LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	window_info* info{ nullptr };

	switch (msg) {
	case WM_DESTROY:
		get_from_handle(hwnd).is_closed = true;
		break;
	case WM_EXITSIZEMOVE:
		info = &get_from_handle(hwnd);
		break;
	case WM_SIZE:
		if (wparam == SIZE_MAXIMIZED) {
			info = &get_from_handle(hwnd);
		}
		break;
	case WM_SYSCOMMAND:
		if (wparam == SC_RESTORE){
			info = &get_from_handle(hwnd);
		}
		break;
	default:
		break;
	}

	if (info) {
		assert(info->hwnd);
		GetClientRect(info->hwnd, info->is_fullscreen ? &info->fullscreen_area : &info->client_area);
	}

	LONG_PTR long_ptr{ GetWindowLongPtr(hwnd, 0) };
	return long_ptr ? ((window_proc)long_ptr)(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
}

void resize_window(const window_info& info, const RECT& area) {
	// Adjust the window size for correct devive size
	RECT window_rect{ area };
	AdjustWindowRect(&window_rect, info.style, FALSE); // fit the complete window to the client area

	const s32 width{ window_rect.right - window_rect.left };
	const s32 height{ window_rect.bottom - window_rect.top };

	MoveWindow(info.hwnd, info.top_left.x, info.top_left.y, width, height, true);
}

void resize_window(window_id id, u32 width, u32 height) {
	window_info& info{ get_from_id(id) };

	// NOTE: when we host the window in the editor we just update the internal data(i.e. the client are dimensions).
	if (info.style & WS_CHILD) {
		GetClientRect(info.hwnd, &info.client_area);
	}
	else {
		// NOTE: we also resize while in fullscreen mode to support the case when the use changes the screen resolution
		RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
		area.bottom = area.top + height;
		area.right = area.left + width;

		resize_window(info, area);
	}
}

void set_window_fullscreen(window_id id, bool is_fullscreen) {
	window_info& info{ get_from_id(id) };

	if (info.is_fullscreen != is_fullscreen) {

		info.is_fullscreen = is_fullscreen;

		if (is_fullscreen) {
			// Store the current window dimensions so they can be restored when switching out of fullscreen state.

			// store the current size
			GetClientRect(info.hwnd, &info.client_area);
			RECT rect;
			GetWindowRect(info.hwnd, &rect);
			// store the current position
			info.top_left.x = rect.left;
			info.top_left.y = rect.top;
			SetWindowLongPtr(info.hwnd, GWL_STYLE, 0); 
			ShowWindow(info.hwnd, SW_MAXIMIZE);
		}
		else {
			SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
			resize_window(info, info.client_area);
			ShowWindow(info.hwnd, SW_SHOWNORMAL);
		}
	}
}

inline bool is_window_fullscreen(window_id id) {
	return get_from_id(id).is_fullscreen;
}

inline window_handle get_window_handle(window_id id) {
	return get_from_id(id).hwnd;
}

inline void set_window_caption(window_id id, const wchar_t* caption) {
	window_info& info{ get_from_id(id) };
	SetWindowText(info.hwnd, caption);
}

MATH::u32v4 get_window_size(window_id id) {
	window_info& info{ get_from_id(id) };
	RECT area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
	return { (u32)area.left, (u32)area.top, (u32)area.right, (u32)area.bottom };
}

inline bool is_window_closed(window_id id) {
	return get_from_id(id).is_closed;
}

} // anonymous namespace

/*

	©°©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤-©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤-©¤©¤©¤©¤©¤©¤©´  ¡û complete window
	©¦  title (Caption)								  ©¦  ¡û non_client_area
	©À©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤-©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤-©¤©¤©¤©¤©È
	©¦ border ©¦  client_area (1920¡Á1080)		  ©¦ border©¦  ¡û client + border
	©¦		 ©¦                                ©¦       ©¦
	©¦        ©¦                                ©¦       ©¦
	©¦        ©¦				drawing			  ©¦       ©¦
	©¦        ©¦								  ©¦       ©¦
	©¦        ©¦								  ©¦       ©¦
	©¸©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¼
*/


window create_window(const window_init_info* const init_info /* = nullptr */) {
	window_proc callback{ init_info ? init_info->callback : nullptr };
	window_handle parent{ init_info ? init_info->parent : nullptr };

	// Setup a window class
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX); // size of the struct
	wc.style = CS_HREDRAW | CS_VREDRAW; // style of the window
	wc.lpfnWndProc = internal_window_proc; // function pointer to a internal window procedure
	wc.cbClsExtra = 0; // the number of extra bytes to allocate following the window-class structure
	wc.cbWndExtra = callback ? sizeof(callback) : 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"WaveWindow";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// Register the window class
	if (!RegisterClassExW(&wc)) {
		DWORD err = GetLastError();
	}

	// Create an instance of the window class
	window_info info{};

	info.client_area.right = (init_info && init_info->width) ? info.client_area.left + init_info->width : info.client_area.right;
	info.client_area.bottom = (init_info && init_info->height) ? info.client_area.top + init_info->height : info.client_area.bottom;
	info.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

	RECT rect{ info.client_area };
	
	// ref: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrect
	AdjustWindowRect(&rect, info.style, FALSE); // adjust the window size for correct device size

	const wchar_t* caption{ (init_info && init_info->caption) ? init_info->caption : L"Wave Game" }; // title bar
	const s32 left{ init_info ? init_info->left : info.top_left.x };
	const s32 top{ init_info ? init_info->top : info.top_left.y };
	const s32 width{ rect.right - rect.left };
	const s32 height{ rect.bottom - rect.top };

	info.hwnd = CreateWindowEx(
		/* DWORD     dwExStyle    */ 0,					// extended style
		/* LPCSTR    lpClassName  */ wc.lpszClassName,	// window extra class
		/* LPCSTR    lpWindowName */ caption,			// instance title
		/* DWORD     dwStyle      */ info.style,		// window style
		/* int       X            */ left, top,			// initial window position
		/* int       Y            */
		/* int       nWidth       */ width, height,		// initial window size		
		/* int       nHeight      */
		/* HWND      hWndParent   */ parent,			// handle to parent window	
		/* HMENU     hMenu        */ NULL,				// handle to menu 
		/* HINSTANCE hIntance     */ NULL,				// instance of this application
		/* LPVOID    lpParam      */ NULL				// extra creation parameters
	);

	if (info.hwnd) {

		DEBUG_OP(SetLastError(0));

		const window_id id{ windows.add(info) };
		SetWindowLongPtr(info.hwnd, GWLP_USERDATA, (LONG_PTR)id);

		// Set in the "extra" bytes the pointer to the window callback function which handles messaged for the window
		if(callback) SetWindowLongPtr(info.hwnd, 0, (LONG_PTR)callback);
		assert(GetLastError() == 0);

		ShowWindow(info.hwnd, SW_SHOWNORMAL);
		UpdateWindow(info.hwnd);
		return window{ id };
	}
	else {
		DWORD err = GetLastError();
		assert(err == 0);
	}
	return {};
}

void remove_window(window_id id) {
	window_info& info{ get_from_id(id) };
	DestroyWindow(info.hwnd);
	windows.remove(id);
}

#elif defined(LINUX)
// TODO
#else
#error "must implement at least one platform"
#endif // _WIN64

void window::set_fullscreen(bool is_fullscreen) const {
	assert(is_valid());
	set_window_fullscreen(_id, is_fullscreen);
}

bool window::is_fullscreen() const {
	assert(is_valid());
	return is_window_fullscreen(_id);
}

void* window::handle() const {
	assert(is_valid());
	return get_window_handle(_id);
}

void window::set_caption(const wchar_t* caption) const {
	assert(is_valid());
	set_window_caption(_id, caption);
}

const MATH::u32v4 window::size() const {
	assert(is_valid());
	return get_window_size(_id);
}

void window::resize(u32 width, u32 height) const {
	assert(is_valid());
	resize_window(_id, width, height);
}

const u32 window::width() const {
	MATH::u32v4 s{ size() };
	return s.z - s.x; // right - left;
}

const u32 window::height() const {
	MATH::u32v4 s{ size() };
	return s.w - s.y; // bottom - top
} 

bool window::is_closed() const {
	assert(is_valid());
	return is_window_closed(_id);
}

} // WAVEENGINE::PLATFORM