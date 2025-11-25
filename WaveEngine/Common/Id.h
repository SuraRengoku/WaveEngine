#pragma once
#include "CommonHeaders.h"
#include <iostream>

namespace WAVEENGINE::ID {

// refs: https://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html

using id_type = u32; 

namespace DETAIL {
	
constexpr u32 generation_bits{ 8 };
constexpr u32 index_bits{ sizeof(id_type) * 8 - generation_bits }; // 24
constexpr id_type generation_mask{ (id_type{1} << generation_bits) - 1 }; //0x000000FF
constexpr id_type index_mask{ (id_type{1} << index_bits) - 1 }; // 0x00FFFFFF

}

constexpr id_type invalid_id{ id_type(-1) }; // all invalid
constexpr u32 min_deleted_elements{ 1024 }; // after 1024 removal of elements in the arrar, start write back to the available slots

using generation_type = std::conditional_t<DETAIL::generation_bits <= 16, std::conditional_t<DETAIL::generation_bits <= 8, u8, u16>, u32>;

static_assert(sizeof(generation_type) * 8 >= DETAIL::generation_bits); 
static_assert((sizeof(id_type) - sizeof(generation_type)) > 0);


constexpr bool is_valid(id_type id) {
	return id != invalid_id;
}

constexpr id_type index(id_type id) {
	id_type index = id & DETAIL::index_mask;
	assert(index != DETAIL::index_mask);
	return index;
}

constexpr id_type generation(id_type id) {
	return (id >> DETAIL::index_bits) & DETAIL::generation_mask;
}

constexpr id_type new_generation(id_type id) {
	const id_type generation{ ID::generation(id) + 1 };
	assert(generation < (((u64)1 << DETAIL::generation_bits) - 1)); // make sure generation < 255
	return index(id) | (generation << DETAIL::index_bits);
}

#if _DEBUG
namespace DETAIL {
struct id_base {
  constexpr explicit id_base(id_type id) : _id(id) {}
  constexpr operator id_type() const { return _id; } // back to id_type type

private:
  id_type _id;
};
}

#define DEFINE_TYPED_ID(name)								\
		struct name final : ID::DETAIL::id_base {			\
			constexpr explicit name(ID::id_type id)			\
			:id_base(id) {}									\
			constexpr name() : id_base(0) {}				\
		};			

#else
#define DEFINE_TYPED_ID(name) using name = ID::id_type;
#endif

}