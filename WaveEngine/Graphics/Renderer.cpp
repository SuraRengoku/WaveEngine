#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "Direct3D12\D3D12Interface.h"

namespace WAVEENGINE::GRAPHICS {

namespace {
platform_interface gfx{};

bool set_platform_interface(graphics_platform platform) {
	switch (platform) {
	case graphics_platform::Direct3D12:
		D3D12::get_platform_interface(gfx);
		break;
	case graphics_platform::Direct3D11:
		break;
	case graphics_platform::Vulkan:
		break;
	case graphics_platform::OpenGL:
		break;
	default:
		return false;
	}
	
	return true;
}

} // anonymous namespace

bool initialize(graphics_platform platform) {
	return set_platform_interface(platform) && gfx.initialize();
}

void shutdown() {
	gfx.shutdown();
}

surface create_surface(PLATFORM::window window) {
	return gfx.surface.create(window);
}

void remove_surface(surface_id id) {
	assert(ID::is_valid(id));
	gfx.surface.remove(id);
}

void surface::resize(u32 width, u32 height) const {
	assert(is_valid());
	gfx.surface.resize(this->_id, width, height);
}

u32 surface::width() const {
	assert(is_valid());
	return gfx.surface.width(this->_id);
}

u32 surface::height() const {
	assert(is_valid());
	return gfx.surface.height(this->_id);
}

void surface::render() const {
	assert(is_valid());
	gfx.surface.render(this->_id);
}

}
