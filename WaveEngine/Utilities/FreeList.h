#pragma once
#include "CommonHeaders.h"

namespace WAVEENGINE::UTL {

#if USE_STL_VECTOR
#pragma message("WARNING: using UTL::freeList with std::vector results in duplicate calls to class destructor!")
#endif

template<typename T>
class freeList {
	// we have to make sure that each slot have enough space to store free list pointer
	static_assert(sizeof(T) >= sizeof(u32));

public:
	freeList() = default;

	explicit freeList(u32 count) {
		_array.reserve(count);
	}

	~freeList() { 
		assert(!_size);

		// if we use std::vector, _array will call ~T() when the destructing the vector itself.
		// However, the item.~T() has already been called in remove(u32 id) function, which will cause duplicate destructing.
		// for items that have already been destructed, if they had pointers, there might be addresses left on the memory.
		// When std::vector call destructor, it will try destruct these "ghost" object again.
#if USE_STL_VECTOR
		memset(_array.data(), 0, _array.size() * sizeof(T));
#endif
	}

	template<class... params>
	constexpr u32 add(params&&... p) {
		u32 id{ u32_invalid_id };
		if (_next_free_index == u32_invalid_id) { // no free slots, expand space
			id = static_cast<u32>(_array.size());
			_array.emplace_back(std::forward<params>(p)...);
		}
		else { // reuse free slots
			id = _next_free_index;
			assert(id < _array.size() && already_removed(id));
			_next_free_index = *(const u32 *const)std::addressof(_array[id]);
			new (std::addressof(_array[id])) T(std::forward<params>(p)...);
		}
		++_size;
		return id;
	}

	constexpr void remove(u32 id) {
		assert(id < _array.size() && !already_removed(id));
		T& item{ _array[id] };
		item.~T(); 
		// this step may destroy virtual pointer and dangling pointer
		DEBUG_OP(memset(std::addressof(_array[id]), 0xcc, sizeof(T)));
		// set the first 4 bytes as the _next_free_index for next allocation
		// this step will overwrite the memory. if T contains pointer, virtual pointer, smart pointer etc., they will be covered by u32 data
		*(u32 *const)std::addressof(_array[id]) = _next_free_index; 
		// set the _next_free_index as current removed id for 
		// 1. if next step is removal, fill the _next_free_index into the first bytes of next removed slot, then iterate
		// 2. if next step is allocation, use _next_free_index as an index to point out closest free slot
		_next_free_index = id;
		--_size;
	}

	constexpr u32 size() const {
		return _size;
	}

	constexpr u32 capacity() const {
		return _array.size();
	}

	constexpr u32 empty() const {
		return _size == 0;
	}

	[[nodiscard]] constexpr T& operator[](u32 id) {
		assert(id < _array.size() && !already_removed(id));
		return _array[id];
	}

	[[nodiscard]] constexpr const T& operator[](u32 id) const {
		assert(id < _array.size() && !already_removed(id));
		return _array[id];
	}

private:
	constexpr bool already_removed(u32 id) {
		// TODO: fix when sizeof(T) == sizeof(u32) we can't test if the item was already removed!
		if constexpr (sizeof(T) > sizeof(u32)) {
			u32 i{ sizeof(u32) }; // skip the first 4 bytes
			const u8 *const p{ (const u8 *const)std::addressof(_array[id]) };
			while ((p[i] == 0xcc) && (i < sizeof(T))) ++i;
			return i == sizeof(T); // all bytes after the first 4 bytes are 0xcc, which means that this item is removed
		}
		else { // sizeof(T) == sizeof(u32)
			return true;
		}
	}


#if USE_STL_VECTOR
	UTL::vector<T>				_array;
#else
	UTL::vector<T, false>		_array;
#endif
	u32							_next_free_index{ u32_invalid_id };
	u32							_size{ 0 }; // number of active object
};

} // WAVEENGINE::UTL
