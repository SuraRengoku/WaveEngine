#pragma once
#include "ComponentsCommon.h"

namespace WAVEENGINE::SCRIPT {

struct init_info {
	DETAIL::script_creator script_creator;
};

component create(const init_info& info, GAME_ENTITY::entity entity);
void remove(component c);
void update(float dt);
}