#include "Script.h"
#include "Entity.h"

namespace WAVEENGINE::SCRIPT {

namespace {

UTL::vector<DETAIL::script_ptr> entity_scripts;
UTL::vector<ID::id_type> id_mapping;

UTL::vector<ID::generation_type> generations;
UTL::deque<script_id> free_ids;

using script_registry = std::unordered_map<size_t, DETAIL::script_creator>;

script_registry& registery() {
	/*
		NOTE:	We put this static variable in a function because of the initialization order of static data.
				This way, we can be certain that the data is initialized before accessing it.
	*/
	static script_registry reg;
	return reg;
}

#ifdef USE_WITH_EDITOR
UTL::vector<std::string>& script_names() {
	/*
	NOTE:	We put this static variable in a function because of the initialization order of static data.
			This way, we can be certain that the data is initialized before accessing it.
	*/
	static UTL::vector<std::string> names; // singleton pattern
	return names;
}
#endif // USE_WITH_EDITOR

bool exists(script_id id) {
	assert(ID::is_valid(id));
	const ID::id_type index{ ID::index(id) };
	assert(index < generations.size() && id_mapping[index] < entity_scripts.size());
	assert(generations[index] == ID::generation(id));
	return (generations[index] == ID::generation(id)) && entity_scripts[id_mapping[index]] && entity_scripts[id_mapping[index]]->is_valid();
}

}

namespace DETAIL {

u8 register_script(size_t tag, script_creator func) {
	bool result{ registery().insert(script_registry::value_type{tag, func}).second }; 
	assert(result);
	return result;
}

script_creator get_script_creator(size_t tag)
{
	auto script = WAVEENGINE::SCRIPT::registery().find(tag);
	assert(script != WAVEENGINE::SCRIPT::registery().end() && script->first == tag);
	return script->second;
}

#ifdef USE_WITH_EDITOR
u8 add_script_name(const char* name)
{
	script_names().emplace_back(name);
	return true;
}
#endif

}

component create(const init_info& info, GAME_ENTITY::entity entity) {
	assert(entity.is_valid());
	assert(info.script_creator);

	script_id id{};
	
 	if (free_ids.size() > ID::min_deleted_elements) {
		id = free_ids.front();
		assert(!exists(id));
		free_ids.pop_front();
		id = script_id(ID::new_generation(id));
		++generations[ID::index(id)];
	}
	else {
		id = script_id{ (ID::id_type)id_mapping.size() };
		id_mapping.emplace_back(id);
		generations.push_back(0);
	}

	assert(ID::is_valid(id));
	const ID::id_type index{ (ID::id_type)entity_scripts.size() }; // just add a script element
	entity_scripts.emplace_back(info.script_creator(entity));
	assert(entity_scripts.back()->get_id() == entity.get_id());
	id_mapping[ID::index(id)] = index;

	return component{id};
}

void remove(component c) {
	assert(c.is_valid() && exists(c.get_id()));
	const script_id id{ c.get_id() };
	const ID::id_type index{ id_mapping[ID::index(id)] };

	script_id last_id{};
	if (entity_scripts.size() > 1) {
		last_id = entity_scripts.back()->script().get_id();
	}

	UTL::erase_unordered(entity_scripts, index);

	if (entity_scripts.size() > index) {
		id_mapping[ID::index(last_id)] = index;
	}
	id_mapping[ID::index(id)] = ID::invalid_id;
	free_ids.push_back(id);
}

void update(float dt) {
	for (auto& ptr : entity_scripts) {
		ptr->update(dt);
	}
}

}

#ifdef USE_WITH_EDITOR
#include <atlsafe.h>

/*
     C++ Engine         =>           C# Editor
   script_names()       =>      string[] scriptNames
	    ||                               ||
CComSafeArray<BSTR>     =>          LPSAFEARRAY
	    ||                               ||
     Detach()           =>         Marshal.Copy()
*/

extern "C" __declspec(dllexport)
LPSAFEARRAY get_script_names() {
	const u32 size{ (u32)WAVEENGINE::SCRIPT::script_names().size() };
	if (!size)
		return nullptr;
	CComSafeArray<BSTR> names(size);
	for (u32 i{ 0 }; i < size; ++i) {
		names.SetAt(i, A2BSTR_EX(WAVEENGINE::SCRIPT::script_names()[i].c_str()), false);
	}
	return names.Detach(); // include GC
}

#endif // USE_WITH_EDITOR