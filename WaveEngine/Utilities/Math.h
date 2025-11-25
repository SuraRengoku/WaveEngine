#pragma once

#include "CommonHeaders.h"
#include "PrimitiveTypes.h"
#include "MathTypes.h"

namespace WAVEENGINE::MATH {

template<typename T>
constexpr T clamp(T value, T min, T max) {
	return (value < min) ? min : (value > max) ? max : value;
}

/// <summary>
/// [0.0, 1.0] -> [0, 2^bits - 1]
/// packed_value = round(float_value x (2^bits - 1))
/// </summary>
/// <typeparam name="bits">packed bits</typeparam>
/// <param name="f">input float value</param>
/// <returns>packed u32 value</returns>
template<u32 bits>
constexpr u32 pack_unit_float(f32 f) {
	static_assert(bits <= sizeof(u32) * 8); // check bits <= 32
	assert(f >= 0.0f && f <= 1.0f); // check f in [0.0, 1.0]
	constexpr f32 intervals{ (f32)((1ui32 << bits) - 1) }; // calculate the maximum value (bits == 8 -> interval == 255.0)
	return (u32)(intervals * f + 0.5f); 
}

/// <summary>
/// [0, 2^bits - 1] -> [0.0, 1.0]
/// float_value = packed_value / (2^bits - 1)
/// </summary>
/// <typeparam name="bits">packed bits</typeparam>
/// <param name="i">input u32 value</param>
/// <returns>unpacked float value</returns>
template<u32 bits>
constexpr f32 unpack_to_unit_float(u32 i) {
	static_assert(bits <= sizeof(u32) * 8); // check bits <= 32
	assert(i < (1ui32 << bits)); // check i < 2^bits
	constexpr f32 intervals{ (f32)((1ui32 << bits) - 1) }; // calculate the maximum value
	return (f32)i / intervals; // normalization
}

template<u32 bits>
constexpr u32 pack_float(f32 f, f32 min, f32 max) {
	assert(min < max);
	assert(f <= max && f >= min);
	const f32 distance{ (f - min) / (max - min) };
	return pack_unit_float<bits>(distance);
}

template<u32 bits>
constexpr f32 unpack_float(u32 i, f32 min, f32 max) {
	assert(min < max);
	const f32 origin = unpack_to_unit_float<bits>(i) * (max - min) + min;
	assert(origin >= min && origin <= max);
	return origin;
}

}