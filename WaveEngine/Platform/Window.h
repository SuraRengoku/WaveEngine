#pragma once

#include "CommonHeaders.h"

namespace WAVEENGINE::PLATFORM {

DEFINE_TYPED_ID(window_id);

class window {
public:
	constexpr explicit window(window_id id) : _id(id) {}
	constexpr window() = default;
	constexpr window_id get_id() const { return _id; }
	constexpr bool is_valid() const { return ID::is_valid(_id); }

	void set_fullscreen(bool is_fullscreen) const;
	bool is_fullscreen() const;
	void* handle() const;
	void set_caption(const wchar_t* caption) const;
	const MATH::u32v4 size() const;
	void resize(u32 width, u32 height) const;
	const u32 width() const;
	const u32 height() const;
	bool is_closed() const;
private:
	window_id _id{ ID::invalid_id };
};

}

