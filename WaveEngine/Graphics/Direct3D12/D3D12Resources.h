#pragma once

#include "D3D12CommonHeaders.h"
#include "D3D12Core.h"

namespace WAVEENGINE::GRAPHICS::D3D12 {

class descriptorHeap;

struct descriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE		cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE		gpu{};
	u32								index{ u32_invalid_id };

	constexpr bool is_valid() const { return cpu.ptr != 0; }
	constexpr bool is_shader_visible() const { return gpu.ptr != 0; }

#ifdef _DEBUG
private:
	friend class descriptorHeap;
	descriptorHeap*		container{ nullptr };
#endif

};

class descriptorHeap {
public:
	explicit descriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : _type(type) {}

	DISABLE_COPY_AND_MOVE(descriptorHeap); 
	// we delete copy constructor and move constructor because:
	// 1. std::mutex can not be copied or moved
	// 2. COM pointer hold a reference count
	// 3. descriptorHandle hold a reversing pointer of descriptorHeap
	// 4. need to move #frame_buffer_count vectors

	~descriptorHeap() {
		assert(!_heap);
	}

	bool initialize(u32 capacity, bool is_shader_visible);
	
	void release();

	void process_deferred_free(u32 frame_idx);

	[[nodiscard]] descriptorHandle allocate();
	void free(descriptorHandle& handle);

	[[nodiscard]] constexpr D3D12_DESCRIPTOR_HEAP_TYPE type() const { return _type; }
	[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpu_start() const { return _cpu_start; }
	[[nodiscard]] constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpu_start() const { return _gpu_start; }
	[[nodiscard]] constexpr ID3D12DescriptorHeap* const heap() const { return _heap; }
	[[nodiscard]] constexpr u32 capacity() const { return _capacity; }
	[[nodiscard]] constexpr u32 size() const { return _size; }
	[[nodiscard]] constexpr u32 descriptor_size() const { return _descriptor_size; }
	[[nodiscard]] constexpr bool is_shader_visible() const { return _gpu_start.ptr != 0; }

private:
	ID3D12DescriptorHeap*				_heap;												// assess map for real memory heap on GPU
	D3D12_CPU_DESCRIPTOR_HANDLE			_cpu_start{};
	D3D12_GPU_DESCRIPTOR_HANDLE			_gpu_start{};
	std::unique_ptr<u32[]>				_free_handles{};									// the indices of slots in the heap which are available for allocation
	// for each type of descriptorHeap, its _free_handles is shared between frames in each rendering round.
	UTL::vector<u32>					_deferred_free_indices[frame_buffer_count]{};
	std::mutex							_mutex{};
	u32									_capacity{ 0 };
	u32									_size{ 0 };											// heaps that already got allocated, which is also shared between frames
	u32									_descriptor_size{};
	const D3D12_DESCRIPTOR_HEAP_TYPE	_type{};
};


/// <summary>
/// info pack for a texture for process: allocate graphics memory -> create resource -> bind view
/// </summary>
struct d3d12TextureInitInfo {
	ID3D12Heap1*							heap{ nullptr };			// pointer to the real memory heap on GPU
	ID3D12Resource*							resource{ nullptr };		// the real texture resource object in graphics memory
	D3D12_SHADER_RESOURCE_VIEW_DESC*		srv_desc{ nullptr };		// Shader Resource View descriptor
	D3D12_RESOURCE_DESC*					desc{ nullptr };			// texture resource descriptor
	D3D12_RESOURCE_ALLOCATION_INFO1			allocation_info{};			// account of graphics memory and allocation information
	D3D12_RESOURCE_STATES					initial_state{};			// GPU state when create texture resource
	D3D12_CLEAR_VALUE						clear_value{};				// default clear value (for RTV / DSV)
};


class d3d12Texture {
public:
	constexpr static u32 max_mips{ 14 }; // support up to 16k resolutions
	d3d12Texture() = default;
	explicit d3d12Texture(d3d12TextureInitInfo info);

