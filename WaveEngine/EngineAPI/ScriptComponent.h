#pragma once
#include "..\Components\ComponentsCommon.h"

namespace WAVEENGINE::SCRIPT {

DEFINE_TYPED_ID(script_id)

class component final {
public:
	constexpr explicit component(script_id id) : _id(id) {}
	constexpr component() : _id(ID::invalid_id) {}
	constexpr script_id get_id() const { return _id; }
	constexpr bool is_valid() const { return ID::is_valid(_id); }

private:
	script_id _id;
};

}