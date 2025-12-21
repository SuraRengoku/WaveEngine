#pragma once
#include "CommonHeaders.h"

namespace WAVEENGINE::UTL {
	
template<typename T>
class arrayRef {
public:
	// construct from empty, count = 0
	arrayRef() = default;

	// construct from single object, count = 1
	arrayRef(T& data) : _pArray(&data), _count(1) {}

	// construct from top-level array
	template<size_t ElementCount>
	arrayRef(T(&data)[ElementCount]) : _pArray(data), _count(ElementCount) {}

	// construct from pointer and number of elements
	arrayRef(T* pData, u64 elementCount) : _pArray(pData), _count(elementCount) {}

	// if T has a const modifier, make it compatible to construct from non-const version
	arrayRef(const arrayRef<std::remove_const_t<T>>& other) : _pArray(other.data()), _count(other.count()) {}

	T* data() const { return _pArray; }
	u64 count() const { return _count; }
	T& operator[](u64 index) const { return _pArray[index]; }
	T* begin() const { return _pArray; }
	T* end() const { return _pArray + _count; }
	DISABLE_COPY_AND_MOVE(arrayRef);
private:
	T* const		_pArray = nullptr;
	u64				_count = 0;
};

}
