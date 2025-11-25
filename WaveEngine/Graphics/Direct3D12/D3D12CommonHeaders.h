#pragma once

#include "CommonHeaders.h"
#include "Graphics\Renderer.h"
#include "Platform\Window.h"

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
// wrl.h doesn't have anything to do with DirectX, 
// but contains their ComPtr class that acts as a smart pointer wrapper for COM interface pointers

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")


namespace WAVEENGINE::GRAPHICS::D3D12 {

constexpr u32 frame_buffer_count{ 3 };

}


// assert that COM call to D3D API succeed
#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)									\
if (FAILED(x)) {									\
	char line_number[32];							\
	sprintf_s(line_number, "%u", __LINE__);			\
	OutputDebugStringA("Error in: ");				\
	OutputDebugStringA(__FILE__);					\
	OutputDebugStringA("\nLine: ");					\
	OutputDebugStringA(line_number);				\
	OutputDebugStringA("\n");						\
	OutputDebugStringA(#x);							\
	OutputDebugStringA("\n");						\
	__debugbreak();									\
}

#endif // !DXCall
#else
#ifndef DXCall
#define DXCall(x) x
#endif // !DXCall

#endif // _DEBUG

#ifdef _DEBUG
// set the name of the COM object and output a debug information 
#define NAME_D3D12_OBJECT(obj, name) obj->SetName(name); OutputDebugString(L"::D3D12 Object Created: "); OutputDebugString(name); OutputDebugString(L"\n");
// the indexed veriant will include the index in the name of the object
#define NAME_D3D12_OBJECT_INDEXED(obj, idx, name)		\
{														\
wchar_t full_name[128];									\
if(swprintf_s(full_name, L"%s[%u]", name, idx) > 0) {	\
	obj->SetName(full_name);							\
	OutputDebugString(L"::D3D12 Object Created: ");		\
	OutputDebugString(full_name);						\
	OutputDebugString(L"\n");							\
}}

#else

#define NAME_D3D12_OBJECT(x, name)
#define NAME_D3D12_OBJECT_INDEXED(x, idx, name)

#endif // _DEBUG