	DISABLE_COPY(d3d12Texture);

	constexpr d3d12Texture(d3d12Texture&& o) :
		_resource{ o._resource }, _srv{ o._srv } {
		o.reset(); // reset to default value
	}

	constexpr d3d12Texture& operator=(d3d12Texture&& o) {
		assert(this != &o);
		if (this != &o) {
			release();
			move(o);
		}

		return *this;
	}

	~d3d12Texture() {
		release();
	}

	void release();

	[[nodiscard]] constexpr ID3D12Resource* const resource() const { return _resource; }
	[[nodiscard]] constexpr descriptorHandle srv() const { return _srv; }

private:
	constexpr void move(d3d12Texture& o) {
		_resource = o._resource;
		_srv = o._srv;
		o.reset();
	}

	constexpr void reset() {
		_resource = nullptr;
		_srv = {};
	}

	ID3D12Resource*			_resource{ nullptr };
	descriptorHandle		_srv{};
};

class d3d12RenderTexture {
public:
	d3d12RenderTexture() = default;
	explicit d3d12RenderTexture(const d3d12TextureInitInfo& info);

	DISABLE_COPY(d3d12RenderTexture);

	constexpr d3d12RenderTexture(d3d12RenderTexture&& o) noexcept:
		_texture{ std::move(o._texture) }, _mip_count{ o._mip_count } {
		
		for (u32 i{ 0 }; i < _mip_count; ++i) {
			_rtv[i] = o._rtv[i];
		}
		o.reset();
	}

	constexpr d3d12RenderTexture& operator=(d3d12RenderTexture&& o) noexcept {
		assert(this != &o);
		if (this != &o) {
			release();
			move(o);
		}
		return *this;
	}

	~d3d12RenderTexture() {
		release();
	}

	void release();

	[[nodiscard]] constexpr u32 mip_count() const { return _mip_count; }
	[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv(u32 mip_index) const { assert(mip_index < _mip_count); return _rtv[mip_index].cpu; }
	[[nodiscard]] constexpr descriptorHandle srv() const { return _texture.srv(); }
	[[nodiscard]] constexpr ID3D12Resource* const resource() const { return _texture.resource(); }

private:

	constexpr void move(d3d12RenderTexture& o) {
		_texture = std::move(o._texture);
		_mip_count = o._mip_count;
		for (u32 i{ 0 }; i < _mip_count; ++i) {
			_rtv[i] = o._rtv[i];
		}
		o.reset();
	}

	constexpr void reset() {
		//_texture = {}; 
		_mip_count = 0;

		for (u32 i{ 0 }; i < _mip_count; ++i) {
			_rtv[i] = {};
		}
	}

	d3d12Texture			_texture{};
	descriptorHandle		_rtv[d3d12Texture::max_mips]{};
	u32						_mip_count{ 0 };
};

class d3d12DepthStencilBuffer {
public:
	d3d12DepthStencilBuffer() = default;
	explicit d3d12DepthStencilBuffer(d3d12TextureInitInfo& info);
	
	DISABLE_COPY(d3d12DepthStencilBuffer);

	constexpr d3d12DepthStencilBuffer(d3d12DepthStencilBuffer&& o) noexcept:
		_texture{ std::move(o._texture) }, _dsv{o._dsv} {
		o._dsv = {};
	}
	
	constexpr d3d12DepthStencilBuffer& operator=(d3d12DepthStencilBuffer&& o) noexcept {
		assert(this != &o);
		if (this != &o) {
			_texture = std::move(o._texture);
			_dsv = o._dsv;
			o._dsv = {};
		}
		return *this;
	}

	void release();

	~d3d12DepthStencilBuffer() { release(); }

	[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE dsv() const { return _dsv.cpu; }
	[[nodiscard]] constexpr descriptorHandle srv() const { return _texture.srv(); }
	[[nodiscard]] constexpr ID3D12Resource* const resource() const { return _texture.resource(); }

private:
	d3d12Texture			_texture{};
	descriptorHandle		_dsv{};
};

}
