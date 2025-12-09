#include "D3D12Core.h"
#include "D3D12Surface.h"

using namespace Microsoft::WRL;

namespace WAVEENGINE::GRAPHICS::D3D12::CORE {

void create_a_root_signature();
void create_a_root_signature2();

namespace {

class d3d12Command {
public:
	d3d12Command() = default;

	DISABLE_COPY_AND_MOVE(d3d12Command);

	explicit d3d12Command(id3d12Device* const device, D3D12_COMMAND_LIST_TYPE type) {
		HRESULT hr{ S_OK };
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0; // commands on which GPU node? default 0
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // NORMAL -> HIGH -> REALTIME
		desc.Type = type;

		DXCall(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_cmd_queue)));
		if (FAILED(hr))	goto _error;
		NAME_D3D12_OBJECT(_cmd_queue,
			type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
			L"GFX Command Queue" :
			type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
			L"Compute Command Queue" : L"Command Queue");

		for (u32 i{ 0 }; i < frame_buffer_count; ++i) {
			command_frame& frame{ _cmd_frames[i] };
			DXCall(hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.cmd_allocator)));
			if (FAILED(hr)) goto _error;
			NAME_D3D12_OBJECT_INDEXED(frame.cmd_allocator, i, 
				type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
				L"GFX Command Allocator" :
				type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
				L"Compute Command Allocator" : L"Command Allocator");
		}

		DXCall(hr = device->CreateCommandList(0, type, _cmd_frames[0].cmd_allocator, nullptr, IID_PPV_ARGS(&_cmd_list)));
		if (FAILED(hr)) goto _error;
		DXCall(_cmd_list->Close());
		NAME_D3D12_OBJECT(_cmd_list,
			type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
			L"GFX Command List" :
			type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
			L"Compute Command List" : L"Command List");

		DXCall(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
		if (FAILED(hr))	goto _error;
		NAME_D3D12_OBJECT(_fence, L"D3D12 Fence");

		_fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		assert(_fence_event);

		return;
		
	_error:
		release();
	}

	~d3d12Command() {
		assert(!_cmd_queue && !_cmd_list && !_fence);
	}

	// wait for the current frame to be signalled and reset the command list/allocator
	void begin_frame() {
		command_frame& frame{ _cmd_frames[_frame_index] };
		// fence value <= completed value -> continue
		// fence value > completed value -> WAIT
		frame.wait(_fence_event, _fence);
		
		// resetting the command allocator will free the memory used by previously recorded commands
		DXCall(frame.cmd_allocator->Reset()); 
		// resetting the command list will reopen it for recording new commands
		DXCall(_cmd_list->Reset(frame.cmd_allocator, nullptr));
	}

	// signal the fence with new fence value
	void end_frame(const d3d12Surface& surface) {
		DXCall(_cmd_list->Close());
		ID3D12CommandList* const cmd_lists[]{ _cmd_list };
		_cmd_queue->ExecuteCommandLists(_countof(cmd_lists), &cmd_lists[0]);

		// presenting swap chain buffers happens in lockstep with frame buffers;
		surface.present();

		u64& fence_value{ _fence_value };
		++fence_value;
		command_frame& frame{ _cmd_frames[_frame_index] };
		frame.fence_value = fence_value;
		_cmd_queue->Signal(_fence, fence_value); // update completed value as fence_value after GPU finished former commands

		_frame_index = (_frame_index + 1) % frame_buffer_count;
	}

	void flush() {
		for (u32 i{ 0 }; i < frame_buffer_count; ++i) {
			_cmd_frames[i].wait(_fence_event, _fence);
		}
		_frame_index = 0;
	}

	void release() {
		flush(); // let GPU finish all its work

		CORE::release(_fence);
		_fence_value = 0;

		CloseHandle(_fence_event);
		_fence_event = nullptr;

		CORE::release(_cmd_queue);
		CORE::release(_cmd_list);
		
		for (u32 i{ 0 }; i < frame_buffer_count; ++i) {
			_cmd_frames[i].release();
		}

	}

	[[nodiscard]] constexpr ID3D12CommandQueue* const command_queue() const { return _cmd_queue; }
	[[nodiscard]] constexpr id3d12GraphicsCommandList* const command_list() const { return _cmd_list; }
	[[nodiscard]] constexpr u32 frame_index() const { return _frame_index; }

