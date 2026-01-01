#pragma once
#include "CommonHeaders.h"

namespace WAVEENGINE::UTL {

// A vector class similar to std::vector with basic functionality.
// The user can specify in the template argument whether they want 
// element' destructor to be called when being removed or while 
// clearing / destructing the vector.
template<typename T, bool destruct = true>
class vector {
public:
	// default constructor. Doesn't allocate memory.
	vector() = default;

	// constructor resizes the vector and initializes 'count' items.
	constexpr explicit vector(u64 count) {
		resize(count);
	}

	// constructor resizes the vector and initializes 'count' items using 'value'.
	constexpr explicit vector(u64 count, const T& value) {
		resize(count, value);
	}

	constexpr vector(std::initializer_list<T> init) {
		reserve(init.size());
		for (const auto& item : init)
			emplace_back(item);
	}

#ifdef _WIN64
	template<typename it, typename = std::enable_if_t<std::_Is_iterator_v<it>>>
#else
	template<typename it, typename = std::enable_if_t<_Is_iterator_v<it>>>
#endif
	constexpr explicit vector(it first, it last) {
		for (; first != last; ++first) {
			emplace_back(*first);
		}
	}

	// copy-constructor. Constructs by copying another vector. 
	// The items in the copied vector must be copyable.
	constexpr vector(const vector& o) {
		*this = o; // use operator = here
	}

	// move-constructor. Constructs by moving another vector.
	// the original vector will be emptied after move.
	constexpr vector(vector&& o) noexcept :
		_capacity{ o._capacity }, _size{ o._size }, _data{ o._data } {
		o.reset();
	}

	// copy-assignment operator. Clears this vector and copies items
	// from another vector. The items must be copyable
	constexpr vector& operator=(const vector& o) {
		assert(this != std::addressof(o));
		if (this != std::addressof(o)) {
			clear();
			reserve(o._size);
			for (auto& item : o) {
				emplace_back(item);
			}
			assert(_size == o._size);
		}
		return *this;
	}

	// move-assignment operator. Frees all resources in this vector and 
	// moves the other vector into this one.
	constexpr vector& operator=(vector&& o) {
		assert(this != std::addressof(o));
		if (this != std::addressof(o)) {
			destroy();
			move(o);
		}
		return *this;
	}

	// destructs the vector and its items as specified in template argument
	~vector() { destroy(); }
	
	// inserts an item at the end of the vector by copying value.
	constexpr void push_back(const T& value) {
		emplace_back(value);
	}

	// inserts an item at the end of the vector by moving value.
	constexpr void push_back(T&& value) {
		emplace_back(std::move(value));
	}

	template<typename... params>
	constexpr decltype(auto) emplace_back(params&&... p) {
		if (_size == _capacity) {
			reserve(((_capacity + 1) * 3) >> 1); // reserve 50% more
		}
		assert(_size < _capacity);
		
		T* const item{ new (std::addressof(_data[_size])) T(std::forward<params>(p)...) };
		++_size;
		return *item;
	}

	template<typename It>
	constexpr T* insert(T* const position, It first, It last) {
		assert(!_data || (position >= begin() && position <= end()));
		
		if (first == last) return position; // empty

		const u64 num_elements = std::distance(first, last);
		const u64 index = position - begin();
		const u64 new_size = _size + num_elements;

		if (new_size > _capacity) {
			reserve(((new_size + 1) * 3) >> 1);
		}

		// make room
		T* insert_pos = begin() + index;
		if (index < _size) {
			// move elements after insertion point
			std::memmove(insert_pos + num_elements, insert_pos, (_size - index) * sizeof(T));
		}

		for (u64 i = 0; first != last; ++first, ++i) {
			new (std::addressof(insert_pos[i])) T(*first);
		}

		_size = new_size;
		return insert_pos;
	}

	// resizes the vector and initializes new items with their default value.
	constexpr void resize(u64 new_size) {
		static_assert(std::is_default_constructible<T>::value, "Type must be default-constructible.");
		
		if (new_size > _size) {
			reserve(new_size);
			while (new_size > _size)
				emplace_back();
		}
		else if (new_size < _size) {
			if constexpr (destruct) {
				destruct_range(new_size, _size);
			}
			_size = new_size;
		}

		// do nothing if the new_size equals _size
		assert(new_size == _size);
	}

	// resizes the vector and initializes new items by copying value
	constexpr void resize(u64 new_size, const T& value) {
		static_assert(std::is_copy_constructible<T>::value, "Type must be copy-constructible.");

		if (new_size > _size) {
			reserve(new_size);
			while (new_size > _size)
				emplace_back(value);
		}
		else if (new_size < _size) {
			if constexpr (destruct) {
				destruct_range(new_size, _size);
			}
			_size = new_size;
		}

		// do nothing if the new_size equals _size
		assert(new_size == _size);
	}

