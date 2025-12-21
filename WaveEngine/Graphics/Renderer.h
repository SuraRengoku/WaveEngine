#pragma once
#include "CommonHeaders.h"
#include "..\Platform\Window.h"

#define USE_VULKAN 1
#define USE_D3D12 0
#define USE_OPENGL 0

namespace WAVEENGINE::GRAPHICS {

DEFINE_TYPED_ID(surface_id);

class surface {
public:
	constexpr explicit surface(surface_id id) : _id(id) {}
	constexpr surface() = default;
	constexpr surface_id get_id() const { return _id; }
	constexpr bool is_valid() const { return ID::is_valid(_id); }

	void resize(u32 width, u32 height) const;
	u32 width() const;
	u32 height() const;
	void render() const;
private:
	surface_id _id{ ID::invalid_id };
};

struct render_surface {
	PLATFORM::window window{};
	surface surface{};
};

enum class graphics_platform : u32 {
	Direct3D12 = 0,
	Direct3D11 = 1,
	Vulkan = 2,
	OpenGL = 3
};

bool initialize(graphics_platform platform);
void shutdown();

const char* get_engine_shaders_path();
const char* get_engine_shaders_path(graphics_platform platform);

surface create_surface(PLATFORM::window window);
void remove_surface(surface_id id);
}