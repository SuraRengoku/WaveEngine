#pragma once
#include "ToolsCommon.h"

namespace WAVEENGINE::TOOLS {

namespace PACKED_VERTEX {

struct [[deprecated("use dynamic combination")]] vertex_static {
	MATH::v3			position;
	u8				reserved[3];
	u8				t_sign;			// bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
	u16				normal[2];		// we can use x and y component to calculate the z component
	u16				tangent[2];		// same as normal
	MATH::v2			uv;
};

}

struct vertex {
	MATH::v4		tangent{};
	MATH::v4		joint_weight{};
	MATH::u32v4	joint_indices{ u32_invalid_id, u32_invalid_id, u32_invalid_id, u32_invalid_id };
	MATH::v3		position{};
	MATH::v3		normal{};
	MATH::v2		uv{};
	u8			red{}, green{}, blue{};
	u8			pad;
};

namespace ELEMENTS {

struct elements_type {
	enum type : u32 {
		position_only = 0x00,
		static_normal = 0x01,
		static_normal_texture = 0x03,
		static_color = 0x04,
		skeletal = 0x08,
		skeletal_color = skeletal | static_color,
		skeletal_normal = skeletal | static_normal,
		skeletal_normal_color = skeletal_normal | static_color, 
		skeletal_normal_texture = skeletal | static_normal_texture,
		skeletal_normal_texture_color = skeletal_normal_texture | static_color,
	};
};

struct static_color {
	u8			color[3];
	u8			pad;
};

struct static_normal {
	u8			color[3];	
	u8			t_sign;				// bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign(0 means -1, 1 means + 1).
	u16			normal[2];
};

struct static_normal_texture {
	u8			color[3];
	u8			t_sign;				// bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign(0 means -1, 1 means + 1).
	u16			normal[2];
	u16			tangent[2];
	MATH::v2		uv;
}; 

struct skeletal {
	u8			joint_weights[3];		// normalized joint weights for up to 4 joints
	u8			pad;
	u16			joint_indices[4];
};

struct skeletal_color {
	u8			joint_weights[3];		// normalized joint weights for up to 4 joints
	u8			pad;
	u16			joint_indices[4];
	u8			color[3];
	u8			pad2;
};

struct skeletal_normal {
	u8			joint_weights[3];		// normalized joint weights for up to 4 joints
	u8			t_sign;				// bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign(0 means -1, 1 means + 1).
	u16			joint_indices[4];
	u16			normal[2];
};

struct skeletal_normal_color {
	u8			joint_weights[3];		// normalized joint weights for up to 4 joints
	u8			t_sign;				// bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign(0 means -1, 1 means + 1).
	u16			joint_indices[4];
	u16			normal[2];
	u8			color[3];
	u8			pad;
};

struct skeletal_normal_texture {
	u8			joint_weights[3];		// normalized joint weights for up to 4 joints
	u8			t_sign;				// bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign(0 means -1, 1 means + 1).
	u16			joint_indices[4];
	u16			normal[2];
	u16			tangent[2];
	MATH::v2		uv;
};

struct skeletal_normal_texture_color {
	u8			joint_weights[3];		// normalized joint weights for up to 4 joints
	u8			t_sign;				// bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign(0 means -1, 1 means + 1).
	u16			joint_indices[4];
	u16			normal[2];
	u16			tangent[2];
	MATH::v2		uv;
	u8			color[3];
	u8			pad;
};

}

struct mesh {
	// Initial data
	UTL::vector<MATH::v3>						positions;
	UTL::vector<MATH::v3>						normals;
	UTL::vector<MATH::v4>						tangents;
	UTL::vector<MATH::v3>						colors; // vertex color

	UTL::vector<UTL::vector<MATH::v2>>			uv_sets; // a list of uv coordinates, we could in theory different texture on a single point
	UTL::vector<u32>							material_indices;
	UTL::vector<u32>							material_used;

	UTL::vector<u32>							raw_indices;

	// Intermediate data
	UTL::vector<vertex>						vertices;
	UTL::vector<u32>							indices;

	// Output data
	std::string								name;
	ELEMENTS::elements_type::type				elements_type;
	UTL::vector<u8>							position_buffer;
	UTL::vector<u8>							element_buffer;

	f32										lod_threshold{ -1.0f };
	u32										lod_id{ u32_invalid_id };
};

struct lod_group {
	std::string			name;
	UTL::vector<mesh>		meshes;
};

struct scene {
	std::string				name;
	UTL::vector<lod_group>		lod_groups;
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
	geometry_import_settings	 settings;
};

void process_scene(scene& scene, const geometry_import_settings& settings);

void pack_data(const scene& scene, scene_data& data);

}