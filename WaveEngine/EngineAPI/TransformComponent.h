#pragma once
#include "..\Components\ComponentsCommon.h"

namespace WAVEENGINE::TRANSFORM {

DEFINE_TYPED_ID(transform_id)

class component final {
public:
	constexpr explicit component(transform_id id) : _id(id) {}
	constexpr component() : _id(ID::invalid_id) {} 
	constexpr transform_id get_id() const { return _id; }
	constexpr bool is_valid() const { return ID::is_valid(_id); }

	MATH::v4 rotation() const;
	MATH::v3 position() const;
	MATH::v3 scale() const;
private:
	transform_id _id;
};

}