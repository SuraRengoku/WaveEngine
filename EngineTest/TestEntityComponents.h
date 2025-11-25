#pragma once

#include "Test.h"
#include "..\WaveEngine\Components\Entity.h"
#include "..\WaveEngine\Components\Transform.h"

#include <iostream>
#include <ctime>

using namespace WAVEENGINE;

class engineTest : public test {
public:
	bool initialize() override { 
		srand((u32)time(nullptr));
		return true; 
	}

	void run() override {
		do {
			for (u32 i{ 0 }; i < 10000; ++i) {
				create_random();
				remove_random();
				_num_entities = (u32)_entities.size();
			}
			print_results();
		} while (getchar() != 'q');
	}

	void shutdown() override {
		// empty
	}

private:

	void create_random() {
		u32 count = rand() % 20;
		if (_entities.empty()) count = 1000;
		TRANSFORM::init_info transform_info{};
		GAME_ENTITY::entity_info entity_info{
			&transform_info,
		};
		while (count > 0) {
			++_added;
			GAME_ENTITY::entity entity{ GAME_ENTITY::create(entity_info) };
			assert(entity.is_valid() && ID::is_valid(entity.get_id()));
			_entities.push_back(entity);
			assert(GAME_ENTITY::is_alive(entity.get_id()));
			--count;
		}

	}

	void remove_random() {
		u32 count = rand() % 20;
		if (_entities.size() < 1000) return;
		while (count > 0) {
			const u32 index{ (u32)rand() % (u32)_entities.size() };
			const GAME_ENTITY::entity entity{ _entities[index] };
			assert(entity.is_valid() && ID::is_valid(entity.get_id()));
			if (entity.is_valid()) {
				GAME_ENTITY::remove(entity.get_id());
				_entities.erase(_entities.begin() + index);
				assert(!GAME_ENTITY::is_alive(entity.get_id()));
				++_removed;
			}
			--count;
		}
	}

	void print_results() {
		std::cout << "Entities created " << _added << "\n";
		std::cout << "Entities removed " << _removed << "\n";
	}

	UTL::vector<GAME_ENTITY::entity> _entities;
	
	u32 _added{ 0 };
	u32 _removed{ 0 };
	u32 _num_entities{ 0 };
};