private:
	struct command_frame {
		ID3D12CommandAllocator* cmd_allocator{ nullptr };
		u64						fence_value{ 0 };

		// wait for the GPU to finish the current frame
		void wait(HANDLE fence_event, ID3D12Fence1* fence) {
			assert(fence && fence_event);
			// if the current fence value is still less than "fence_value"
			// then we know the GPU has not finished executing the command lists
			// since it has not reached the "_cmd_queue->Signal()" command
			if (fence->GetCompletedValue() < fence_value) {
				// We have the fence create an event which is signaled one the fence's current value equals "fence_value"
				DXCall(fence->SetEventOnCompletion(fence_value, fence_event)); // lauch fence_event until fence reaches fence_value
				// Wait until the fence has triggered the event that its current value has reached "fence_value"
				// indicating that command queue has finished executing.
				WaitForSingleObject(fence_event, INFINITE); // suspend CPU until fence_event
			}
		}

		void release() {
			CORE::release(cmd_allocator);
			fence_value = 0;
		}
	};
	
	ID3D12CommandQueue*         _cmd_queue{ nullptr }; // for GPU
	id3d12GraphicsCommandList*	_cmd_list{ nullptr }; // for CPU
	ID3D12Fence1*				_fence{ nullptr };
	u64							_fence_value{ 0 };
	HANDLE						_fence_event{ nullptr };
	command_frame				_cmd_frames[frame_buffer_count]{};
	u32							_frame_index{ 0 };
};

using surfaceCollection = UTL::freeList<d3d12Surface>;

id3d12Device*				main_device{ nullptr }; // latest for windows 10
IDXGIFactory7*				dxgi_factory{ nullptr };
d3d12Command				gfx_command;
surfaceCollection			surfaces;
D3DX::d3d12ResourceBarrier	resource_barriers{};

descriptorHeap				rtv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
descriptorHeap				dsv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
descriptorHeap				srv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
descriptorHeap				uav_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

//// Inheritance Relation ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//					 IUnknown
//						|
//		+---------------+---------------+
//		|								|
//	ID3D12Object                IDXGIObject
//	|                           |
//	+--ID3D12Device				+--IDXGISwapChain
//	+--ID3D12Resource			+--IDXGIAdapter
//	+--ID3D12CommandQueue		+--IDXGIFactory
//	+--ID3D12CommandList
//	+-- ...
//
//// all these types can be converted into IUnknown
//// all these types can use Release() method

UTL::vector<IUnknown*>		deferred_releases[frame_buffer_count]{};
u32							deferred_releases_flag[frame_buffer_count];
std::mutex					deferred_releases_mutex{};

// constexpr DXGI_FORMAT render_target_format{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
constexpr D3D_FEATURE_LEVEL minimum_feature_level{ D3D_FEATURE_LEVEL_11_0 };

/// <summary>
/// Finds and returns the first high-performance IDXGIAdapter4 that supports the configured minimum D3D12 feature level.
/// </summary>
/// <returns>A pointer to an IDXGIAdapter4 that supports the required feature level and is enumerated with high-performance preference, 
/// or nullptr if no suitable adapter is found. The returned adapter has a live COM reference (ownership is transferred to the caller), 
/// so the caller is responsible for calling Release when finished. 
/// Note: the function enumerates adapters via the global dxgi_factory and tests each adapter with D3D12CreateDevice; 
/// adapters that are not selected are released internally.</returns>
IDXGIAdapter4* determine_main_adapter() {
	IDXGIAdapter4* adapter{ nullptr };

	// get adapters in descending order of performance
	for (u32 i{ 0 };
		dxgi_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
		++i) {
		// pick the first adapter supports the minimum feature level.
		if (SUCCEEDED(D3D12CreateDevice(adapter, minimum_feature_level, __uuidof(ID3D12Device), nullptr))) {
			return adapter;
		}
		release(adapter);
	}
	return nullptr;
}

