#include "D3D12Surface.h"

namespace WAVEENGINE::GRAPHICS::D3D12 {

namespace {

constexpr DXGI_FORMAT to_non_srgb(DXGI_FORMAT format) {
	if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	return format;
}

} // anonymous namespace

void d3d12Surface::create_swap_chain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format /*=default_back_buffer_format*/) {
	assert(factory && cmd_queue);
	release();

	if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allow_tearing, sizeof(u32))) && _allow_tearing) {
		_present_flags = DXGI_PRESENT_ALLOW_TEARING;
	}

	_format = format;

	// TODO: for test
	//_present_flags = _allow_tearing = 0;

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;				// do not use windows blending
	desc.BufferCount = buffer_count;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
	desc.Flags = _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Format = to_non_srgb(format);
	desc.Height = _window.height();
	desc.Width = _window.width();
	desc.SampleDesc.Count = 1;									// one sample in a single pixel -> forbid MSAA
	desc.SampleDesc.Quality = 0;								//
	desc.Scaling = DXGI_SCALING_STRETCH;						// stretch graphics to fill the whole window when resized
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;			// drop buffer content after presenting to achieve higher performance
	desc.Stereo = false;										// forbid VR

	IDXGISwapChain1* swap_chain;
	HWND hwnd{ (HWND)_window.handle() };
	DXCall(factory->CreateSwapChainForHwnd(cmd_queue, hwnd, &desc, nullptr, nullptr, &swap_chain)); // get IDXGISwapChain1 interface
	DXCall(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
	DXCall(swap_chain->QueryInterface(IID_PPV_ARGS(&_swap_chain))); // update to IDXGISwapChain4
	CORE::release(swap_chain);

	_current_bb_index = _swap_chain->GetCurrentBackBufferIndex(); // render to this back_buffer

	for (u32 i{ 0 }; i < buffer_count; ++i) {
		_render_target_data[i].rtview = CORE::rtv_heap().allocate();
	}

	finalize(); // this function will be reused when we resize the swap chain, so it's better to be decoupled with create_swap_chain() method
}

void d3d12Surface::present() const {
	assert(_swap_chain);
	// Present(1, 0) -> V-Sync On
	// Present(0, 0) -> V-Sync Off
	// Present(0, ALLOW_TEARING) -> tearing mode
	DXCall(_swap_chain->Present(0, _present_flags)); 
	_current_bb_index = _swap_chain->GetCurrentBackBufferIndex();
}

void d3d12Surface::finalize() {
	// create RTVs for back_buffers
	for (u32 i{ 0 }; i < buffer_count; ++i) {
		render_target_data& data{ _render_target_data[i] };
		assert(!data.resource); // we are going to create resource data, so there should not be any resource data inside
		DXCall(_swap_chain->GetBuffer(i, IID_PPV_ARGS(&data.resource))); // i is used to retrieve the specific internal buffers in swap chain

		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = _format;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// here we reuse the allocated descriptors to put the new rtv's in
		CORE::device()->CreateRenderTargetView(data.resource, &desc, data.rtview.cpu);
	}

	DXGI_SWAP_CHAIN_DESC desc{};
	DXCall(_swap_chain->GetDesc(&desc));
	const u32 width{ desc.BufferDesc.Width };
	const u32 height{ desc.BufferDesc.Height };
	assert(_window.width() == width && _window.height() == height);

	// set viewport and scissor rect
	_viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0, 1.0f };
	_scissor_rect = { 0, 0, static_cast<s32>(width), static_cast<s32>(height) };
}

void d3d12Surface::release() {
	for (u32 i{ 0 }; i < buffer_count; ++i) {
		render_target_data& data{ _render_target_data[i] };
		CORE::release(data.resource);
		CORE::rtv_heap().free(data.rtview);
	}

	CORE::release(_swap_chain);
}

void d3d12Surface::resize() {
	assert(_swap_chain);
	// do not free descriptor heaps because we still need them after resizing
	for (u32 i{0}; i < buffer_count; ++i) {
		CORE::release(_render_target_data[i].resource);
	}
	const u32 flags{ _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0ul };
	DXCall(_swap_chain->ResizeBuffers(buffer_count, 0, 0, DXGI_FORMAT_UNKNOWN, flags));
	_current_bb_index = _swap_chain->GetCurrentBackBufferIndex();

	finalize();

	DEBUG_OP(OutputDebugString(L"::D3D12 Surface Resized.\n"));
}

} 
