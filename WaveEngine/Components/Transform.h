#pragma once
#include "ComponentsCommon.h"

namespace WAVEENGINE::TRANSFORM {

struct init_info {
	f32 position[3]{};
	f32 rotation[4]{}; // Quaternion
    f32 scale[3]{1.0f, 1.0f, 1.0f};
};

component create(const init_info& info, GAME_ENTITY::entity entity);
void remove(component c);
}
