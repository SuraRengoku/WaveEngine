#pragma once
#include "ToolsCommon.h"

namespace WAVEENGINE::TOOLS {

enum primitive_mesh_type : u32 {
	plane,
	cube, 
	uv_sphere,
	ico_sphere,
	cylinder,
	capsule,

	count
};

struct primitive_init_info {
	primitive_mesh_type type;						// basic types
	u32					segments[3]{ 1, 1, 1 };		// segment of detail on each axis
	MATH::v3			size{ 1, 1, 1 };			// size on each axis
	u32					lod{ 0 };					// level of details 
};

}