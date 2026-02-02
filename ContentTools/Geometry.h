#pragma once
#include "ToolsCommon.h"

namespace WAVEENGINE::TOOLS {

namespace PACKED_VERTEX {

struct vertex_static {
	MATH::v3		position;
	u8				reserved[3];
	u8				t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
	u16				normal[2]; // we can use x and y component to calculate the z component
	u16				tangent[2]; // same as normal
	MATH::v2		uv;
};

}

struct vertex {
	MATH::v4 tangent{};
	MATH::v3 position{};
	MATH::v3 normal{};
	MATH::v2 uv{};
};

struct mesh {
	// Initial data
	UTL::vector<MATH::v3>						positions;
	UTL::vector<MATH::v3>						normals;
	UTL::vector<MATH::v4>						tangents;

	UTL::vector<UTL::vector<MATH::v2>>			uv_sets; // a list of uv coordinates, we could in theory different texture on a single point
	UTL::vector<u32>							material_indices;
	UTL::vector<u32>							material_used;

	UTL::vector<u32>							raw_indices;

	// Intermediate data
	UTL::vector<vertex>							vertices;
	UTL::vector<u32>							indices;

	// Output data
	std::string									name;
	UTL::vector<PACKED_VERTEX::vertex_static>	pack_vertices_static; 
	f32											lod_threshold{ -1.0f };
	u32											lod_id{ u32_invalid_id };
};

struct lod_group {
	std::string			name;
	UTL::vector<mesh>	meshes;
};

struct scene {
	std::string				name;
	UTL::vector<lod_group>	lod_groups;
};

struct geometry_import_settings {
	f32 smoothing_angle;
	u8	calculate_normals;
	u8	calculate_tangents;
	u8	reverse_handedness;
	u8	import_embeded_textures;
	u8	import_animations;
};

struct scene_data {
	u8*						 buffer;
	u32						 buffer_size;
	geometry_import_settings settings;
};

void process_scene(scene& scene, const geometry_import_settings& settings);

void pack_data(const scene& scene, scene_data& data);

}