	// allocates memory to contain teh specified number of items
	constexpr void reserve(u64 new_capacity) {
		if (new_capacity > _capacity) {
			// if the memory region can be expanded, realloc() will just expand memory region;
			// if not(no enough space left), realloc() will try to find a larger memory region and copy the original data
			// realloc() will automatically copy the data in the buffer 
			// if a new region of memory is allocated.
			void* new_buffer{ realloc(_data, new_capacity * sizeof(T)) };
			assert(new_buffer);
			if (new_buffer) {
				_data = static_cast<T*>(new_buffer);
				_capacity = new_capacity;
			}
		}
	}

	// removes the item at specified index
	constexpr T* const erase(u64 index) {
		assert(_data && index >= 0 && index < _size); 
		return erase(std::addressof(_data[index]));
	}

	// removes the item at specified location
	constexpr T* const erase(T* const item) {
		assert(_data && item >= std::addressof(_data[0]) && item < std::addressof(_data[_size])); 
		if constexpr (destruct) item->~T(); // there is a hole in the memory
		--_size;

		// move forward all elements which are after the item to fill the memory hole
		if (item < std::addressof(_data[_size])) { 
			memcpy(item, item + 1, (std::addressof(_data[_size]) - item) * sizeof(T)); // dst <- src
		}

		return item;
	}

	// same as erase() but faster because it just copies the last item
	constexpr T* const erase_unordered(u64 index) {
		assert(_data && index >= 0 && index < _size);
		return erase_unordered(std::addressof(_data[index]));
	}

	// after erase specified item, copy the last item into the current item location
	constexpr T* const erase_unordered(T* const item) {
		assert(_data && item >= std::addressof(_data[0]) && item < std::addressof(_data[_size]));
		if constexpr (destruct) item->~T();
		--_size;

		if (item < std::addressof(_data[_size])) {
			memcpy(item, std::addressof(_data[_size]), sizeof(T));
		}

		return item;
	}

	// clears the vector an destructs items as specified in template argument
	constexpr void clear() {
		if constexpr (destruct) {
			destruct_range(0, _size);
		}
		_size = 0;
	}

	// can a vector<T> and vector<T, false> be swapped
	constexpr void swap(vector& o) {
		if (this != std::addressof(o)) {
			auto temp(std::move(o));
			o.move(*this);
			move(temp);
		}
	}

	constexpr bool empty() const {
		return _size == 0;
	}

	[[nodiscard]] constexpr u64 capacity() const {
		return _capacity;
	}

	[[nodiscard]] constexpr u64 size() const {
		return _size;
	}


	// for vector<> usage
	[[nodiscard]] constexpr T* data() {
		return _data;
	}

	[[nodiscard]] constexpr T& operator[](u64 index) {
		assert(_data && index < _size);
		return _data[index];
	}

	[[nodiscard]] constexpr T& front() {
		assert(_data && _size);
		return _data[0];
	}

	[[nodiscard]] constexpr T& back() {
		assert(_data && _size);
		return _data[_size - 1];
	}

	[[nodiscard]] constexpr T* begin() {
		return std::addressof(_data[0]);
	}

	[[nodiscard]] constexpr T* end() {
		return std::addressof(_data[_size]); // invalid
	}

	// for const vector<> usage
	[[nodiscard]] constexpr const T* data() const {
		return _data;
	}

	[[nodiscard]] constexpr const T& operator[](u64 index) const {
		assert(_data && index < _size);
		return _data[index];
	}

	[[nodiscard]] constexpr const T& front() const {
		assert(_data && _size);
		return _data[0];
	}

	[[nodiscard]] constexpr const T& back() const {
		assert(_data && _size);
		return _data[_size - 1];
	}

	[[nodiscard]] constexpr const T* begin() const {
		return std::addressof(_data[0]);
	}

	[[nodiscard]] constexpr const T* end() const {
		return std::addressof(_data[_size]); // invalid
	}


private:
	constexpr void move(vector& o) {
		_capacity = o._capacity;
		_size = o._size;
		_data = o._data;
		o.reset();
	}

	constexpr void reset() {
		_capacity = 0;
		_size = 0;
		_data = nullptr;
	}

	constexpr void destruct_range(u64 first, u64 last) {
		assert(destruct);
		assert(first <= _size && last <= _size && first <= last);
		if (_data) {
			for (; first != last; ++first) {
				_data[first].~T();
			}
		}
	}

	constexpr void destroy() {
		assert([&] {return _capacity ? _data != nullptr : _data == nullptr; }());
		clear(); // call destructor for each item and set _size = 0 inside 
		_capacity = 0;
		if (_data) {
			free(const_cast<void*>(static_cast<const void*>(_data)));
		}
		_data = nullptr;
	}

	u64			_capacity{ 0 };
	u64			_size{ 0 };
	T*			_data{ nullptr };
};

}
