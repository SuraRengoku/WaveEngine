#include "Common.h"
#include "CommonHeaders.h"
#include "Id.h"
#include "..\WaveEngine\Components\Entity.h"
#include "..\WaveEngine\Components\Transform.h"
#include "..\WaveEngine\Components\Script.h"

using namespace WAVEENGINE;

namespace {

struct transform_component {
	f32 position[3];
	f32 rotation[3];
	f32 scale[3];

	TRANSFORM::init_info to_init_info() {
		using namespace DirectX;
		TRANSFORM::init_info info{};
		memcpy(&info.position[0], &position[0], sizeof(position));
		memcpy(&info.scale[0], &scale[0], sizeof(scale));
		XMFLOAT3A rot{ &rotation[0] };
		XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
		XMFLOAT4A rot_quat{};
		XMStoreFloat4A(&rot_quat, quat); //SIMD
		memcpy(&info.rotation[0], &rot_quat.x, sizeof(info.rotation));
		return info;
	}
};

struct script_component {
	SCRIPT::DETAIL::script_creator script_creator;

	SCRIPT::init_info to_init_info() {
		SCRIPT::init_info info{};
		info.script_creator = script_creator;
		return info;
	}
};

struct game_entity_descriptor {
	transform_component transform;
	script_component script;
};

GAME_ENTITY::entity entity_from_id(ID::id_type id) {
	return GAME_ENTITY::entity{
		GAME_ENTITY::entity_id{id}
	};
}

}

EDITOR_INTERFACE
ID::id_type CreateGameEntity(game_entity_descriptor* e) {
	assert(e);
	game_entity_descriptor& desc{ *e };
	TRANSFORM::init_info transform_info{ desc.transform.to_init_info() };
	SCRIPT::init_info script_info{ desc.script.to_init_info() };
	GAME_ENTITY::entity_info entity_info{
		&transform_info,
		&script_info,
	};
	return GAME_ENTITY::create(entity_info).get_id();
}


EDITOR_INTERFACE
void RemoveGameEntity(ID::id_type id) {
	assert(ID::is_valid(id));
	GAME_ENTITY::remove(GAME_ENTITY::entity_id{ id });
}