D3D_FEATURE_LEVEL get_max_feature_level(IDXGIAdapter4* adapter) {
	constexpr D3D_FEATURE_LEVEL feature_levels[4]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS feature_level_info{};
	feature_level_info.NumFeatureLevels = _countof(feature_levels);
	feature_level_info.pFeatureLevelsRequested = feature_levels;

	ComPtr<ID3D12Device> device;
	DXCall(D3D12CreateDevice(adapter, minimum_feature_level, IID_PPV_ARGS(&device)));
	DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_level_info, sizeof(feature_level_info)));
	return feature_level_info.MaxSupportedFeatureLevel;
}

bool failed_init() {
	shutdown();
	return false;
}

void __declspec(noinline) process_deferred_releases(u32 frame_idx) {
	std::lock_guard lock{ deferred_releases_mutex };
	
	// TODO: maybe set the flag in the end will not cause problem?
	// clear the flag in the beginning but not the end.
	// if we clear it at the end then it might be overwritten by some other thread that was trying to set it.
	// it's fine if overwriting happens before processing the items.
	deferred_releases_flag[frame_idx] = 0; 
	
	rtv_desc_heap.process_deferred_free(frame_idx);
	dsv_desc_heap.process_deferred_free(frame_idx);
	srv_desc_heap.process_deferred_free(frame_idx);
	uav_desc_heap.process_deferred_free(frame_idx);
	
	UTL::vector<IUnknown*>& resources{ deferred_releases[frame_idx] };
	if (!resources.empty()) {
		for (auto& resource : resources) release(resource);
		resources.clear();
	}

}

} // anonymous namespace


namespace DETAIL {

void deferred_release(IUnknown* resource) {
	const u32 frame_idx{ current_frame_index() };
	std::lock_guard lock{ deferred_releases_mutex };
	deferred_releases[frame_idx].push_back(resource);
	set_deferred_releases_flag(); // resources for current frame need to be deferred release
}

} // DETAIL


bool initialize() {

	if (main_device) shutdown();

	u32 dxgi_factory_flags = 0;
#ifdef _DEBUG
	// Enable debugging layer. Requires "Graphics Tools" optional feature
	{
		ComPtr<ID3D12Debug3> debug_interface;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))) {
			debug_interface->EnableDebugLayer();
#if 1
#pragma message("WARNING: GPU_based validation is enabled. This will considerably slow down the renderer!")
			debug_interface->SetEnableGPUBasedValidation(1); 
#endif
		}
		else {
			OutputDebugStringA("Warning: D3D12 Debug interface is not available, Verify that Graphics Tools optional feature is installed in this device.\n");
		}

		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif // _DEBUG

	HRESULT hr{ S_OK };
	//CreateDXGIFactory2(factory_flags, __uuidof(IDXGIFactory7), (void**)&dxgi_factory); // use macro below
	DXCall(hr = CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));
	if (FAILED(hr)) return failed_init();

	// determine which adapter (i.e. graphics card) to use
	ComPtr<IDXGIAdapter4> main_adapter;
	main_adapter.Attach(determine_main_adapter());
	if (!main_adapter) return failed_init();

	// determine what is the maximum feature level that is supporter
	D3D_FEATURE_LEVEL max_feature_level{ get_max_feature_level(main_adapter.Get()) };
	assert(max_feature_level >= minimum_feature_level);
	if (max_feature_level < minimum_feature_level) return failed_init();

	// create a ID3D12Device (this is a virtual adapter)
	DXCall(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, IID_PPV_ARGS(&main_device)));
	if (FAILED(hr)) return failed_init();

	NAME_D3D12_OBJECT(main_device, L"Main D3D12 Device");

