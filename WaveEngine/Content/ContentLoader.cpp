#include "ContentLoader.h"
#include "..\Components\Entity.h"
#include "..\Components\Transform.h"
#include "..\Components\Script.h"
#include "Graphics\Renderer.h"

#if !defined(SHIPPING)

#include <fstream>
#include <filesystem>
#include <Windows.h>

namespace WAVEENGINE::CONTENT {

namespace {

enum component_type {
	transform,
	script,

	count
};

UTL::vector<GAME_ENTITY::entity> entities;
TRANSFORM::init_info transform_info{}; // f32: 3, 4, 3
SCRIPT::init_info script_info{};

/*
 * [Transform format]
 * position.x
 * position.y
 * position.z
 * rotation.x
 * rotation.y
 * rotation.z
 * scale.x
 * scale.y
 * scale.z
 */

bool read_transform(const u8*& data, GAME_ENTITY::entity_info& info) {
	using namespace DirectX;
	f32 rotation[3];

	assert(!info.transform);

	memcpy(&transform_info.position[0], data, sizeof(transform_info.position));
	data += sizeof(transform_info.position); // move pointer forward
	memcpy(&rotation[0], data, sizeof(rotation));
	data += sizeof(rotation);
	memcpy(&transform_info.scale[0], data, sizeof(transform_info.scale));
	data += sizeof(transform_info.scale);
	
	// remember to transfer euler coordinates to quaternion coordinates

	XMFLOAT3A rot{ &rotation[0] };
	XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
	XMFLOAT4A rot_quat{};
	XMStoreFloat4A(&rot_quat, quat);
	memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));

	info.transform = &transform_info;

	return true;
}

/*
 * [Script format]
 * script name length
 * script name
 */

bool read_script(const u8*& data, GAME_ENTITY::entity_info& info) {
	assert(!info.script);

	const u32 name_length{ *data };
	data += sizeof(u32);

	if (!name_length)
		return false;

	// something is probably wrong when a script name is longer than 256
	assert(name_length < 256);
	char script_name[256];
	memcpy(&script_name[0], data, name_length);
	data += name_length;

	// make the name a zero_terminated c-string
	script_name[name_length] = 0;

	script_info.script_creator = SCRIPT::DETAIL::get_script_creator(SCRIPT::DETAIL::string_hash()(script_name));
	info.script = &script_info;
	
	return script_info.script_creator != nullptr;
}

using component_reader = bool(*)(const u8*&, GAME_ENTITY::entity_info&);

component_reader component_readers[]{
	read_transform,
	read_script,
};
static_assert(_countof(component_readers) == component_type::count); // runtime check

bool read_file(std::filesystem::path path, std::unique_ptr<u8[]>& data, u64& size) {
	if (!std::filesystem::exists(path)) return false;

	size = std::filesystem::file_size(path);
	assert(size);
	if (!size) return false;
	data = std::make_unique<u8[]>(size);
	std::ifstream file{ path, std::ios::in | std::ios::binary };
	if (!file || !file.read((char*)data.get(), size)) {
		file.close();
		return false;
	}

	file.close();
	return true;
}

} // namespace anonymous

/*
 * [game.bin format]
 * Game Entities Count
 * Game Entity Type
 * Game Entity Components Count
 * Component Type
 * Component Information
 * Component Type
 * Component Information
 * ...
 * ...
 */

bool load_game() {
	// set the working directory to the executable path
	//wchar_t path[MAX_PATH];
	//const u32 length{ GetModuleFileName(0, &path[0], MAX_PATH) };
	//if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	//	return false; 

	//std::filesystem::path p{ path };
	//SetCurrentDirectory(p.parent_path().wstring().c_str());

	// read game.bin and create the entities.
	std::unique_ptr<u8[]> game_data{};
	u64 size{ 0 };
	if (!read_file("game.bin", game_data, size))	return false;
	assert(game_data.get());
	//std::ifstream game("game.bin", std::ios::in | std::ios::binary);
	//UTL::vector<u8> buffer(std::istreambuf_iterator<char>(game), {});
	//assert(buffer.size());

	const u8* at{ game_data.get() };
	constexpr u32 su32{ sizeof(u32) }; 
	const u32 num_entities{ *at }; // Game Entities Count
	at += su32; 
	if (!num_entities) 
		return false;

	for (u32 entity_index{ 0 }; entity_index < num_entities; ++entity_index) {
		GAME_ENTITY::entity_info info{};
		const u32 entity_type{ *at }; // Game Entity Type
		at += su32; 
		const u32 num_components{ *at }; // Game Entity Components Count
		at += su32; 
		if (!num_components)
			return false;

		for (u32 component_index{ 0 }; component_index < num_components; ++component_index) {
			const u32 component_type{ *at }; // Component Type
			at += su32;
			if (!component_readers[component_type](at, info)) // Component Information
				return false;
		}

		assert(info.transform); // at least each entity has transform information
		GAME_ENTITY::entity entity{ GAME_ENTITY::create(info) };
		if (!entity.is_valid())
			return false;

		entities.emplace_back(entity);
	}

	assert(at == game_data.get() + size);
	return true;
}

void unload_game() {
	for (auto& entity : entities) {
		GAME_ENTITY::remove(entity.get_id());
	}
}

bool load_engine_shaders(std::unique_ptr<u8[]>& shaders, u64& size) {
	auto path = GRAPHICS::get_engine_shaders_path();
	return read_file(path, shaders, size);
}

}

#endif