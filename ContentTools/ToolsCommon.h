#pragma once
#include "CommonHeaders.h"
#include <type_traits>

#ifdef _WIN32
#include <combaseapi.h>
#endif

#ifndef EDITOR_INTERFACE
#define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif // EDITOR_INTERFACE

template<typename sizeT>
inline void* alloc_interop_memory(sizeT size) {
	static_assert(
		std::is_same_v<sizeT, u64> ||
		std::is_same_v < sizeT, u32> ||
		std::is_same_v<sizeT, size_t>,
		"alloc_interop_memory only accepts u64, u32, or size_t"
		);

	// no overflow
	if constexpr (sizeof(sizeT) > sizeof(size_t)) {
		if (size > static_cast<sizeT>(SIZE_MAX)) {
			return nullptr;
		}
	}

	// CoTaskMemAlloc is a windows com api
	// it is safe to use between modules, dlls and coms, and it is also thread-safe
	// However it is 15% slower than malloc because its bookkeeping and thread synchronization overhead
#ifdef _WIN32
	return CoTaskMemAlloc(size);
#else
	return malloc(size);
#endif
}

inline void free_interop_memory(void* ptr) {
#ifdef _WIN32
	CoTaskMemFree(ptr);
#else
	free(ptr);
#endif
}