#ifdef _DEBUG
	{
		ComPtr<ID3D12InfoQueue> info_queue; // only used when debug layer is enabled
		DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif // _DEBUG

	///// DESCRIPTOR HEAPS //////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool descriptorInitResult{ true };
	descriptorInitResult &= rtv_desc_heap.initialize(512, false);
	descriptorInitResult &= dsv_desc_heap.initialize(512, false);
	descriptorInitResult &= srv_desc_heap.initialize(4096, true);
	descriptorInitResult &= uav_desc_heap.initialize(512, false);
	if (!descriptorInitResult) return failed_init();

	NAME_D3D12_OBJECT(rtv_desc_heap.heap(), L"RTV Descriptor Heap");
	NAME_D3D12_OBJECT(dsv_desc_heap.heap(), L"DSV Descriptor Heap");
	NAME_D3D12_OBJECT(srv_desc_heap.heap(), L"SRV Descriptor Heap");
	NAME_D3D12_OBJECT(uav_desc_heap.heap(), L"UAV Descriptor Heap");

	//// COMMAND BUFFER /////////////////////////////////////////////////////////////////////////////////////////////////////////////

	new (&gfx_command) d3d12Command(main_device, D3D12_COMMAND_LIST_TYPE_DIRECT); // placement new because copy and move constructor has been removed
	if (!gfx_command.command_queue()) return failed_init();

	// initialize modules
	if (!(SHADERS::initialize() && GPASS::initialize() && POSTP::initialize()))
		return failed_init();

	// TODO: remove
	//create_a_root_signature();
	//create_a_root_signature2();

	return true;
}

