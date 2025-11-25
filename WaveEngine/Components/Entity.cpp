#include "Entity.h"
#include "Transform.h"
#include "Script.h"
#include <iostream>

namespace WAVEENGINE::GAME_ENTITY {

namespace {

UTL::vector<TRANSFORM::component> transforms;
UTL::vector<SCRIPT::component> scripts;

UTL::vector<ID::generation_type> generations; // record generations for each entity
UTL::deque<entity_id> free_ids; // index free to be used

}

entity create(const entity_info& info) {
	assert(info.transform);
	if (!info.transform)
		return entity{}; // default with invalid_id

	entity_id id;

	if (free_ids.size() > ID::min_deleted_elements) { // reuse id when the number of free ids reach to 1024
		id = free_ids.front();
		assert(!is_alive(id)); // make sure the entity with id is not alive, otherwise it will cause crush later
		free_ids.pop_front();
		id = entity_id{ ID::new_generation(id) }; 
		++generations[ID::index(id)];
	} else {
		id = entity_id{ (ID::id_type)generations.size() }; // next available index
		generations.push_back(0);

		//// resize components
		//// NOTE: we don't call resize(), so the number of memory allocation stays low
		transforms.emplace_back();
		scripts.emplace_back();
	}

	const entity new_entity{ id };
	const ID::id_type index{ ID::index(id) };

	// create transform component 
	assert(!transforms[index].is_valid());
	transforms[index] = TRANSFORM::create(*info.transform, new_entity);
	if (!transforms[index].is_valid())
		return {}; // default with invalid_id

	// Create script component
	if (info.script && info.script-> script_creator) {
		assert(!scripts[index].is_valid());
		scripts[index] = SCRIPT::create(*info.script, new_entity);
		assert(scripts[index].is_valid());
	}

	return new_entity;
}

void remove(entity_id id) {
	const ID::id_type index{ ID::index(id) };
	assert(is_alive(id));

	if (scripts[index].is_valid()) {
		SCRIPT::remove(scripts[index]);
		scripts[index] = {};
	}

	TRANSFORM::remove(transforms[index]);
	transforms[index] = {};

	free_ids.push_back(id);
}

bool is_alive(const entity_id id) {
	assert(ID::is_valid(id)); // check if id is valid 
	const ID::id_type index{ ID::index(id) };
	assert(index < generations.size());
	//assert(generations[index] == ID::generation(id));
	//return (generations[index] == ID::generation(id) && transforms[index].is_valid() && scripts[index].is_valid());
	// not every game entity has a script
	return (generations[index] == ID::generation(id) && transforms[index].is_valid());
}

TRANSFORM::component entity::transform() const {
	assert(is_alive(this->get_id()));
	const ID::id_type index{ ID::index(_id) };
	return transforms[index];
}

SCRIPT::component entity::script() const {
	assert(is_alive(this->get_id()));
	const ID::id_type index{ ID::index(_id) };
	return scripts[index];
}

}