#pragma once
#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace WAVEENGINE::GRAPHICS::D3D12 {

class descriptorHeap;

} // forward decleration

namespace WAVEENGINE::GRAPHICS::D3D12::CORE {

bool initialize();
void shutdown();

template<typename T>
constexpr void release(T*& resource) {
	if (resource) {
		resource->Release();
		resource = nullptr;
	}
}

namespace DETAIL {

void deferred_release(IUnknown* resource);

}

template<typename T>
constexpr void deferred_release(T*& resource) {
	if (resource) {
		DETAIL::deferred_release(resource);
		resource = nullptr;
	}
}


ID3D12Device *const device();

descriptorHeap& rtv_heap();

descriptorHeap& dsv_heap();

descriptorHeap& srv_heap();

descriptorHeap& uav_heap();

DXGI_FORMAT default_render_target_format();

u32 current_frame_index();

void set_deferred_releases_flag();



surface create_surface(PLATFORM::window window);

void remove_surface(surface_id id);

void resize_surface(surface_id id, u32 width, u32 height);

u32 surface_width(surface_id id);
u32 surface_height(surface_id id);

void render_surface(surface_id id);
}
