#include "PrimitiveMesh.h"
#include "Geometry.h"

namespace WAVEENGINE::TOOLS {

namespace {

using namespace MATH;
using namespace DirectX;
using primitive_mesh_creator = void(*)(scene&, const primitive_init_info&);

void create_plane(scene& scene, const primitive_init_info& info);
void create_cube(scene& scene, const primitive_init_info& info);
void create_uv_sphere(scene& scene, const primitive_init_info& info);
void create_ico_sphere(scene& scene, const primitive_init_info& info);
void create_cylinder(scene& scene, const primitive_init_info& info);
void create_capsule(scene& scene, const primitive_init_info& info);

primitive_mesh_creator creators[]{
	create_plane,
	create_cube,
	create_uv_sphere,
	create_ico_sphere,
	create_cylinder,
	create_capsule
};

static_assert(_countof(creators) == primitive_mesh_type::count);

struct axis {
	enum : u32 {
		x = 0,
		y = 1,
		z = 2
	};
};

mesh create_plane(const primitive_init_info& info, u32 horizontal_index = axis::x, u32 vertical_index = axis::z, bool flip_winding = false, 
				  v3 offset = { -0.5f, 0.0f, -0.5f }, v2 u_range = { 0.0f, 1.0f }, v2 v_range = { 0.0f, 1.0f }) {
	assert(horizontal_index < 3 && vertical_index < 3);
	assert(horizontal_index != vertical_index);

	const u32 horizontal_count{ clamp(info.segments[horizontal_index], 1u, 10u) }; // at most 10 segments, at least 1 segment 
	const u32 vertical_count{ clamp(info.segments[vertical_index], 1u, 10u) }; 
	const f32 horizontal_step{ 1.0f / horizontal_count };
	const f32 vertical_step{ 1.0f / vertical_count };
	const f32 u_step{ (u_range.y - u_range.x) / horizontal_count };
	const f32 v_step{ (v_range.y - v_range.x) / vertical_count };
	const u32 num_positions{ (horizontal_count + 1) * (vertical_count + 1) };

	mesh m{};
	m.name = "plane";
	m.positions.reserve(num_positions);
	UTL::vector<v2> uvs;

	// on each level(horizontal / vertical), we have 1 more points than the number of segments
	for (u32 j{ 0 }; j <= vertical_count; ++j) {
		for (u32 i{ 0 }; i <= horizontal_count; ++i) {
			v3 position{ offset };
			f32* const as_array{ &position.x }; 
			as_array[horizontal_index] += i * horizontal_step;
			as_array[vertical_index] += j * vertical_step;
			/* feeling weird? look below
			 * 
			 * position.x == as_array[0]
			 * position.y == as_array[1]
			 * position.z == as_array[2]
			 * 
			 */
			m.positions.emplace_back(position.x * info.size.x, position.y * info.size.y, position.z * info.size.z); // times scaling factor

			v2 uv{ u_range.x, 1.0f - v_range.x }; // fliping v
			uv.x += i * u_step;
			uv.y -= j * v_step;

			//blow for segments test
			//v2 uv{ 0, 1.0f }; 
			//uv.x += (i % 2);
			//uv.y -= (j % 2);

			uvs.emplace_back(uv);
		}
	}

	assert(m.positions.size() == (((u64)horizontal_count + 1) * ((u64)vertical_count + 1)));

	const u32 num_indices{ 3 * 2 * horizontal_count * vertical_count };
	m.raw_indices.reserve(num_indices);

	const u32 row_length{ horizontal_count + 1 }; // number of vertices in a row
	for (u32 j{ 0 }; j < vertical_count; ++j) { /* �� */
		u32 k{ 0 };
		for (u32 i{ k }; i < horizontal_count; ++i) { /* -> */
			/* counter clockwise vertices permutation
			 *  a <------- c
			 *	|		   ��
			 *	|		   |
			 *	��		   |
			 *	b -------> d
			 */
			const u32 index[4]{
				i + j * row_length,					// a 0
				i + (j + 1) * row_length,			// b 1
				(i + 1) + j * row_length,			// c 2
				(i + 1) + (j + 1) * row_length		// d 3
			};

			m.raw_indices.emplace_back(index[0]);
			m.raw_indices.emplace_back(index[flip_winding ? 2 : 1]);
			m.raw_indices.emplace_back(index[flip_winding ? 1 : 2]);

			m.raw_indices.emplace_back(index[2]);
			m.raw_indices.emplace_back(index[flip_winding ? 3 : 1]);
			m.raw_indices.emplace_back(index[flip_winding ? 1 : 3]);
		}
		++k;
	}

	assert(m.raw_indices.size() == num_indices);

	m.uv_sets.resize(1);

	for (u32 i{ 0 }; i < num_indices; ++i) {
		m.uv_sets[0].emplace_back(uvs[m.raw_indices[i]]);
	}
	
	return m;
}

mesh create_uv_sphere(const primitive_init_info& info) {
	const u32 phi_count{ clamp(info.segments[axis::x], 3u, 64u) }; // when 3u, it looks like a triangle shape from top/botton view -> horizontal
	const u32 theta_count{ clamp(info.segments[axis::y], 2u, 64u) }; // when 2u, it looks like a diamond shape from side view -> vertical
	const f32 phi_step{ tau / phi_count };
	const f32 theta_step{ pi / theta_count };
	const u32 num_positions{ 2 + phi_count * (theta_count - 1) };
	const u32 num_indices{ 2 * 3 * phi_count + 2 * 3 * phi_count * (theta_count - 2) };

	mesh m{};
	m.name = "uv_sphere";
	m.positions.resize(num_positions);

	// add the top position
	u32 c{ 0 };
	m.positions[c++] = { 0.0f, info.size.y, 0.0f };

	for (u32 j{ 1 }; j <= (theta_count - 1); ++j) {
		const f32 theta{ j * theta_step };
		for (u32 i{ 0 }; i < phi_count; ++i) {
			const f32 phi{ i * phi_step };
			// right-handed coordinated system 
			m.positions[c++] = {
				info.size.x * XMScalarSin(theta) * XMScalarCos(phi),
				info.size.y * XMScalarCos(theta),
				-info.size.z * XMScalarSin(theta) * XMScalarSin(phi)
			};
		}
	}

	// add the bottom position
	m.positions[c++] = { 0.0f, -info.size.y, 0.0f };
	assert(c == num_positions);

	c = 0;
	m.raw_indices.resize(num_indices);
	UTL::vector<v2> uvs(num_indices);
	const f32 inv_theta_count{ 1.0f / theta_count };
	const f32 inv_phi_count{ 1.0f / phi_count };
	
	// indices for the top cap, connecting the north pole to the first rings
	for (u32 i{ 0 }; i < phi_count - 1; ++i) {
		uvs[c] = { (2 * i + 1) * 0.5f * inv_phi_count, 1.0f };
		m.raw_indices[c++] = 0;
		uvs[c] = { i * inv_phi_count, 1.0f - inv_theta_count };
		m.raw_indices[c++] = i + 1;
		uvs[c] = { (i + 1) * inv_phi_count, 1.0f - inv_theta_count };
		m.raw_indices[c++] = i + 2;
	}

	uvs[c] = { 1.0f - 0.5f * inv_phi_count, 1.0f };
	m.raw_indices[c++] = 0;
	uvs[c] = { 1.0f - inv_phi_count, 1.0f - inv_theta_count };
	m.raw_indices[c++] = phi_count;
	uvs[c] = { 1.0f, 1.0f - inv_theta_count };
	m.raw_indices[c++] = 1;

	// indices for the section between top cap and bottom cap
	for (u32 j{ 0 }; j < (theta_count - 2); ++j) {
		for (u32 i{ 0 }; i < (phi_count - 1); ++i) { // inner: move horizontally
			const u32 index[4]{
				1 + i + j * phi_count,
				1 + i + (j + 1) * phi_count,
				1 + (i + 1) + (j + 1) * phi_count,
				1 + (i + 1) + j * phi_count
			};
			
			uvs[c] = { i * inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
			m.raw_indices[c++] = index[0];
			uvs[c] = { i * inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
			m.raw_indices[c++] = index[1];
			uvs[c] = { (i + 1) * inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
			m.raw_indices[c++] = index[2];
			
			uvs[c] = { i * inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
			m.raw_indices[c++] = index[0];
			uvs[c] = { (i + 1) * inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
			m.raw_indices[c++] = index[2];
			uvs[c] = { (i + 1) * inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
			m.raw_indices[c++] = index[3];
		}

		const u32 index[4]{
			phi_count + j * phi_count,
			phi_count + (j + 1) * phi_count,
			1 + (j + 1) * phi_count,
			1 + j * phi_count
		};

		uvs[c] = { 1.0f - inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
		m.raw_indices[c++] = index[0];
		uvs[c] = { 1.0f - inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
		m.raw_indices[c++] = index[1];
		uvs[c] = { 1.0f, 1.0f - (j + 2) * inv_theta_count };
		m.raw_indices[c++] = index[2];

		uvs[c] = { 1.0f - inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
		m.raw_indices[c++] = index[0];
		uvs[c] = { 1.0f, 1.0f - (j + 2) * inv_theta_count };
		m.raw_indices[c++] = index[2];
		uvs[c] = { 1.0f, 1.0f - (j + 1) * inv_theta_count };
		m.raw_indices[c++] = index[3];
	}

	// indices for the bottom cap, again connecting the north pole to the first rings
	const u32 south_pole_index{ (u32)m.positions.size() - 1 };
	for (u32 i{ 0 }; i < phi_count - 1; ++i) {
		uvs[c] = { (2 * i + 1) * 0.5f * inv_phi_count, 0.0f };
		m.raw_indices[c++] = south_pole_index;
		uvs[c] = { (i + 1) * inv_phi_count, inv_theta_count };
		m.raw_indices[c++] = south_pole_index - phi_count + i + 1;
		uvs[c] = { i * inv_phi_count, inv_theta_count };
		m.raw_indices[c++] = south_pole_index - phi_count + i;
	}

	uvs[c] = { 1.0f - 0.5f * inv_phi_count, 0.0f };
	m.raw_indices[c++] = south_pole_index;
	uvs[c] = { 1.0f, inv_theta_count };
	m.raw_indices[c++] = south_pole_index - phi_count;
	uvs[c] = { 1.0f - inv_phi_count, inv_theta_count };
	m.raw_indices[c++] = south_pole_index - 1;
	
	assert(c == num_indices);

	m.uv_sets.emplace_back(uvs);

	return m;
}

mesh create_cube(const primitive_init_info& info) {
	// TODO: fix the uv coordinates
	mesh m{};
	m.name = "cube";

	auto& p = m.positions;
	auto& indices = m.raw_indices;
	UTL::vector<v2> uvs;

	// Front face(XY plane, facing +Z)
	mesh front = create_plane(info, axis::x, axis::y, true, { -0.5f, -0.5f, 0.5f });
	// Back face(XY plane, facing -Z)
	mesh back = create_plane(info, axis::x, axis::y, false, { -0.5f, -0.5f, -0.5f });
	// Left face(ZY plane, facing -X)
	mesh left = create_plane(info, axis::z, axis::y, true, { -0.5f, -0.5f, -0.5f });
	// Right face(ZY plane, facing +X)
	mesh right = create_plane(info, axis::z, axis::y, false, { 0.5f, -0.5f, -0.5f });
	// Top face(XZ plane, facing +Y)
	mesh top = create_plane(info, axis::x, axis::z, false, { -0.5f, 0.5f, -0.5f });
	// Bottom face(XZ plane, facing -Y)
	mesh bottom = create_plane(info, axis::x, axis::z, true, { -0.5f, -0.5f, -0.5f });

	mesh faces[] = { front, back, left, right, top, bottom };

	for (const auto& face : faces) {
		const u32 vertex_offset = (u32)p.size();
		p.insert(p.end(), face.positions.begin(), face.positions.end());

		for (u32 idx : face.raw_indices) {
			indices.emplace_back(idx + vertex_offset);
		}

		uvs.insert(uvs.end(), face.uv_sets[0].begin(), face.uv_sets[0].end());
	}

	m.uv_sets.emplace_back(uvs);

	return m;
}

void create_plane(scene& scene, const primitive_init_info& info) {
	lod_group lod{};
	lod.name = "plane";
	lod.meshes.emplace_back(create_plane(info));
	scene.lod_groups.emplace_back(lod);
}

void create_cube(scene& scene, const primitive_init_info& info) {
	lod_group lod{};
	lod.name = "cube";
	lod.meshes.emplace_back(create_cube(info));
	scene.lod_groups.emplace_back(lod);
}

void create_uv_sphere(scene& scene, const primitive_init_info& info) {
	lod_group lod{};
	lod.name = "uv_sphere";
	lod.meshes.emplace_back(create_uv_sphere(info));
	scene.lod_groups.emplace_back(lod);
}

void create_ico_sphere(scene& scene, const primitive_init_info& info) {

}

void create_cylinder(scene& scene, const primitive_init_info& info) {

}

void create_capsule(scene& scene, const primitive_init_info& info) {

}

}

EDITOR_INTERFACE
void CreatePrimitiveMesh(scene_data* data, primitive_init_info* info) {
	assert(data && info);
	assert(info->type < primitive_mesh_type::count);
	scene scene{};
	creators[info->type](scene, *info);

	data->settings.calculate_normals = 1;
	process_scene(scene, data->settings);
	pack_data(scene, *data);
}

}