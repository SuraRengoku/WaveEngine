#include "D3D12Resources.h"

namespace WAVEENGINE::GRAPHICS::D3D12 {

//// DESCRIPTOR HEAP ///////////////////////////////////////////////////////////////////////////////////////

// descriptor type		shader visible		maximum capacity						size(bytes)
// CBV_SRV_UAV			yes					1,000,000(Tier2)						32 / 64
// CBV_SRV_UAV			no					unlimited / limited by system memory	32 / 64
// SAMPLER				yes					2048									16 / 32
// SAMPLER				no					2048									16 / 32
// RTV					no					unlimited								16 / 32
// DSV					no					unlimited								16 / 32

// rendering pipeline
// ©°©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©´
// ©¦ Shader Stages										 ©¦
// ©¦ - Vertex Shader      ¡û CBV / SRV / UAV accessible   ©¦
// ©¦ - Pixel Shader       ¡û CBV / SRV / UAV accessible   ©¦
// ©¦ - Compute Shader     ¡û CBV / SRV / UAV accessible   ©¦
// ©¸©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¼
//							¡ý
// ©°©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©´
// ©¦ Output Merger (fixed function)											©¦
// ©¦ - RTV(render target)     ¡û written by pipeline, not a shader resource	©¦
// ©¦ - DSV(depth + stencil)   ¡û written by pipeline, not a shader resource	©¦
// ©¸©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¼

bool descriptorHeap::initialize(u32 capacity, bool is_shader_visible) {
	std::lock_guard lock{ _mutex };
	assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
	// for sampler type the capacity limitation is much more strict because sampler is a hardware resource
	assert(!(_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));
	if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		is_shader_visible = false;

	release();

	auto* const device{ CORE::device() };
	assert(device);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = is_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NumDescriptors = capacity;
	desc.Type = _type;
	desc.NodeMask = 0;

	HRESULT hr{ S_OK };
	DXCall(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
	if (FAILED(hr)) return false;

	_free_handles = std::move(std::make_unique<u32[]>(capacity));
	_capacity = capacity;
	_size = 0;

	for (u32 i{ 0 }; i < capacity; ++i) {
		_free_handles[i] = i;
	} // initilially, every slot is free.

	DEBUG_OP(for (u32 i{ 0 }; i < frame_buffer_count; ++i) {
		assert(_deferred_free_indices[i].empty());
	});

	_descriptor_size = device->GetDescriptorHandleIncrementSize(_type); 
	_cpu_start = _heap->GetCPUDescriptorHandleForHeapStart();
	_gpu_start = is_shader_visible ? _heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

	return true;
}

void descriptorHeap::release() {
	assert(!_size);
	CORE::deferred_release(_heap);
}

/// <summary>
/// in this function, we will eventually free the handle slots
/// </summary>
/// <param name="frame_idx">we do this for each frame separately</param>
void descriptorHeap::process_deferred_free(u32 frame_idx) {
	std::lock_guard lock{ _mutex };
	assert(frame_idx < frame_buffer_count);

	UTL::vector<u32>& indices{ _deferred_free_indices[frame_idx] };
	if (!indices.empty()) {
		for (auto index : indices) {
			--_size;
			_free_handles[_size] = index;
		}
		indices.clear();
	}
}

descriptorHandle descriptorHeap::allocate() {
	std::lock_guard lock{ _mutex };
	assert(_heap);
	assert(_size < _capacity);

	const u32 index{ _free_handles[_size] };
	const u32 offset{ index * _descriptor_size };
	++_size;

	descriptorHandle handle;
	handle.cpu.ptr = _cpu_start.ptr + offset;
	if (is_shader_visible()) {
		handle.gpu.ptr = _gpu_start.ptr + offset;
	}

	DEBUG_OP(handle.container = this); 
	DEBUG_OP(handle.index = index);
	return handle;
}

/// <summary>
/// we will put the slot index into the corresponding _deferred_free_indices instead of actually free the slot
/// </summary>
/// <param name="handle">contain cpu pointer and gpu pointer, indicating the slot index in _free_handles</param>
void descriptorHeap::free(descriptorHandle& handle) {
	if (!handle.is_valid()) return;
	std::lock_guard lock{ _mutex };

	assert(_heap && _size);
	assert(handle.container == this);
	assert(handle.cpu.ptr >= _cpu_start.ptr);
	assert((handle.cpu.ptr - _cpu_start.ptr) % _descriptor_size == 0);
	assert(handle.index < _capacity);
	const u32 index{ (u32)(handle.cpu.ptr - _cpu_start.ptr) / _descriptor_size };
	assert(handle.index == index);

	const u32 frame_idx{ CORE::current_frame_index() };
	_deferred_free_indices[frame_idx].push_back(index);
	CORE::set_deferred_releases_flag();

	handle = {};
	
}

//// D3D12 TEXTURE ///////////////////////////////////////////////////////////////////////////////////////

d3d12Texture::d3d12Texture(d3d12TextureInitInfo info) {
	auto* const device{ CORE::device() };
	assert(device);

	D3D12_CLEAR_VALUE* const clear_value{
		(info.desc &&
		(info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
		 info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
		? &info.clear_value : nullptr
	};

	if (info.resource) {
		assert(!info.heap);
		_resource = info.resource;
	}
	else if(info.heap && info.desc) {
		assert(!info.resource);
		// create resource and put it in a self-managered memory heap
		DXCall(device->CreatePlacedResource(
			info.heap, info.allocation_info.Offset, info.desc, info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
	}
	else if(info.desc) {
		assert(!info.heap && !info.resource);
		// create resource and put it in a driver-managed memory heap
		DXCall(device->CreateCommittedResource(
			&D3DX::heap_properties.default_heap, D3D12_HEAP_FLAG_NONE, info.desc, info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
	}

	assert(_resource);
	_srv = CORE::srv_heap().allocate();
	// create a read-only sampling view
	device->CreateShaderResourceView(_resource, info.srv_desc, _srv.cpu);
}

void d3d12Texture::release() {
	CORE::srv_heap().free(_srv);
	CORE::deferred_release(_resource);
}

//// RENDER TEXTURE ///////////////////////////////////////////////////////////////////////////////////////

d3d12RenderTexture::d3d12RenderTexture(d3d12TextureInitInfo info) : _texture{ info } {
	assert(info.desc);
	_mip_count = resource()->GetDesc().MipLevels;
	assert(_mip_count && _mip_count <= d3d12Texture::max_mips);

	descriptorHeap& rtv_heap{ CORE::rtv_heap() };
	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = info.desc->Format;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	auto* const device{ CORE::device() };
	assert(device);
	
	for (u32 i{ 0 }; i < _mip_count; ++i) {
		_rtv[i] = CORE::srv_heap().allocate();
		// create a color writing view
		device->CreateRenderTargetView(resource(), &desc, _rtv[i].cpu);
		++desc.Texture2D.MipSlice;
	}
}

void d3d12RenderTexture::release() {
	for (u32 i{ 0 }; i < _mip_count; ++i) {
		CORE::rtv_heap().free(_rtv[i]);
	}
	_texture.release();
	_mip_count = 0;
}
 
//// DEPTH BUFFER ///////////////////////////////////////////////////////////////////////////////////////

d3d12DepthStencilBuffer::d3d12DepthStencilBuffer(d3d12TextureInitInfo info) : _texture{ info } {
	assert(info.desc);
	const DXGI_FORMAT dsv_format{ info.desc->Format }; // store the original format for creating dsv

	// NOTE: For creating dsv we need the resource format to be D32_FLOAT.
	//		 Howerve for creating srv we need the resource format to be R32_FLOAT.
	//		 After all, we set the function to be TYPELESS to make the resource both suitable for dsv and srv (can be written or read)
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
	if (info.desc->Format == DXGI_FORMAT_D32_FLOAT) {
		info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	}

	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.PlaneSlice = 0;
	srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

	assert(!info.srv_desc && !info.resource);
	info.srv_desc = &srv_desc;
	_texture = d3d12Texture(info); // create srv

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
	dsv_desc.Format = dsv_format; // now use the original format
	dsv_desc.Texture2D.MipSlice = 0;

	_dsv = CORE::dsv_heap().allocate();

	auto* const device{ CORE::device() };
	assert(device);
	// create a depth/stencil writing view
	device->CreateDepthStencilView(resource(), &dsv_desc, _dsv.cpu); // create dsv
}

void d3d12DepthStencilBuffer::release() {
	CORE::dsv_heap().free(_dsv);
	_texture.release();
}

}
 