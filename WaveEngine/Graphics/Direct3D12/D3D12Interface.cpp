#include "D3D12Interface.h"

namespace WAVEENGINE::GRAPHICS::D3D12 {

void get_platform_interface(platform_interface& pi) {
	pi.initialize = CORE::initialize;
	pi.shutdown = CORE::shutdown;

	pi.surface.create = CORE::create_surface;
	pi.surface.remove = CORE::remove_surface;
	pi.surface.resize = CORE::resize_surface;
	pi.surface.width = CORE::surface_width;
	pi.surface.height = CORE::surface_height;
	pi.surface.render = CORE::render_surface;
}

}
