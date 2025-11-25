#pragma once

#include "ComponentsCommon.h"

namespace WAVEENGINE {

#define INIT_INFO(component) namespace component { struct init_info; } // forward decleration

INIT_INFO(TRANSFORM);
INIT_INFO(SCRIPT);

#undef INIT_INFO

namespace GAME_ENTITY {

struct entity_info {
	TRANSFORM::init_info *transform{ nullptr };
	SCRIPT::init_info *script{ nullptr };
};
	
entity create(const entity_info& info);

void remove(entity_id e);

bool is_alive(entity_id e);

} // namespace GAME_ENTITY
} // namespace WAVEENGINE