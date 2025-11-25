#pragma once

#include "..\Components\ComponentsCommon.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"

namespace WAVEENGINE {
namespace GAME_ENTITY {

DEFINE_TYPED_ID(entity_id);

class entity {
public:
	constexpr explicit entity(entity_id id) : _id(id) {}
	constexpr entity() : _id(ID::invalid_id) {}
	constexpr entity_id get_id() const { return _id; }
	constexpr bool is_valid() const { return ID::is_valid(_id); }

	TRANSFORM::component transform() const;
	SCRIPT::component script() const;

private:
	entity_id _id;
};
} // namespace GAME_ENTITY

namespace SCRIPT {

class entity_script : public GAME_ENTITY::entity {
public:
	virtual ~entity_script() = default;
	virtual void begin_play() {}
	virtual void update(float) {}
protected:
	constexpr explicit entity_script(GAME_ENTITY::entity entity) 
		: GAME_ENTITY::entity(entity.get_id()) {}

};

namespace DETAIL {

using script_ptr = std::unique_ptr<entity_script>;
using script_creator = script_ptr(*)(GAME_ENTITY::entity entity);
using string_hash = std::hash<std::string>;

u8 register_script(size_t, script_creator);

#ifdef USE_WITH_EDITOR
extern "C" __declspec(dllexport)
#endif
script_creator get_script_creator(size_t tag);


template<typename script_class>
script_ptr create_script(GAME_ENTITY::entity entity) {
	assert(entity.is_valid());
	return std::make_unique<script_class>(entity);
}

#ifdef USE_WITH_EDITOR
u8 add_script_name(const char* name);
#define REGISTER_SCRIPT(TYPE)														\
		namespace {																	\
		const u8 _reg_##TYPE														\
		{ WAVEENGINE::SCRIPT::DETAIL::register_script(								\
			WAVEENGINE::SCRIPT::DETAIL::string_hash()(#TYPE),						\
			&WAVEENGINE::SCRIPT::DETAIL::create_script<TYPE>) };					\
		const u8 _name_##TYPE														\
		{ WAVEENGINE::SCRIPT::DETAIL::add_script_name(#TYPE) };						\
		}

#else

#define REGISTER_SCRIPT(TYPE)														\
		namespace {																	\
		const u8 _reg_##TYPE														\
		{ WAVEENGINE::SCRIPT::DETAIL::register_script(								\
			WAVEENGINE::SCRIPT::DETAIL::string_hash()(#TYPE),						\
			&WAVEENGINE::SCRIPT::DETAIL::create_script<TYPE>) };					\
		}

#endif

} // namespace DETAIL

} // namespace SCRIPT
}