void shutdown() {
	gfx_command.release();

	// shutdown Post-process module
	POSTP::shutdown();
	// shutdown gpass module
	GPASS::shutdown();
	// shutdown shader module
	SHADERS::shutdown();

	release(dxgi_factory);
	
	// process_deferred_releases should be called first because 
	// some resources(i.e. swap chains) can't be released before their depending on resources are released
	for (u32 i{ 0 }; i < frame_buffer_count; ++i) {
		process_deferred_releases(i);
	}

	// NOTE: some types only use deferred release for their resources during shutdown/reset/clear.
	//		 To finally release them we call process_deferred_free once more.
	rtv_desc_heap.process_deferred_free(0);
	dsv_desc_heap.process_deferred_free(0);
	srv_desc_heap.process_deferred_free(0);
	uav_desc_heap.process_deferred_free(0);

	rtv_desc_heap.release();
	dsv_desc_heap.release();
	srv_desc_heap.release();
	uav_desc_heap.release();

	// release again because descriptorHeap.release() may introduce descriptorHeap resources to be deferredly released
	process_deferred_releases(0);

#ifdef _DEBUG
	{
		{
			ComPtr<ID3D12InfoQueue> info_queue;
			DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
		}

		ComPtr<ID3D12DebugDevice> debug_device;
		DXCall(main_device->QueryInterface(IID_PPV_ARGS(&debug_device)));
		release(main_device);
		DXCall(debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
	}
#endif // _DEBUG

	release(main_device);
}


id3d12Device* const device() {
	return main_device;
}

descriptorHeap& rtv_heap() {
	return rtv_desc_heap;
}

descriptorHeap& dsv_heap() {
	return dsv_desc_heap;
}

descriptorHeap& srv_heap() {
	return srv_desc_heap;
}

descriptorHeap& uav_heap() {
	return uav_desc_heap;
}

u32 current_frame_index() {
	return gfx_command.frame_index();
}

void set_deferred_releases_flag() {
	deferred_releases_flag[current_frame_index()] = 1; // atomic
}

surface create_surface(PLATFORM::window window) {
	surface_id id{ surfaces.add(window) }; // constructor only takes PLATFORM::window as input parameter
	surfaces[id].create_swap_chain(dxgi_factory, gfx_command.command_queue());
	return surface{ id };
}

void remove_surface(surface_id id) {
	gfx_command.flush(); // waiting GPU
	surfaces.remove(id); 
}

void resize_surface(surface_id id, u32 width, u32 height) {
	gfx_command.flush();
	surfaces[id].resize();
}

u32 surface_width(surface_id id) {
	return surfaces[id].width();
}
u32 surface_height(surface_id id) {
	return surfaces[id].height();
}

void render_surface(surface_id id) {
	// wait for the GPU to finish with the command allocator and 
	// reset the allocator once the GPU is done with it
	// This frees the memory that was used to store commands
	gfx_command.begin_frame();
	id3d12GraphicsCommandList* cmd_list{ gfx_command.command_list() };
	// last round rendering for current frame finished

	// release resources in last round
	const u32 frame_idx{ current_frame_index() };
	if (deferred_releases_flag[frame_idx]) {
		process_deferred_releases(frame_idx);
	}

	const d3d12Surface& surface{ surfaces[id] };

	ID3D12Resource* const current_back_buffer{ surface.back_buffer() };

	d3d12FrameInfo frame_info{ surface.width(), surface.height() };
	GPASS::set_size({ frame_info.surface_width, frame_info.surface_height });
	D3DX::d3d12ResourceBarrier& barriers{ resource_barriers };

	// Record commands
	// ...
	ID3D12DescriptorHeap* const heaps[]{ srv_desc_heap.heap() };
	cmd_list->SetDescriptorHeaps(1, &heaps[0]);

	cmd_list->RSSetViewports(1, &surface.viewport());
	cmd_list->RSSetScissorRects(1, &surface.scissor_rect());

	// Depth prepass
	barriers.add(current_back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
	GPASS::add_transitions_for_depth_prepass(barriers);
	barriers.apply(cmd_list);
	GPASS::set_render_targets_for_depth_prepass(cmd_list);
	GPASS::depth_prepass(cmd_list, frame_info);

	// Geometry and lighting pass
	GPASS::add_transitions_for_gpass(barriers);
	barriers.apply(cmd_list);
	GPASS::set_render_targets_for_gpass(cmd_list);
	GPASS::render(cmd_list, frame_info); // draw call inside

	//D3DX::transition_resource(cmd_list, current_back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// Post-process
	barriers.add(current_back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
	GPASS::add_transitions_for_post_process(barriers);
	barriers.apply(cmd_list);
	// will write to the current back buffer, so back buffer is a render target

	POSTP::post_process_render(cmd_list, surface.rtv());

	// after post process
	D3DX::transition_resource(cmd_list, current_back_buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// Done recording commands, ready to execute commands
	// signal and increment the fence value for next frame
	gfx_command.end_frame(surface);
}

/// <summary>
/// this function demonstrates how to create a root signature as an example.
/// it will be removed later.
/// </summary>
void create_a_root_signature() {
	D3D12_ROOT_PARAMETER1 params[3];
	{ // param 0: 2 constants
		auto& param = params[0];
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		D3D12_ROOT_CONSTANTS constants{};
		constants.Num32BitValues = 2;
		constants.ShaderRegister = 0;
		constants.RegisterSpace = 0;
		param.Constants = constants;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	{ // param 1: 1 Constant Buffer View (Descriptor)
		auto& param = params[1];
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		D3D12_ROOT_DESCRIPTOR1 root_desc{};
		root_desc.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
		root_desc.ShaderRegister = 0;
		root_desc.RegisterSpace = 0;
		param.Descriptor = root_desc;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	}
	{ // param 2: descriptor table (unbounded/bindless)
		auto& param = params[2];
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		D3D12_ROOT_DESCRIPTOR_TABLE1 table{};
		table.NumDescriptorRanges = 1;
		D3D12_DESCRIPTOR_RANGE1 range{};
		range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range.NumDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		range.BaseShaderRegister = 0;
		range.RegisterSpace = 0;
		table.pDescriptorRanges = &range;
		param.DescriptorTable = table;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	D3D12_STATIC_SAMPLER_DESC sampler_desc{};
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC1 desc{};
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	desc.NumParameters = _countof(params);
	desc.pParameters = &params[0];
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &sampler_desc;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rs_desc{};
	rs_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rs_desc.Desc_1_1 = desc;

	HRESULT hr{ S_OK };
	ID3DBlob* root_sig_blob{ nullptr };
	ID3DBlob* error_blob{ nullptr };
	if (FAILED(hr = D3D12SerializeVersionedRootSignature(&rs_desc, &root_sig_blob, &error_blob))) {
		DEBUG_OP(const char* error_msg{ error_blob ? (const char*)error_blob->GetBufferPointer() : "" });
		DEBUG_OP(OutputDebugStringA(error_msg));
		return;
	}

	assert(root_sig_blob);
	ID3D12RootSignature* root_sig{ nullptr };
	DXCall(device()->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(&root_sig)));

	release(root_sig_blob);
	release(error_blob);

	// use root_sig during rendering (not in this function, obviously)
#if 0
	id3d12GraphicsCommandList* cmd_list{};
	cmd_list->SetGraphicsRootSignature(root_sig);
	// only one resouce heap and sampler heap can be set at any time
	// so, max number of heaps is 2
	ID3D12DescriptorHeap* heaps[]{ srv_heap().heap() };
	cmd_list->SetDescriptorHeaps(1, &heaps[0]);

	// set root parameters
	float dt{ 16.6f };
	u32 dt_uint{ *((u32*)&dt) };
	u32 frame_nr{ 4287827 };
	D3D12_GPU_VIRTUAL_ADDRESS address_of_constant_buffer{/*our constant buffer which we don't have right now*/ };
	cmd_list->SetGraphicsRoot32BitConstant(0, dt_uint, 0);
	cmd_list->SetGraphicsRoot32BitConstant(0, frame_nr, 1);
	cmd_list->SetGraphicsRootConstantBufferView(1, address_of_constant_buffer);
	cmd_list->SetGraphicsRootDescriptorTable(2, srv_heap().gpu_start());
	// record the rest of rendering commands...
#endif

	// when renderer shuts down
	release(root_sig);
}

void create_a_root_signature2() {
	D3DX::d3d12DescriptorRange range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, 0 };
	D3DX::d3d12RootParameter params[3];
	params[0].as_constants(2, D3D12_SHADER_VISIBILITY_PIXEL, 0);
	params[1].as_cbv(D3D12_SHADER_VISIBILITY_PIXEL, 1);
	params[2].as_descriptor_table(D3D12_SHADER_VISIBILITY_PIXEL, &range, 1);

	D3DX::d3d12RootSignatureDesc root_signature_desc{ &params[0], _countof(params) };
	ID3D12RootSignature* root_signature{ root_signature_desc.create() };

	// use root_signature

	// when renderer shuts down
	release(root_signature);
}

ID3D12RootSignature* _root_signature;
D3D12_SHADER_BYTECODE _vs{};

using d3d12PipelineStateSubobjectRootSignature = D3DX::d3d12PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
using d3d12PipelineStateSubobjectVS = D3DX::d3d12PipelineStateSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>;

void create_a_pipeline_state_object() {
	struct {
		struct alignas(void*){
			const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{ D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE };
			ID3D12RootSignature* root_signature;
		} root_sig;
		struct alignas(void*){
			const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{ D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS };
			D3D12_SHADER_BYTECODE vs_code{};
		} vs;
	} stream;

	stream.root_sig.root_signature = _root_signature;
	stream.vs.vs_code = _vs;

	D3D12_PIPELINE_STATE_STREAM_DESC desc{};
	desc.pPipelineStateSubobjectStream = &stream;
	desc.SizeInBytes = sizeof(stream);
	ID3D12PipelineState* pso{ nullptr };
	device()->CreatePipelineState(&desc, IID_PPV_ARGS(&pso));

	// use pso during rendering

	// when renderer shuts down
	release(pso);
}

void create_a_pipeline_state_object2() {
	struct {
		d3d12PipelineStateSubobjectRootSignature root_sig{ _root_signature };
		d3d12PipelineStateSubobjectVS vs{ _vs };
	} stream;

	auto pso = D3DX::create_pipeline_state(&stream, sizeof(stream));

	// use pso during rendering

	// when renderer shuts down
	release(pso);
}

}
