#include "Transform.h"
#include "Entity.h"

namespace WAVEENGINE::TRANSFORM {

namespace {

UTL::vector<MATH::v3> positions;
UTL::vector<MATH::v4> rotations;
UTL::vector<MATH::v3> scales;

}

component create(const init_info& info, GAME_ENTITY::entity entity) {
	assert(entity.is_valid());
	const ID::id_type entity_index{ ID::index(entity.get_id()) };
	
	if (positions.size() > entity_index) {
		rotations[entity_index] = MATH::v4(info.rotation);
		positions[entity_index] = MATH::v3(info.position);
		scales[entity_index] = MATH::v3(info.scale);
	} else {
		assert(positions.size() == entity_index);
		rotations.emplace_back(info.rotation);
		positions.emplace_back(info.position);
		scales.emplace_back(info.scale);
	}

	//return component(transform_id{ (ID::id_type)positions.size() - 1 });
	return component{ transform_id{entity.get_id()} };
}

void remove([[maybe_unused]]component c) {
	assert(c.is_valid());

	// TODO 
}

MATH::v4 component::rotation() const {
	assert(is_valid());
	return rotations[ID::index(_id)];
}

MATH::v3 component::position() const {
	assert(is_valid());
	return positions[ID::index(_id)];
}

MATH::v3 component::scale() const{
	assert(is_valid());
	return scales[ID::index(_id)];
}

}
