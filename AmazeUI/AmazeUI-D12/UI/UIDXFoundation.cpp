#include "UIDXFoundation.h"
#include "UIAnimation.h"
#include "UIElement.h"

using namespace std;

using namespace DirectX;
using namespace SimpleMath;
using namespace UIShape2D;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr; 


//const float gLoadFontSize = 24.0f;




namespace {
    inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt) {
        switch (fmt) {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
            default:                                return fmt;
        }
    }
    
    // Input layout for skeletal animation vertices
    // This matches the Vertex structure in UIModel with bone weights
    const D3D12_INPUT_ELEMENT_DESC SkinnedVertexInputElements[] = {
        { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    
    const D3D12_INPUT_LAYOUT_DESC SkinnedVertexInputLayout = {
        SkinnedVertexInputElements,
        5  // Number of elements
    };

    static const unsigned int c_AllowTearing    = 0x1;
	static const unsigned int c_EnableHDR       = 0x2;

	enum Descriptors {
        // System reserved (0-9)
        WindowsLogo = 0,
        MSYHFont = 1,
        SystemReservedCount = 10,
        
        // UI textures (10-9999)
        UITexturesStart = 10,
        UITexturesCount = 9990,
        
        // Font textures (10000-19999)
        FontTexturesStart = 10000,
        FontTexturesCount = 10000,
        
        // Model textures (20000-29999)
        ModelTexturesStart = 20000,
        ModelTexturesCount = 10000,
        
        TotalDescriptorCount = 30000
    };
};

// Configures the Direct3D device, and stores handles to it and the device context.
void UIGraphicsDeviceDX12::CreateDeviceDependentResources() {
#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
            debugController->EnableDebugLayer();
        }
        else {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())))) {
            _dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] = { 80 };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(_dxgiFactoryFlags, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf())));

    // Determines whether tearing support is available for fullscreen borderless windows.
    if (_options & c_AllowTearing) {
        BOOL allowTearing = FALSE;

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = _dxgiFactory.As(&factory5);
        if (SUCCEEDED(hr)) {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing) {
            _options &= ~c_AllowTearing;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
    }

    ComPtr<IDXGIAdapter1> adapter;
    GetAdapter(adapter.GetAddressOf());

    // Create the DX12 API device object.
    ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        _d3dMinFeatureLevel,
        IID_PPV_ARGS(_d3dDevice.ReleaseAndGetAddressOf())
        ));

    _d3dDevice->SetName(L"UIGraphicsSystem");

#ifndef NDEBUG
    // Configure debug device (if active).
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(_d3dDevice.As(&d3dInfoQueue))) {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
#endif

    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL s_featureLevels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels = {
        _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = _d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr)) {
        _d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else {
        _d3dFeatureLevel = _d3dMinFeatureLevel;
    }

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(_commandQueue.ReleaseAndGetAddressOf())));

    _commandQueue->SetName(L"UIGraphicsSystem");

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = _backBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(_rtvDescriptorHeap.ReleaseAndGetAddressOf())));

    _rtvDescriptorHeap->SetName(L"UIGraphicsSystem");

    _rtvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (_depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

        _dsvDescriptorHeap->SetName(L"UIGraphicsSystem");
    }

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < _backBufferCount; n++) {
        ThrowIfFailed(_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_commandAllocators[n].ReleaseAndGetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        _commandAllocators[n]->SetName(name);
    }

    // Create a command list for recording graphics commands.
    ThrowIfFailed(_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(_commandList.ReleaseAndGetAddressOf())));
    ThrowIfFailed(_commandList->Close());

    _commandList->SetName(L"UIGraphicsSystem");

    // Create a fence for tracking GPU execution progress.
    ThrowIfFailed(_d3dDevice->CreateFence(_fenceValues[_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())));
    _fenceValues[_backBufferIndex]++;

    _fence->SetName(L"UIGraphicsSystem");

    _fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!_fenceEvent.IsValid()) {
        throw exception("CreateEvent");
    }

    // D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
    // msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // msaaQualityLevels.SampleCount = 4;  // 4x MSAA
    // msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    
    // _d3dDevice->CheckFeatureSupport(
    //     D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
    //     &msaaQualityLevels,
    //     sizeof(msaaQualityLevels));
}

// These resources need to be recreated every time the window size is changed.
void UIGraphicsDeviceDX12::CreateWindowSizeDependentResources() {
    // if (!UIFrame::GetSingletonInstance()->GetWindowHandle()) {
    //     throw exception("Call SetWindowHWnd with a valid Win32 window handle");
    // }

    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < _backBufferCount; n++) {
        _renderTargets[n].Reset();
        _fenceValues[n] = _fenceValues[_backBufferIndex];
    }

    // Determine the render target size in pixels.
    UINT backBufferWidth = max<UINT>(static_cast<UINT>(_outputSize.right - _outputSize.left), 1u);
    UINT backBufferHeight = max<UINT>(static_cast<UINT>(_outputSize.bottom - _outputSize.top), 1u);
    DXGI_FORMAT backBufferFormat = NoSRGB(_backBufferFormat);

    // If the swap chain already exists, resize it, otherwise create one.
    if (_swapChain) {
        // If the swap chain already exists, resize it.
        HRESULT hr = _swapChain->ResizeBuffers(
            _backBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            (_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
            );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? _d3dDevice->GetDeviceRemovedReason() : hr);
            OutputDebugStringA(buff);
#endif
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        } else {
            ThrowIfFailed(hr);
        }
    }
    else {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = _backBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = (_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a swap chain for the window.
        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(_dxgiFactory->CreateSwapChainForHwnd(
            _commandQueue.Get(),
            UIFrame::GetSingletonInstance()->GetWindowHandle(),
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            swapChain.GetAddressOf()
            ));

        ThrowIfFailed(swapChain.As(&_swapChain));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(_dxgiFactory->MakeWindowAssociation(UIFrame::GetSingletonInstance()->GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER));
    }

    // Handle color space settings for HDR
    UpdateColorSpace();

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    for (UINT n = 0; n < _backBufferCount; n++) {
        ThrowIfFailed(_swapChain->GetBuffer(n, IID_PPV_ARGS(_renderTargets[n].GetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        _renderTargets[n]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = _backBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = _rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvDescriptor.ptr += static_cast<INT>(n) * _rtvDescriptorSize;
        _d3dDevice->CreateRenderTargetView(_renderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    _backBufferIndex = _swapChain->GetCurrentBackBufferIndex();

    if (_depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        D3D12_HEAP_PROPERTIES depthHeapProperties = {};
        depthHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        depthHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        depthHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        depthHeapProperties.CreationNodeMask = 0;
        depthHeapProperties.VisibleNodeMask = 0;

        D3D12_RESOURCE_DESC depthStencilDesc = {};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = backBufferWidth;
        depthStencilDesc.Height = backBufferHeight;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = _depthBufferFormat;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = _depthBufferFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(_d3dDevice->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(_depthStencil.ReleaseAndGetAddressOf())
            ));

        _depthStencil->SetName(L"Depth stencil");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = _depthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        _d3dDevice->CreateDepthStencilView(_depthStencil.Get(), &dsvDesc, _dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

// This method is called when the Win32 window is created (or re-created).
void UIGraphicsDeviceDX12::SetWindow(int width, int height) {
    _outputSize.left = _outputSize.top = 0;
    _outputSize.right = width;
    _outputSize.bottom = height;
}

// Helper method to clear the views before rendering.
void UIGraphicsDeviceDX12::ClearRenderTargetViews() {
    auto commandList = GetCommandList();

    // Clear the views.
    auto rtvDescriptor = GetRenderTargetView();
    auto dsvDescriptor = GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, UIColor::White.ToXMVECTORF32(), 0, nullptr);   //CornflowerBlue
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport.
    commandList->RSSetViewports(1, &UICameraUI2D::GetSingletonInstance()->GetViewport());
}

// Prepare the command list and render target for rendering.
void UIGraphicsDeviceDX12::PrepareCommandList(D3D12_RESOURCE_STATES beforeState) {
    // Reset command list and allocator.
    ThrowIfFailed(_commandAllocators[_backBufferIndex]->Reset());
    ThrowIfFailed(_commandList->Reset(_commandAllocators[_backBufferIndex].Get(), nullptr));

    if (beforeState != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        // Transition the render target into the correct state to allow for drawing into it.
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = _renderTargets[_backBufferIndex].Get();
        barrier.Transition.StateBefore = beforeState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        _commandList->ResourceBarrier(1, &barrier);
    }
}

// Present the contents of the swap chain to the screen.
void UIGraphicsDeviceDX12::ExecutePresent(D3D12_RESOURCE_STATES beforeState) {
    if (beforeState != D3D12_RESOURCE_STATE_PRESENT) {
        // Transition the render target to the state that allows it to be presented to the display.
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = _renderTargets[_backBufferIndex].Get();
        barrier.Transition.StateBefore = beforeState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        _commandList->ResourceBarrier(1, &barrier);
    }

    // Send the command list off to the GPU for processing.
    ThrowIfFailed(_commandList->Close());
    ID3D12CommandList* pCommandLists[] = { _commandList.Get() };
    _commandQueue->ExecuteCommandLists(1, pCommandLists);

    HRESULT hr;
    if (_options & c_AllowTearing) {
        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        hr = _swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
    else {
        // The first argument instructs DXGI to block until VSync, putting the application
        // to sleep until the next VSync. This ensures we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        hr = _swapChain->Present(1, 0);
    }

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? _d3dDevice->GetDeviceRemovedReason() : hr);
        OutputDebugStringA(buff);
#endif
        HandleDeviceLost();
    }
    else {
        ThrowIfFailed(hr);

        MoveToNextFrame();

        if (!_dxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            ThrowIfFailed(CreateDXGIFactory2(_dxgiFactoryFlags, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf())));
        }
    }
}

// Wait for pending GPU work to complete.
void UIGraphicsDeviceDX12::WaitForGpu() noexcept {
    if (_commandQueue && _fence && _fenceEvent.IsValid()) {
        // Schedule a Signal command in the GPU queue.
        UINT64 fenceValue = _fenceValues[_backBufferIndex];
        if (SUCCEEDED(_commandQueue->Signal(_fence.Get(), fenceValue))) {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(_fence->SetEventOnCompletion(fenceValue, _fenceEvent.Get()))) {
                WaitForSingleObjectEx(_fenceEvent.Get(), INFINITE, FALSE);

                // Increment the fence value for the current frame.
                _fenceValues[_backBufferIndex]++;
            }
        }
    }
}

// Prepare to render the next frame.
void UIGraphicsDeviceDX12::MoveToNextFrame() {
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = _fenceValues[_backBufferIndex];
    ThrowIfFailed(_commandQueue->Signal(_fence.Get(), currentFenceValue));

    // Update the back buffer index.
    _backBufferIndex = _swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (_fence->GetCompletedValue() < _fenceValues[_backBufferIndex])
    {
        ThrowIfFailed(_fence->SetEventOnCompletion(_fenceValues[_backBufferIndex], _fenceEvent.Get()));
        WaitForSingleObjectEx(_fenceEvent.Get(), INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    _fenceValues[_backBufferIndex] = currentFenceValue + 1;
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP. Otherwise throw an exception.
void UIGraphicsDeviceDX12::GetAdapter(IDXGIAdapter1** ppAdapter) {
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    ComPtr<IDXGIFactory6> factory6;
    HRESULT hr = _dxgiFactory.As(&factory6);
    if (SUCCEEDED(hr)) {
        for (UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
            adapterIndex++) {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), _d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr))) {
            #ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
            #endif
                break;
            }
        }
    }
#endif
    if (!adapter) {
        for (UINT adapterIndex = 0;
            SUCCEEDED(_dxgiFactory->EnumAdapters1(
                adapterIndex,
                adapter.ReleaseAndGetAddressOf()));
            ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), _d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr))) {
            #ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
            #endif
                break;
            }
        }
    }

#if !defined(NDEBUG)
    if (!adapter) {
        // Try WARP12 instead
        if (FAILED(_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())))) {
            throw exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!adapter) {
        throw exception("No Direct3D 12 device found");
    }

    *ppAdapter = adapter.Detach();
}

// Sets the color space for the swap chain in order to handle HDR output.
void UIGraphicsDeviceDX12::UpdateColorSpace() {
    DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    bool isDisplayHDR10 = false;

#if defined(NTDDI_WIN10_RS2)
    if (_swapChain) {
        ComPtr<IDXGIOutput> output;
        if (SUCCEEDED(_swapChain->GetContainingOutput(output.GetAddressOf()))) {
            ComPtr<IDXGIOutput6> output6;
            if (SUCCEEDED(output.As(&output6))) {
                DXGI_OUTPUT_DESC1 desc;
                ThrowIfFailed(output6->GetDesc1(&desc));

                if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
                    // Display output is HDR10.
                    isDisplayHDR10 = true;
                }
            }
        }
    }
#endif

    if ((_options & c_EnableHDR) && isDisplayHDR10) {
        switch (_backBufferFormat) {
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                // The application creates the HDR10 signal.
                colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                break;

            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                // The system creates the HDR10 signal; application uses linear values.
                colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
                break;

            default:
                break;
        }
    }

    _colorSpace = colorSpace;

    UINT colorSpaceSupport = 0;
    if (SUCCEEDED(_swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))
        && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
        ThrowIfFailed(_swapChain->SetColorSpace1(colorSpace));
    }
}

void UIGraphicsDeviceDX12::UIEffectManager::Create(ID3D12Device* device, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat) {
    RenderTargetState rtState(backBufferFormat, depthBufferFormat);

    // create common blend description
    D3D12_BLEND_DESC transparentBlendDesc = {};
    transparentBlendDesc.AlphaToCoverageEnable = FALSE;
    transparentBlendDesc.IndependentBlendEnable = FALSE;
    auto& rt = transparentBlendDesc.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.LogicOpEnable = FALSE;
    rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rt.LogicOp = D3D12_LOGIC_OP_NOOP;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC transparentBlendDescForTexture = transparentBlendDesc;
    transparentBlendDescForTexture.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;

    // create common depth description
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    // Create 2D Point Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
        pd.blendDesc = transparentBlendDesc;
        pd.depthStencilDesc = depthStencilDesc;
        p_pointEffect2D = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    // Create 2D Line Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
        pd.blendDesc = transparentBlendDesc;
        pd.depthStencilDesc = depthStencilDesc;
        p_lineEffect2D = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    // Create 2D Triangle Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pd.blendDesc = transparentBlendDesc;
        pd.depthStencilDesc = depthStencilDesc;
        p_triangleEffect2D = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    // Create 2D Textured Triangle Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionTexture::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pd.blendDesc = transparentBlendDescForTexture;
        pd.depthStencilDesc = depthStencilDesc;
        p_triangleTexturedEffect2D = make_unique<BasicEffect>(device, EffectFlags::Texture, pd);
    }

    // Create 3D Point Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
        pd.blendDesc = transparentBlendDesc;
        pd.depthStencilDesc = depthStencilDesc;
        p_pointEffect3D = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    // Create 3D Line Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
        pd.blendDesc = transparentBlendDesc;
        pd.depthStencilDesc = depthStencilDesc;
        p_lineEffect3D = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    // Create 3D Triangle Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pd.blendDesc = transparentBlendDesc;
        pd.depthStencilDesc = depthStencilDesc;
        p_triangleEffect3D = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }

    // Create 3D Textured Triangle Effect
    {
        EffectPipelineStateDescription pd(
            &VertexPositionTexture::InputLayout,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pd.blendDesc = transparentBlendDescForTexture;
        pd.depthStencilDesc = depthStencilDesc;
        p_triangleTexturedEffect3D = make_unique<BasicEffect>(device, EffectFlags::Texture, pd);
    }

    // Create 3D Shape Effect (for geometric primitives)
    {
        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);
        p_shapeEffect3D = make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
        p_shapeEffect3D->EnableDefaultLighting();
    }

    // Create Skinned Effect (for skeletal animation)
    {
        EffectPipelineStateDescription pd(
            &SkinnedVertexInputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pd.renderTargetState.sampleDesc.Count = 1;
        pd.renderTargetState.sampleDesc.Quality = 0;
        
        p_skinnedEffect3D = make_unique<SkinnedEffect>(device, EffectFlags::Lighting | EffectFlags::Texture, pd);
        p_skinnedEffect3D->EnableDefaultLighting();
        
        // Three-point lighting setup
        p_skinnedEffect3D->SetAmbientLightColor(DirectX::XMVectorSet(0.6f, 0.6f, 0.6f, 1.0f));
        
        p_skinnedEffect3D->SetLightEnabled(0, true);
        p_skinnedEffect3D->SetLightDiffuseColor(0, DirectX::XMVectorSet(0.9f, 0.9f, 0.9f, 1.0f));
        p_skinnedEffect3D->SetLightDirection(0, DirectX::XMVectorSet(-0.5773f, -0.5773f, -0.5773f, 0.0f));
        
        p_skinnedEffect3D->SetLightEnabled(1, true);
        p_skinnedEffect3D->SetLightDiffuseColor(1, DirectX::XMVectorSet(0.5f, 0.5f, 0.6f, 1.0f));
        p_skinnedEffect3D->SetLightDirection(1, DirectX::XMVectorSet(0.7071f, -0.3f, -0.5f, 0.0f));
        
        p_skinnedEffect3D->SetLightEnabled(2, true);
        p_skinnedEffect3D->SetLightDiffuseColor(2, DirectX::XMVectorSet(0.4f, 0.4f, 0.5f, 1.0f));
        p_skinnedEffect3D->SetLightDirection(2, DirectX::XMVectorSet(0.0f, 0.7071f, 0.7071f, 0.0f));
    }
}

void UIGraphicsDeviceDX12::UIEffectManager::Reset() {
    p_pointEffect2D.reset();
    p_lineEffect2D.reset();
    p_triangleEffect2D.reset();
    p_triangleTexturedEffect2D.reset();
    p_pointEffect3D.reset();
    p_lineEffect3D.reset();
    p_triangleEffect3D.reset();
    p_triangleTexturedEffect3D.reset();
    p_shapeEffect3D.reset();
    p_skinnedEffect3D.reset();
}

// Get texture rect
UIRECT UITextureManager::Get2DTextureRect(ID3D12Resource* texture) {
    UIRECT rect = NULL_RECT;
    if (!texture) {
        return rect;
    }
    
    D3D12_RESOURCE_DESC desc = texture->GetDesc();
    
    // check if it is a 2D texture
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        return rect;
    }
    
    rect.left = 0;
    rect.top = 0;
    rect.right = static_cast<LONG>(desc.Width);
    rect.bottom = static_cast<LONG>(desc.Height);
    
    return rect;
}

// ComPtr version overload
UIRECT UITextureManager::Get2DTextureRect(const ComPtr<ID3D12Resource>& texture) {
    return Get2DTextureRect(texture.Get());
}

bool UITextureManager::Get2DImageSize(const wstring& imagePath, const UIColor& colorKey, UIRECT& textureRect) {
    // load texture resource
    size_t textureIndex = 0;

    // check if the image is a DDS image
    if (imagePath.find(L".dds") != wstring::npos) {
        if (!GetDDSTextureIndexFromFile(imagePath, colorKey, textureIndex)) {
            return false;
        }
    } else {
        if (!GetWICTextureIndexFromFile(imagePath, colorKey, textureIndex)) {
            return false;
        }
    }

    // get texture resource
    TextureResource& resource = _textureResources[textureIndex];

    // get original texture size
    textureRect = Get2DTextureRect(resource._texture);
    return true;
}

bool UITextureManager::Get2DImageSize(const wstring& dllPath, UINT id, const UIColor& colorKey, UIRECT& textureRect) {
    // load texture resource
    size_t textureIndex = 0;
    if (!GetWICTextureIndexFromDLL(dllPath, id, colorKey, textureIndex)) {
        return false;
    }

    // get texture resource
    TextureResource& resource = _textureResources[textureIndex];

    // get original texture size
    textureRect = Get2DTextureRect(resource._texture);
    return true;
}

// convert image data from file to transparent by color key
bool UITextureManager::ConvertImageTransparencyByWIC(ComPtr<IWICImagingFactory>& wicFactory, ComPtr<IWICBitmapDecoder>& decoder, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) {
    if (!wicFactory || !decoder || !colorKey.IsValid()) {
        return false;
    }
    
    // get first frame
    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        return false;
    }

    // get image size
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr)) {
        return false;
    }

    // create format converter
    ComPtr<IWICFormatConverter> converter;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        return false;
    }

    // initialize format converter to RGBA format
    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeCustom
    );
    if (FAILED(hr)) {
        return false;
    }

    // create temporary buffer to store pixel data
    imageData.resize(width * height * 4);
    hr = converter->CopyPixels(
        nullptr,
        width * 4,
        static_cast<UINT>(imageData.size()),
        imageData.data()
    );
    if (FAILED(hr)) {
        return false;
    }

    // process transparent color
    for (size_t i = 0; i < imageData.size(); i += 4) {
        if (imageData[i] == colorKey._r && imageData[i+1] == colorKey._g && imageData[i+2] == colorKey._b) {
            imageData[i] = 0; imageData[i+1] = 0; imageData[i+2] = 0; imageData[i+3] = 0; // set alpha to 0
        }
    }

    return true;
}

bool UITextureManager::ConvertImageTransparencyByWIC(const void* data, size_t size, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) {   
    if (data == nullptr || size == 0 || !colorKey.IsValid()) {
        return false;
    }

    // create WIC imaging factory
    ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory)
    );
    if (FAILED(hr)) {
        return false;
    }

    // create stream
    ComPtr<IWICStream> stream;
    hr = wicFactory->CreateStream(&stream);
    if (FAILED(hr)) {
        return false;
    }

    // initialize stream from memory
    hr = stream->InitializeFromMemory(
        static_cast<BYTE*>(const_cast<void*>(data)),
        static_cast<DWORD>(size)
    );
    if (FAILED(hr)) {
        return false;
    }

    // create decoder from stream
    ComPtr<IWICBitmapDecoder> decoder;
    hr = wicFactory->CreateDecoderFromStream(
        stream.Get(),
        nullptr,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );
    if (FAILED(hr)) {
        return false;
    }

    return ConvertImageTransparencyByWIC(wicFactory, decoder, colorKey, imageData, width, height);
}

bool UITextureManager::ConvertImageTransparencyByWIC(const wstring& filePath, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) {
    if (filePath.empty() || !colorKey.IsValid()) {
        return false;
    }

    // create WIC imaging factory
    ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory)
    );
    if (FAILED(hr)) {
        return false;
    }

    // create decoder from file
    ComPtr<IWICBitmapDecoder> decoder;
    hr = wicFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );
    if (FAILED(hr)) {
        return false;
    }

    return ConvertImageTransparencyByWIC(wicFactory, decoder, colorKey, imageData, width, height);
}

bool UITextureManager::ConvertImageTransparencyByDDS(const wstring& filePath, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) { 
    auto graphicsSystem = UIGraphicsSystem::GetSingletonInstance();
    
    ComPtr<ID3D12Resource> texture;
    unique_ptr<uint8_t[]> ddsData;
    vector<D3D12_SUBRESOURCE_DATA> subresources;
    DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;
    bool isCubeMap = false;

    // Load DDS texture
    HRESULT hr = LoadDDSTextureFromFile(
        graphicsSystem->_graphicsDevice->GetD3DDevice(),
        filePath.c_str(),
        texture.GetAddressOf(),
        ddsData,
        subresources,
        0,      // maxsize
        &alphaMode,
        &isCubeMap
    );

    if (FAILED(hr) || subresources.empty()) {
        return false;
    }

    // get image size
    D3D12_RESOURCE_DESC desc = texture->GetDesc();
    width = static_cast<UINT>(desc.Width);
    height = static_cast<UINT>(desc.Height);

    // create temporary buffer to store pixel data
    imageData.resize(width * height * 4);
    memcpy(imageData.data(), subresources[0].pData, width * height * 4);

    // process transparent color
    for (size_t i = 0; i < imageData.size(); i += 4) {
        if (imageData[i] == colorKey._r && imageData[i+1] == colorKey._g && imageData[i+2] == colorKey._b) {
            imageData[i] = 0; imageData[i+1] = 0; imageData[i+2] = 0; imageData[i+3] = 0; // set alpha to 0
        }
    }

    return true;
}

// Texture creation helper function
bool UITextureManager::CreateTextureFromImageData(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, 
                                                ComPtr<ID3D12Resource>& texture, const std::vector<uint8_t>& imageData, UINT width, UINT height) {
    
    // Create texture description
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    
    // Create texture resource
    D3D12_HEAP_PROPERTIES defaultHeapProperties = {};
    defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    defaultHeapProperties.CreationNodeMask = 0;
    defaultHeapProperties.VisibleNodeMask = 0;
    
    HRESULT hr = device->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(texture.ReleaseAndGetAddressOf())
    );
    
    if (FAILED(hr)) {
        return false;
    }

    // Create subresource data
    D3D12_SUBRESOURCE_DATA subResData = {};
    subResData.pData = imageData.data();
    subResData.RowPitch = width * 4;  // RGBA = 4 bytes per pixel
    subResData.SlicePitch = subResData.RowPitch * height;

    // Upload texture data
    resourceUpload.Upload(
        texture.Get(),
        0,
        &subResData,
        1
    );

    // Transition resource to pixel shader resource state
    resourceUpload.Transition(
        texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    return true;
}

bool UITextureManager::GetWICTextureIndexFromFile(const wstring& filePath, const UIColor& colorKey, size_t& textureIndex) {
    // use filePath as key
    wstring resourceKey = filePath + (colorKey.IsValid() ? L"#" + colorKey.ToWStringForU32() : L"");

    auto it = _textureResourceMap.find(resourceKey);
    if (it == _textureResourceMap.end()) {
        auto graphicsSystem = UIGraphicsSystem::GetSingletonInstance();
        auto device = graphicsSystem->_graphicsDevice->GetD3DDevice();
        
        // Load texture with ResourceUploadBatch
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();
        
        // create texture resource
        TextureResource resource;

        if (colorKey.IsValid()) {
            // if color key is specified, need to load image data and process transparent color
            vector<uint8_t> imageData;
            UINT width, height;
            
            if (!ConvertImageTransparencyByWIC(filePath, colorKey, imageData, width, height) ||
                !CreateTextureFromImageData(device, resourceUpload, resource._texture, imageData, width, height)) {
                return false;
            }
        } else {
            // if color key is not specified, create texture directly
            HRESULT hr = CreateWICTextureFromFile(
                device,
                resourceUpload, 
                filePath.c_str(),
                resource._texture.ReleaseAndGetAddressOf()
            );
            if (FAILED(hr)) {
                return false;
            }
        }

        // get next available descriptor index
        size_t descriptorIndex = _textureResourceMap.size() + Descriptors::UITexturesStart;
        
        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetCpuHandle(descriptorIndex));
        
        // save GPU descriptor handle
        resource._gpuDescriptor = graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetGpuHandle(descriptorIndex);

        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(graphicsSystem->_graphicsDevice->GetCommandQueue());
        uploadResourcesFinished.wait();
        
        // save texture resource and record index
        _textureResources.push_back(move(resource));
        textureIndex = _textureResources.size() - 1;
        _textureResourceMap[resourceKey] = textureIndex;
    } else {
        textureIndex = it->second;
    }

    return true;
}   

bool UITextureManager::GetWICTextureIndexFromDLL(const wstring& dllPath, UINT id, const UIColor& colorKey, size_t& textureIndex) {
    // use path+id as key
    wstring resourceKey = dllPath + L"#" + to_wstring(id) + (colorKey.IsValid() ? L"#" + colorKey.ToWStringForU32() : L"");

    auto it = _textureResourceMap.find(resourceKey);
    if (it == _textureResourceMap.end()) {
        // load DLL resource
        DLLResourceRAII dllRAII(dllPath);
        HMODULE& hDLL = dllRAII._hDLL;
        if (!hDLL) {
            return false;
        }

        // find resource
        HRSRC hRes = FindResourceW(hDLL, MAKEINTRESOURCEW(id), RT_RCDATA);
        if (!hRes) {
            return false;
        }
        
        // load resource
        HGLOBAL hGlobal = LoadResource(hDLL, hRes);
        const void* data = LockResource(hGlobal);
        DWORD size = SizeofResource(hDLL, hRes);

        auto graphicsSystem = UIGraphicsSystem::GetSingletonInstance();
        auto device = graphicsSystem->_graphicsDevice->GetD3DDevice();
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        // create texture resource
        TextureResource resource;
        
        if (colorKey.IsValid()) {
            vector<uint8_t> imageData;
            UINT width, height;

            if (!ConvertImageTransparencyByWIC(data, size, colorKey, imageData, width, height) ||
                !CreateTextureFromImageData(device, resourceUpload, resource._texture, imageData, width, height)) {
                return false;
            }
        } else {
            // create texture resource
            HRESULT hr = CreateWICTextureFromMemory(
                device,
                resourceUpload,
                static_cast<const uint8_t*>(data),
                size,
                resource._texture.ReleaseAndGetAddressOf()
            );
            if (FAILED(hr)) {
                return false;
            }
        }

        // get next available descriptor index
        size_t descriptorIndex = _textureResourceMap.size() + Descriptors::UITexturesStart;
            
        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetCpuHandle(descriptorIndex));

        // save GPU descriptor handle
        resource._gpuDescriptor = graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetGpuHandle(descriptorIndex);

        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(graphicsSystem->_graphicsDevice->GetCommandQueue());
        uploadResourcesFinished.wait();

        // save texture resource and record index
        _textureResources.push_back(move(resource));
        textureIndex = _textureResources.size() - 1;
        _textureResourceMap[resourceKey] = textureIndex;
    }
    else {
        textureIndex = it->second;
    }

    return true;
}

bool UITextureManager::GetDDSTextureIndexFromFile(const wstring& filePath, const UIColor& colorKey, size_t& textureIndex)
{
    // use filePath as key
    wstring resourceKey = filePath + (colorKey.IsValid() ? L"#" + colorKey.ToWStringForU32() : L"");

    auto it = _textureResourceMap.find(resourceKey);
    if (it == _textureResourceMap.end()) {
        auto graphicsSystem = UIGraphicsSystem::GetSingletonInstance();
        auto device = graphicsSystem->_graphicsDevice->GetD3DDevice();
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();
        
        // create texture resource
        TextureResource resource;

        if (colorKey.IsValid()) {
            // if color key is specified, need to load image data and process transparent color
            vector<uint8_t> imageData;
            UINT width, height;
            
            if (!ConvertImageTransparencyByDDS(filePath, colorKey, imageData, width, height) ||
                !CreateTextureFromImageData(device, resourceUpload, resource._texture, imageData, width, height)) {
                return false;
            }
        } else {
            // if color key is not specified, create texture directly
            HRESULT hr = CreateDDSTextureFromFile(
                device,
                resourceUpload, 
                filePath.c_str(),
                resource._texture.ReleaseAndGetAddressOf()
            );
            if (FAILED(hr)) {
                return false;
            }
        }

        // get next available descriptor index
        size_t descriptorIndex = _textureResourceMap.size() + Descriptors::UITexturesStart;

        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetCpuHandle(descriptorIndex));
        
        // save GPU descriptor handle
        resource._gpuDescriptor = graphicsSystem->_graphicsDevice->p_resourceDescriptors->GetGpuHandle(descriptorIndex);
        
        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(graphicsSystem->_graphicsDevice->GetCommandQueue());
        uploadResourcesFinished.wait();
        
        // save texture resource and record index
        _textureResources.push_back(move(resource));
        textureIndex = _textureResources.size() - 1;
        _textureResourceMap[resourceKey] = textureIndex;
    }
    else {
        textureIndex = it->second;
    }

    return true;
}

// Constructor for UIGraphicsSystem.
UIGraphicsSystem::UIGraphicsSystem() noexcept(false) {
    _graphicsDevice = std::make_unique<UIGraphicsDeviceDX12>();
}

// Destructor for UIGraphicsSystem.
UIGraphicsSystem::~UIGraphicsSystem() {
    Shutdown();
}

// ============================================================================
// UIGraphicsSystem - High-Level System API Implementation
// ============================================================================
bool UIGraphicsSystem::Initialize(const UIGraphicsDeviceHAL::Desc& desc) {
    // 1. Initialize HAL backend
    if (!_graphicsDevice->Initialize(desc)) {
        return false;
    }

    // 2. System-level initialization
    // TODO: User implementation here
    CreateResourcesFT();

    return true;
}

void UIGraphicsSystem::Shutdown() {
    // 1. System-level cleanup
    // TODO: User implementation here

    // 2. Shutdown HAL backend
    _graphicsDevice->Shutdown();
}

void UIGraphicsSystem::Render() {
    // HAL Begin frame
    _graphicsDevice->BeginFrame();

    // Set current camera to UI2D camera and reset clip rect
    ResetClipRect();

    // Clear all batch data for this frame
    ClearAllBatches();

    // Draw top container
    UIFrame::GetSingletonInstance()->GetTopUIContainer()->Draw();

    // Draw animations
    UIAnimationManage::GetSingletonInstance()->DrawAnimations();

    // Execute all batch rendering
    ExecuteAllBatches();

    // HAL Present
    _graphicsDevice->Present();

    // HAL End frame
    _graphicsDevice->EndFrame();
}

bool UIGraphicsSystem::HandleWindowResize(int width, int height) {
    // TODO: UIGraphicsSystem specific logic here

    // HAL backend handle window resize
    return _graphicsDevice->HandleWindowResize(width, height);
}

void UIGraphicsSystem::HandleDeviceLost() {
    // TODO: UIGraphicsSystem specific logic here
    ResetResourcesFT();
    CreateResourcesFT();

    // HAL backend handle device lost
    _graphicsDevice->HandleDeviceLost();
}

UIRECT UIGraphicsSystem::GetOutputSize() const {
    // HAL backend get output size
    return _graphicsDevice->GetOutputSize();
}

void UIGraphicsSystem::Calculate2DPoint(const UIVector2F& point, UIVector2F& p) {
    p._x = point._x + 0.5f;
    p._y = point._y + 0.5f;
}

void UIGraphicsSystem::Calculate2DLinePoints(const UIVector2F& start, const UIVector2F& end, UIVector2F& p1, UIVector2F& p2) {
    // for the "Top-Left Rule" in the triangle rending
    // Horizontal line: Move 1 pixel down, add 1 pixel right  
    // Vertical line: Move 1 pixel right, add 1 pixel down
    // Diagonal line (right endpoint): Move 1 pixel right  
    // Diagonal line (bottom endpoint): Move 1 pixel down 
    p1 = start;
    p2 = end;
    if (fabs(p1._x - p2._x) < 0.001f) {
        p1._x += 1.0f;
        p2._x += 1.0f;
        p1._y > p2._y ? p1._y += 1.0f : p2._y += 1.0f;

    } else if (fabs(p1._y - p2._y) < 0.001f) {
        p1._y += 1.0f;
        p2._y += 1.0f;
        p1._x > p2._x ? p1._x += 1.0f : p2._x += 1.0f;
    } else {
        p1._x > p2._x ? p1._x += 1.0f : p2._x += 1.0f;
        p1._y > p2._y ? p1._y += 1.0f : p2._y += 1.0f;
    }
}

void UIGraphicsSystem::Calculate2DRectPoints(const UIVector2F& start, const UIVector2F& end, UIVector2F& ps, UIVector2F& pe) {
    ps._x = start._x <= end._x ? start._x : end._x;
    ps._y = start._y <= end._y ? start._y : end._y;
    pe._x = start._x > end._x ? start._x : end._x;
    pe._y = start._y > end._y ? start._y : end._y;
    pe._x += 1.0f;
    pe._y += 1.0f;
}

void UIGraphicsSystem::BeginScreenClipRect(const UIRECT& clipRC, bool execute) {
    UIRECT tempRC = NULL_RECT;

    if (clipRC.left > clipRC.right || clipRC.top > clipRC.bottom) {
        tempRC = NULL_RECT;
    } else {
        if (_clipRectStack.size() > 0) {
            UIRECT preClipRC = _clipRectStack.top();

            // calculate cross clip rect
            if (CompareRects()(preClipRC, NULL_RECT)==false) {
                tempRC = IntersectRects()(preClipRC, clipRC);
            }
        } else {
            tempRC = clipRC;
        }
    }

    // Set the scissor rect
    D3D12_RECT scissorRect = { 
        tempRC.left > 1 ? tempRC.left - 1 : 0,
        tempRC.top > 1 ? tempRC.top - 1 : 0,
        tempRC.right + 1,
        tempRC.bottom + 1
    };
    _clipRectStack.push(scissorRect);

    if (execute) {
        ExecuteClipRect(scissorRect);
    }
}

void UIGraphicsSystem::EndScreenClipRect(bool execute) {
    _clipRectStack.pop();

    if (execute) {
        ExecuteClipRect(GetCurrentClipRect());
    }
}

UIRECT UIGraphicsSystem::GetCurrentClipRect() const {
    if (_clipRectStack.size() > 0) {
        return _clipRectStack.top();
    }
    return NULL_RECT;
}

void UIGraphicsSystem::ExecuteClipRect(const UIRECT& clipRC) {
    auto commandList = _graphicsDevice->GetCommandList();

    if (CompareRects()(clipRC, NULL_RECT)) {
        // If the clip rectangle is null, reset the scissor rect
        D3D12_RECT fullscreenRect = {0, 0, LONG_MAX, LONG_MAX};
        commandList->RSSetScissorRects(1, &fullscreenRect);
    } else {
        commandList->RSSetScissorRects(1, &clipRC);
    }
}

void UIGraphicsSystem::ResetClipRect() {
    _clipRectStack = stack<UIRECT>();
    ExecuteClipRect(NULL_RECT);
}

/*************************************************** FreeType APIs Implementation ***************************************************/

void UIGraphicsSystem::CreateResourcesFT() {
    // create FreeType library
    if (FT_Init_FreeType(&_ftLibrary)) {
        OutputDebugString(L"FreeType initialization failed\n");
        return;
    }
    
    // load frequently used characters
}

// reset FreeType resources
void UIGraphicsSystem::ResetResourcesFT() {
    // reset _ftLibrary
    FT_Done_FreeType(_ftLibrary);

    // reset _ftSizeFontMap
    for (auto& pair : _ftSizeFontMap) {
        FT_Done_Face(pair.second._ftFace);
        FT_Done_Face(pair.second._ftFaceBackup);
    }
    //
    _ftSizeFontMap.clear();
}

bool UIGraphicsSystem::GetFTSizeFont(float fontSize, FTSizeFont& ftSizeFont) {
    // check if the font size already exists
    auto it = _ftSizeFontMap.find(fontSize);
    if (it == _ftSizeFontMap.end()) {
        // create new font
        FTSizeFont ftSizeFontCache;

        // load font
        wchar_t strFilePath[MAX_PATH] = {};
        FindMediaFile(strFilePath, MAX_PATH, L"times.ttf");
        if (FT_New_Face(_ftLibrary, WSTR_TO_STR(strFilePath).c_str(), 0, &ftSizeFontCache._ftFace)) {
            return false;
        }

        memset(strFilePath, 0, MAX_PATH);
        FindMediaFile(strFilePath, MAX_PATH, L"simsun.ttc");
        if (FT_New_Face(_ftLibrary, WSTR_TO_STR(strFilePath).c_str(), 0, &ftSizeFontCache._ftFaceBackup)) {
            return false;
        }

        // set font size
        if (FT_Set_Pixel_Sizes(ftSizeFontCache._ftFace, 0, (FT_UInt)fontSize)) {
            FT_Done_Face(ftSizeFontCache._ftFace);
            return false;
        }

        if (FT_Set_Pixel_Sizes(ftSizeFontCache._ftFaceBackup, 0, (FT_UInt)fontSize)) {
            FT_Done_Face(ftSizeFontCache._ftFaceBackup);
            return false;
        }

        // save font metrics
        int ascent = ftSizeFontCache._ftFace->size->metrics.ascender >> 6;
        int descent = -(ftSizeFontCache._ftFace->size->metrics.descender >> 6);
        int height = ftSizeFontCache._ftFace->size->metrics.height >> 6;

        int ascent2 = ftSizeFontCache._ftFaceBackup->size->metrics.ascender >> 6;
        int descent2 = -(ftSizeFontCache._ftFaceBackup->size->metrics.descender >> 6);
        int height2 = ftSizeFontCache._ftFaceBackup->size->metrics.height >> 6;

        ftSizeFontCache._ftFontAscent = max(ascent, ascent2);
        ftSizeFontCache._ftFontDescent = max(descent, descent2);
        ftSizeFontCache._ftFontHeight = max(height, height2);

        _ftSizeFontMap[fontSize] = ftSizeFontCache;

        ftSizeFont = _ftSizeFontMap[fontSize];
    } else {
        ftSizeFont = it->second;
    }

    return true;
}

/*
  FreeType Glyph Bitmap Layout and Metrics
  
                       bitmap.width
                    <-------------->
                    +---------------+      ^
                    |               |      |
                    |               |      | bitmap.rows
      baseline      |    glyph      |      |
      ==============+===============+======v================
                    |               |      
                    +---------------+      
                    ^              ^
                    |              |
                    |              +---- right edge of bitmap
                    |
                    +---- left edge of bitmap

      <-bitmap_left->|              |<----------advance.x---------->|
      
      ^
      |
      origin point       


    horiBearingX >> 6: from the origin point to the left edge of the glyph bitmap
    horiBearingY >> 6: from the baseline to the top edge of the glyph bitmap
    advance.x >> 6: the horizontal distance after rendering this glyph
    bitmap_left: the horizontal distance from the left edge of the glyph bitmap to the origin point
    bitmap_top: the vertical distance from the top edge of the glyph bitmap to the baseline

    For space character:
    bitmap.width & bitmap.rows: 0
    bitmap_left & bitmap_top: 0
    advance.x >> 6 will decide the width of the space
    Note: >> 6 is a bit shift operation that converts FreeType's 1/64 pixel units to integer pixel units.
*/
bool UIGraphicsSystem::GetCharTextureResourceFT(const wchar_t& wch, float fontSize, const UIColor& color, size_t& textureIndex) {
    // use wchar+color as the key
    wstring resourcekey = wch + (L"#" + to_wstring(fontSize)) + (color.IsValid() ? L"#" + color.ToWStringForU32() : L"");

    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);

    // check if the resource already exists
    auto it = _charTextureResourceMap.find(resourcekey);
    if (it == _charTextureResourceMap.end()) {
        auto device = _graphicsDevice->GetD3DDevice();
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();
        
        // load char
        FT_GlyphSlot slot;
        if (FT_Get_Char_Index(ftSizeFont._ftFace, wch) != 0) {
            if (FT_Load_Char(ftSizeFont._ftFace, wch, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT)) {
                OutputDebugString(L"FreeType load character failed\n");
                return false;
            }
            slot = ftSizeFont._ftFace->glyph;
        } else {
            if (FT_Load_Char(ftSizeFont._ftFaceBackup, wch, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT)) {
                OutputDebugString(L"FreeType load character failed\n");
                return false;
            }
            slot = ftSizeFont._ftFaceBackup->glyph;
        }

        // get bitmap
        FT_Bitmap& bitmap = slot->bitmap;

        //
        CharTextureResource resource;
        
        resource._width = bitmap.width;
        resource._height = bitmap.rows;
        // resource._bearingX = slot->metrics.horiBearingX >> 6;
        // resource._bearingY = slot->metrics.horiBearingY >> 6;
        resource._advance = slot->advance.x >> 6;
        resource._left = slot->bitmap_left;
        resource._top = slot->bitmap_top;
        // OutputDebugStringW(format(L"wch: {}\n", wch).c_str());
        // OutputDebugStringW(format(L"bitmap.width: {}\n", bitmap.width).c_str());
        // OutputDebugStringW(format(L"bitmap.rows: {}\n", bitmap.rows).c_str());
        // OutputDebugStringW(format(L"horiBearingX: {}\n", slot->metrics.horiBearingX >> 6).c_str());
        // OutputDebugStringW(format(L"horiBearingY: {}\n", slot->metrics.horiBearingY >> 6).c_str());
        // OutputDebugStringW(format(L"advance.x: {}\n", slot->advance.x >> 6).c_str());
        // OutputDebugStringW(format(L"bitmap_left: {}\n", slot->bitmap_left).c_str());
        // OutputDebugStringW(format(L"bitmap_top: {}\n", slot->bitmap_top).c_str());

        UINT& width = resource._width;
        UINT& height = resource._height;

        // if the character has no bitmap (such as space)
        // space' widht&height is 0, but advance has value
        bool isNoBitmap = false;
        if (width == 0 || height == 0) {
            width = resource._advance; 
            height = 1;
            isNoBitmap = true;
        }

        // get image data from bitmap
        vector<uint8_t> imageData(width * height * 4);
        memset(imageData.data(), 0, imageData.size());

        for (UINT y = 0; y < height; y++) {
            for (UINT x = 0; x < width; x++) {
                // get grey value
                uint8_t grey = isNoBitmap ? 0 : bitmap.buffer[y * bitmap.pitch + x];

                UINT index = (y * width + x) * 4;
                if (grey > 0) {
                    // convert to RGBA format
                    imageData[index] = color._r; // R
                    imageData[index + 1] = color._g; // G
                    imageData[index + 2] = color._b; // B
                    imageData[index + 3] = (grey * color._a) / 255; // A
                } else {
                    // make sure the background is completely transparent
                    imageData[index] = 0;
                    imageData[index + 1] = 0;
                    imageData[index + 2] = 0;
                    imageData[index + 3] = 0;       // completely transparent
                }
            }
        }

        if (!_textureManager.CreateTextureFromImageData(device, resourceUpload, resource._texture, imageData, width, height)) {
            return false;
        }

        // get next available descriptor index
        size_t descriptorIndex = _charTextureResources.size() + Descriptors::FontTexturesStart;
        
        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), _graphicsDevice->p_resourceDescriptors->GetCpuHandle(descriptorIndex));
        
        // save GPU descriptor handle
        resource._gpuDescriptor = _graphicsDevice->p_resourceDescriptors->GetGpuHandle(descriptorIndex);

        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(_graphicsDevice->GetCommandQueue());
        uploadResourcesFinished.wait();
        
        // save texture resource and record index
        _charTextureResources.push_back(move(resource));
        textureIndex = _charTextureResources.size() - 1;
        _charTextureResourceMap[resourcekey] = textureIndex;
    } else {
        textureIndex = it->second;
    }

    return true;
}

void UIGraphicsSystem::Draw2DCharTextureFT(size_t textureIndex, UIVector2F position, float z, float scale, UCHAR alpha, int renderLevel) {
    position._x = round(position._x);
    position._y = round(position._y);

    // get texture resource
    CharTextureResource& resource = _charTextureResources[textureIndex];

    // get original texture size
    const UIRECT textureRect = _textureManager.Get2DTextureRect(resource._texture);

    // calculate destination rectangle end point
    UIVector2F dstEnd;
    dstEnd._x = position._x + (float)(GetRectWidth()(textureRect)) * scale;
    dstEnd._y = position._y + (float)(GetRectHeight()(textureRect)) * scale;

    // Get view space Z value using 2D transform
    float viewZ = UICameraUI2D::GetSingletonInstance()->CalculateViewZByOrtho(z);

    // Create vertex data
    vector<VertexPositionTexture> vertices = {
        { {position._x, position._y, viewZ}, XMFLOAT2(0.f, 0.f) },
        { {dstEnd._x, position._y, viewZ}, XMFLOAT2(1.f, 0.f) },
        { {position._x, dstEnd._y, viewZ}, XMFLOAT2(0.f, 1.f) },
        { {dstEnd._x, dstEnd._y, viewZ}, XMFLOAT2(1.f, 1.f) }
    };

    // define index data
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };

    RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleTexturedEffect2D, 
                             resource._gpuDescriptor, _graphicsDevice->p_states->LinearClamp(), alpha, GetCurrentClipRect(), UICameraUI2D::GetSingletonInstance(), renderLevel);
}

void UIGraphicsSystem::Draw3DCharTextureFT(size_t textureIndex, UIVector2F position, float z, float scale, UCHAR alpha, int renderLevel, const XMMATRIX& transformMatrix) {
    position._x = round(position._x);
    //position._y = round(position._y);

    // get original texture size 
    const UIRECT textureRect = _textureManager.Get2DTextureRect(_charTextureResources[textureIndex]._texture);

    UIVector2F dstEnd;
    dstEnd._x = position._x + (float)(GetRectWidth()(textureRect)) * scale;
    dstEnd._y = position._y + (float)(GetRectHeight()(textureRect)) * scale;

    UIVector2F ps, pe;
    Calculate2DRectPoints(position, dstEnd, ps, pe);
    vector<UIVector3F> wps;
    UIZPlaneTransform::TransformRectPoints(transformMatrix, ps, pe, z, wps);

    // create vertices with texture coordinates
    vector<VertexPositionTexture> vertices = {
        { {wps[0]._x, wps[0]._y, wps[0]._z - 50.0f}, XMFLOAT2(0.f, 0.f) },
        { {wps[1]._x, wps[1]._y, wps[1]._z - 50.0f}, XMFLOAT2(1.f, 0.f) },
        { {wps[2]._x, wps[2]._y, wps[2]._z - 50.0f}, XMFLOAT2(0.f, 1.f) },
        { {wps[3]._x, wps[3]._y, wps[3]._z - 50.0f}, XMFLOAT2(1.f, 1.f) }
    };
    // Define indices for two triangles
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };

    RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleTexturedEffect3D, 
                             _charTextureResources[textureIndex]._gpuDescriptor, _graphicsDevice->p_states->LinearClamp(), 
                             alpha, GetCurrentClipRect(), UICameraUI3D::GetSingletonInstance(), renderLevel);
}

UIVector2F UIGraphicsSystem::CalculateTextPosition(const std::wstring& text, const UIRECT& rc, UIFontPos alignment, float fontSize, float lineSpacing) {
    // measure text size
    UISIZE textSize = GetTextSizeFT(text, fontSize, lineSpacing);
    float textWidth = (float)textSize.cx;
    float textHeight = (float)textSize.cy;
    
    UIVector2F position;

    // Horizontal alignment
    switch (alignment) {
        case UIFontPos::TopLeft:
        case UIFontPos::MiddleLeft:
        case UIFontPos::BottomLeft:
            position._x = (float)rc.left;
            break;
        
        case UIFontPos::TopCenter:
        case UIFontPos::MiddleCenter:
        case UIFontPos::BottomCenter:
            position._x = rc.left + (rc.right - rc.left - textWidth) / 2.f;
            break;
        
        case UIFontPos::TopRight:
        case UIFontPos::MiddleRight:
        case UIFontPos::BottomRight:
            position._x = rc.right - textWidth;
            break;
    }

    // Vertical alignment (baseline Y position)
    switch (alignment) {
        case UIFontPos::TopLeft:
        case UIFontPos::TopCenter:
        case UIFontPos::TopRight:
            position._y = (float)rc.top;
            break;
        
        case UIFontPos::MiddleLeft:
        case UIFontPos::MiddleCenter:
        case UIFontPos::MiddleRight:
            position._y = rc.top + (GetRectHeight()(rc) - textHeight) / 2.f;
            break;
        
        case UIFontPos::BottomLeft:
        case UIFontPos::BottomCenter:
        case UIFontPos::BottomRight:
            position._y = rc.bottom - textHeight;
            break;
    }

    return position;
}

void UIGraphicsSystem::Draw2DTextFT(const wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, int renderLevel) {
    float x = position._x;
    float y = position._y;

    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);

    int baselineYFromTop = ftSizeFont._ftFontAscent;
    
    // render each character
    for (wchar_t ch : text) {      
        // get or create texture
        size_t textureIndex;
        if (!GetCharTextureResourceFT(ch, fontSize, color, textureIndex)) {
            continue; // skip characters that cannot be loaded
        }

        // get character resource
        CharTextureResource& resource = _charTextureResources[textureIndex];
        
        // calculate position (consider baseline alignment)
        float charX = x + resource._left;
        float charY = y + baselineYFromTop - resource._top;
        
        // draw character
        Draw2DCharTextureFT(textureIndex, UIVector2F(charX, charY), z, 1.0f, 255, renderLevel);
        
        // update pen position
        x += resource._advance;
    }
}

void UIGraphicsSystem::Draw2DTextFT(const wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, int renderLevel) { 
    UIVector2F position = CalculateTextPosition(text, rc, alignment, fontSize);

    UIScreenClipRectGuard clipRect(rc);
    Draw2DTextFT(text, position, z, color, fontSize, renderLevel);
}

// Split text into lines by '\n'
vector<wstring> UIGraphicsSystem::SplitTextIntoLines(const wstring& text) {
    vector<wstring> lines;
    wstring currentLine;
    
    for (size_t i = 0; i < text.length(); i++) {
        wchar_t ch = text[i];

        if (ch == L'\r') {
            continue; // ignore carriage return
        } else if (ch == L'\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            continue;
        } else {
            currentLine += ch;
        }
    }
    
    // Add the last line
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    // If text was empty or only had newlines, ensure at least empty line
    if (lines.empty() && !text.empty()) {
        lines.push_back(L"");
    }
    
    return lines;
}

void UIGraphicsSystem::Draw2DTextMultiLineFT(const wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, float lineSpacing, int renderLevel) {
    if (text.empty()) {
        return;
    }

    // Split text into lines
    vector<wstring> lines = SplitTextIntoLines(text);
    
    // Get font metrics for line height
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);
    float lineHeight = ftSizeFont._ftFontHeight * lineSpacing;
    
    // Render each line
    UIVector2F currentPos = position;
    for (const auto& line : lines) {
        if (!line.empty()) {
            Draw2DTextFT(line, currentPos, z, color, fontSize, renderLevel);  // Call Level 1 core function
        }
        currentPos._y += lineHeight;
    }
}

void UIGraphicsSystem::Draw2DTextMultiLineFT(const wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, float lineSpacing, int renderLevel) {
    if (text.empty()) {
        return;
    }

    UIVector2F position = CalculateTextPosition(text, rc, alignment, fontSize, lineSpacing);
    
    UIScreenClipRectGuard clipRect(rc);
    Draw2DTextMultiLineFT(text, position, z, color, fontSize, lineSpacing, renderLevel);
}

void UIGraphicsSystem::Draw3DTextFT(const wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, int renderLevel,
					              const XMMATRIX& transformMatrix) {
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);

    float x = position._x;
    float y = position._y;

    int baselineYFromTop = ftSizeFont._ftFontAscent;
    
    // render each character
    for (wchar_t ch : text) {      
        // get or create texture
        size_t textureIndex;
        if (!GetCharTextureResourceFT(ch, fontSize, color, textureIndex)) {
            continue; // skip characters that cannot be loaded
        }

        // get character resource
        CharTextureResource& resource = _charTextureResources[textureIndex];
        
        // calculate position (consider baseline alignment)
        float charX = x + resource._left;
        float charY = y + baselineYFromTop - resource._top;
        
        // draw character
        Draw3DCharTextureFT(textureIndex, UIVector2F(charX, charY), z, 1.0f, 255, renderLevel, transformMatrix);
        
        // update pen position
        x += resource._advance;
    }
}

void UIGraphicsSystem::Draw3DTextFT(const wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, int renderLevel,
					         const XMMATRIX& transformMatrix) {
    if (text.empty()) {
        return;
    }

    UIVector2F position = CalculateTextPosition(text, rc, alignment, fontSize);

    //UIScreenClipRectGuard clipRect(rc);
    Draw3DTextFT(text, position, z, color, fontSize, renderLevel, transformMatrix);
}

void UIGraphicsSystem::Draw3DTextMultiLineFT(const wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, float lineSpacing, int renderLevel,
					  		               const XMMATRIX& transformMatrix) {
    if (text.empty()) {
        return;
    }

    // Split text into lines
    vector<wstring> lines = SplitTextIntoLines(text);
    
    // Get font metrics for line height
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);
    float lineHeight = ftSizeFont._ftFontHeight * lineSpacing;
    
    // Render each line
    UIVector2F currentPos = position;
    for (const auto& line : lines) {
        if (!line.empty()) {
            Draw3DTextFT(line, currentPos, z, color, fontSize, renderLevel, transformMatrix);  // Call Level 1 core function
        }
        currentPos._y += lineHeight;
    }
}

void UIGraphicsSystem::Draw3DTextMultiLineFT(const wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, float lineSpacing, int renderLevel,
					  		               const XMMATRIX& transformMatrix) {
    if (text.empty()) {
        return;
    }

    UIVector2F position = CalculateTextPosition(text, rc, alignment, fontSize, lineSpacing);

    //UIScreenClipRectGuard clipRect(rc);
    Draw3DTextMultiLineFT(text, position, z, color, fontSize, lineSpacing, renderLevel, transformMatrix);
}

void UIGraphicsSystem::Draw3DWorldTextFT(const std::wstring& text, const UIVector3F& worldPosition,
                                       const UIColor& color, float fontSize, int renderLevel, UICameraBase3D* pCamera) {
    float x = worldPosition._x;
    float y = worldPosition._y;
    float z = worldPosition._z;

    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);

    // Calculate baseline offset in world coordinates
    float baselineOffset = (float)ftSizeFont._ftFontAscent;
    
    // render each character
    for (wchar_t ch : text) {      
        // get or create texture
        size_t textureIndex;
        if (!GetCharTextureResourceFT(ch, fontSize, color, textureIndex)) {
            continue; // skip characters that cannot be loaded
        }

        // get character resource
        CharTextureResource& resource = _charTextureResources[textureIndex];
        
        // Use pixel-to-world conversion to calculate positions precisely
        // Character left offset
        float charX = x + CalculateWorldLengthFromPixelLength((float)resource._left, worldPosition, pCamera);
        // Character top position = baseline position - (distance from baseline to character top)
        float charY = y - CalculateWorldLengthFromPixelLength((float)(baselineOffset - resource._top), worldPosition, pCamera);
        
        // Pass fontSize as scale parameter for Draw3DWorldCharTextureFT to perform correct pixel-to-world conversion
        Draw3DWorldCharTextureFT(textureIndex, UIVector3F{charX, charY, z}, 1, 255, renderLevel, pCamera);
        
        // Update pen position (world space)
        x += CalculateWorldLengthFromPixelLength((float)resource._advance, worldPosition, pCamera);
    }
}

void UIGraphicsSystem::Draw3DWorldCharTextureFT(size_t textureIndex, UIVector3F worldPosition, float scale, UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    // Get texture resource
    CharTextureResource& resource = _charTextureResources[textureIndex];
    
    // Scale parameter is already in world units, use character dimensions directly
    float charWorldWidth = CalculateWorldLengthFromPixelLength(resource._width * scale, worldPosition, pCamera);
    float charWorldHeight = CalculateWorldLengthFromPixelLength(resource._height * scale, worldPosition, pCamera);
    
    // Create world-space vertex data - fix Y coordinate ordering to ensure correct vertical positioning
    vector<VertexPositionTexture> vertices = {
        // Top-left (UV: 0, 0)
        { {worldPosition._x, worldPosition._y, worldPosition._z}, XMFLOAT2(0.f, 0.f) },
        // Top-right (UV: 1, 0) 
        { {worldPosition._x + charWorldWidth, worldPosition._y, worldPosition._z}, XMFLOAT2(1.f, 0.f) },
        // Bottom-left (UV: 0, 1)
        { {worldPosition._x, worldPosition._y - charWorldHeight, worldPosition._z}, XMFLOAT2(0.f, 1.f) },
        // Bottom-right (UV: 1, 1)
        { {worldPosition._x + charWorldWidth, worldPosition._y - charWorldHeight, worldPosition._z}, XMFLOAT2(1.f, 1.f) }
    };
    
    // Define index data
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };

    // Use the provided camera instead of a fixed UI camera, so each control can have its own perspective
    RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleTexturedEffect3D, 
                             resource._gpuDescriptor, _graphicsDevice->p_states->LinearClamp(), alpha, GetCurrentClipRect(), pCamera, renderLevel);
}

UISIZE UIGraphicsSystem::GetTextSizeFT(const wstring& text, float fontSize, float lineSpacing) {
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);
    int lineHeight = (int)(ftSizeFont._ftFontHeight * lineSpacing);

    UISIZE size;
    size.cx = 0;
    size.cy = lineHeight;

    // render each character
    for (wchar_t ch : text) {      
        // get or create texture
        size_t textureIndex;
        if (!GetCharTextureResourceFT(ch, fontSize, UIColor::Black, textureIndex)) {
            continue; // skip characters that cannot be loaded
        }

        if (ch == L'\r') {
            continue; // ignore carriage return
        } else if (ch == L'\n') {
            // new line
            size.cy += lineHeight;
            continue;
        } else {
            // update width
            size.cx += _charTextureResources[textureIndex]._advance;
        }
    }
    
    return size;
}

/*************************************************** 2D UI APIs ***************************************************/

void UIGraphicsSystem::Draw2DPoint(const UIVector2F& point, float z, const UIColor& color, float pointSize, int renderLevel) {
    float viewZ = UICameraUI2D::GetSingletonInstance()->CalculateViewZByOrtho(z);

    UIVector2F p;
    Calculate2DPoint(point, p);

    // Define the point size (2x2 pixels)
    const float halfSize = pointSize / 2;
    
    // Create a small square with 4 vertices
    vector<VertexPositionColor> vertices = {
        {{ p._x - halfSize, p._y - halfSize, viewZ }, color.ToXMVECTORF32()},
        {{ p._x - halfSize, p._y + halfSize, viewZ }, color.ToXMVECTORF32()},
        {{ p._x + halfSize, p._y - halfSize, viewZ }, color.ToXMVECTORF32()},
        {{ p._x + halfSize, p._y + halfSize, viewZ }, color.ToXMVECTORF32()}
    };
    // Define the indexes of the two triangles
    vector<uint16_t> indices = { 
        0, 1, 2,  // 1st triangle
        1, 3, 2   // 2nd triangle
    };

    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleEffect2D, GetCurrentClipRect(), UICameraUI2D::GetSingletonInstance(), renderLevel);
}

void UIGraphicsSystem::Draw2DPoints(const vector<UIVector2F>& points, float z, const UIColor& color, float pointSize, int renderLevel) {
    // call Draw2DPoint for each point
    for (const auto& point : points) {
        Draw2DPoint(point, z, color, pointSize, renderLevel);
    }
}

void UIGraphicsSystem::Draw2DLine(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth, int renderLevel) {
    XMVECTORF32 finalColor = color.ToXMVECTORF32();

    UIVector2F p1, p2;
    Calculate2DLinePoints(start, end, p1, p2);
    // calculate line direction vector
    UIVector2F dir = { p2._x - p1._x, p2._y - p1._y };
    float length = sqrt(dir._x * dir._x + dir._y * dir._y);
    if (length < 0.0001f) {
        return;
    }
    // normalize and calculate normal vector
    dir._x /= length;
    dir._y /= length;
    UIVector2F normal = { -dir._y, dir._x };
    // calculate four vertices, using transformed viewZ
    float halfWidth = max(1.0f, lineWidth) * 0.5f;

    // Get viewZ
    float viewZ = UICameraUI2D::GetSingletonInstance()->CalculateViewZByOrtho(z);

    // Calculate four vertices
    vector<VertexPositionColor> vertices = {
        {{ p1._x + normal._x * halfWidth, p1._y + normal._y * halfWidth, viewZ }, finalColor},
        {{ p1._x - normal._x * halfWidth, p1._y - normal._y * halfWidth, viewZ }, finalColor},
        {{ p2._x + normal._x * halfWidth, p2._y + normal._y * halfWidth, viewZ }, finalColor},
        {{ p2._x - normal._x * halfWidth, p2._y - normal._y * halfWidth, viewZ }, finalColor}
    };
    // draw two triangles
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };

    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleEffect2D, GetCurrentClipRect(), UICameraUI2D::GetSingletonInstance(), renderLevel);
}

// Draw a 2D rectangle with specified color, hollow style
void UIGraphicsSystem::Draw2DRectOutline(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth, int renderLevel) {    
    Draw2DLine(UIVector2F(start._x, start._y), UIVector2F(end._x, start._y), z, color, lineWidth, renderLevel); // up
    Draw2DLine(UIVector2F(end._x, start._y), UIVector2F(end._x, end._y), z, color, lineWidth, renderLevel); //right
    Draw2DLine(UIVector2F(start._x, end._y), UIVector2F(end._x, end._y), z, color, lineWidth, renderLevel); // down
    Draw2DLine(UIVector2F(start._x, start._y), UIVector2F(start._x, end._y), z, color, lineWidth, renderLevel); // left
}

void UIGraphicsSystem::Draw2DRectSolid(const UIVector2F& start, const UIVector2F& end, float z, 
						               const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel) {
    XMVECTORF32  finalColorLT = colorLT.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorRT = colorRT.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorLB = colorLB.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorRB = colorRB.ToXMVECTORF32(alpha);
    
    // ps: real start point
    // pe: real end point
    UIVector2F ps, pe;
    Calculate2DRectPoints(start, end, ps, pe);

    float viewZ = UICameraUI2D::GetSingletonInstance()->CalculateViewZByOrtho(z);

    // Create vertices for the rectangle corners
    vector<VertexPositionColor> vertices = {
        {{ ps._x, ps._y, viewZ }, finalColorLT},         // Top-left (0)
        {{ pe._x, ps._y, viewZ }, finalColorRT},         // Top-right (1)
        {{ ps._x, pe._y, viewZ }, finalColorLB},         // Bottom-left (2)
        {{ pe._x, pe._y, viewZ }, finalColorRB}          // Bottom-right (3)
    };
    
    // Define indices for two triangles
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };

    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleEffect2D, GetCurrentClipRect(), UICameraUI2D::GetSingletonInstance(), renderLevel);
}

// Draw a 2D rectangle with specified color, alpha transparency and z-depth
void UIGraphicsSystem::Draw2DRectSolid(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, UCHAR alpha, int renderLevel) {
    Draw2DRectSolid(start, end, z, color, color, color, color, alpha, renderLevel);
}

void UIGraphicsSystem::Draw2DImage(size_t textureIndex, 
                            UIRECT srcRect, UIVector2F dstStart, UIVector2F dstEnd, 
                            float z, UCHAR alpha, int renderLevel) {
    // get texture resource
    UITextureManager::TextureResource& resource = _textureManager._textureResources[textureIndex];

    // get original texture size 
    const UIRECT textureRect = _textureManager.Get2DTextureRect(resource._texture);

    // if srcRect is NULL_RECT, use textureRect
    if (IsRectEmpty(&srcRect)) {
        srcRect = textureRect;
    }

    // if dstEnd is 0, calculate it
    if (dstEnd._x == 0 || dstEnd._y == 0) {
        dstEnd._x = dstStart._x + (float)(GetRectWidth()(srcRect)) - 1;
        dstEnd._y = dstStart._y + (float)(GetRectHeight()(srcRect)) - 1;
    }

    // calculate real rectangle points
    UIVector2F ps, pe;
    Calculate2DRectPoints(dstStart, dstEnd, ps, pe);

    // Get view space Z value using 2D transform
    float viewZ = UICameraUI2D::GetSingletonInstance()->CalculateViewZByOrtho(z);

    // Calculate texture coordinates
    float texLeft = (float)srcRect.left / (float)GetRectWidth()(textureRect);
    float texTop = (float)srcRect.top / (float)GetRectHeight()(textureRect);
    float texRight = (float)srcRect.right / (float)GetRectWidth()(textureRect);
    float texBottom = (float)srcRect.bottom / (float)GetRectHeight()(textureRect);

    // create vertex data
    vector<VertexPositionTexture> vertices = {
        { {ps._x, ps._y, viewZ}, XMFLOAT2(texLeft, texTop) },      // Top-left
        { {pe._x, ps._y, viewZ}, XMFLOAT2(texRight, texTop) },     // Top-right
        { {ps._x, pe._y, viewZ}, XMFLOAT2(texLeft, texBottom) },   // Bottom-left
        { {pe._x, pe._y, viewZ}, XMFLOAT2(texRight, texBottom) }   // Bottom-right
    };

    // define index data
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };

    RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleTexturedEffect2D, 
                             resource._gpuDescriptor, _graphicsDevice->p_states->LinearClamp(), alpha, GetCurrentClipRect(), UICameraUI2D::GetSingletonInstance(), renderLevel);
}

void UIGraphicsSystem::Draw2DImage(const wstring& filePath, const UIColor& colorKey,
                            const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd,
                            float z, UCHAR alpha, int renderLevel) {
    size_t textureIndex = 0;
    // check is dds file
    if (filePath.find(L".dds") != wstring::npos) {
        if (!_textureManager.GetDDSTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    } else {
        if (!_textureManager.GetWICTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    }
    Draw2DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha, renderLevel);
}

void UIGraphicsSystem::Draw2DImage(const wstring& dllPath, UINT id, const UIColor& colorKey,
                                   const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd,
                                   float z, UCHAR alpha, int renderLevel) {
    size_t textureIndex = 0; 
    if (!_textureManager.GetWICTextureIndexFromDLL(dllPath, id, colorKey, textureIndex)) {
        return;
    }
    Draw2DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha, renderLevel);
}

void UIGraphicsSystem::Draw3DPoint(const UIVector2F& point, float z, const UIColor& color, float pointSize, int renderLevel, const XMMATRIX& transformMatrix) {
    XMVECTORF32 finalColor = color.ToXMVECTORF32();

    UIVector2F p;
    Calculate2DPoint(point, p);

    UIVector3F wp;
    UIZPlaneTransform::TransformPoint(transformMatrix, p, z, wp);

    Draw3DWorldCircle(wp, pointSize / 2, finalColor, 255, renderLevel, UICameraUI3D::GetSingletonInstance());
}

void UIGraphicsSystem::Draw3DPoints(const vector<UIVector2F>& points, float z, const UIColor& color, float pointSize, int renderLevel,
                                  const XMMATRIX& transformMatrix) {
    // call Draw3DPoint for each point
    for (const auto& point : points) {
        Draw3DPoint(point, z, color, pointSize, renderLevel, transformMatrix);
    }
}

void UIGraphicsSystem::Draw3DLine(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth, int renderLevel, const XMMATRIX& transformMatrix) { 
    UIVector2F p1, p2;
    Calculate2DLinePoints(start, end, p1, p2);

    vector<UIVector3F> wps;
    UIZPlaneTransform::TransformLinePoints(transformMatrix, p1, p2, z, wps);

    Draw3DWorldLine(wps[0], wps[1], color, lineWidth, renderLevel, UICameraUI3D::GetSingletonInstance());
}

void UIGraphicsSystem::Draw3DRectOutline(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth, int renderLevel, const XMMATRIX& transformMatrix) {    
    UIVector2F ps, pe;
    Calculate2DRectPoints(start, end, ps, pe);
    vector<UIVector3F> wps;
    UIZPlaneTransform::TransformRectPoints(transformMatrix, ps, pe, z, wps);
    
    Draw3DWorldRectOutline(wps[0], wps[1], wps[2], wps[3], color, lineWidth, renderLevel, UICameraUI3D::GetSingletonInstance());
}

// Draw a 3D rectangle with specified color, alpha transparency and z-depth
void UIGraphicsSystem::Draw3DRectSolid(const UIVector2F& start, const UIVector2F& end, float z,
                                     const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel,
                                     const XMMATRIX& transformMatrix) {
    UIVector2F ps, pe;
    Calculate2DRectPoints(start, end, ps, pe);
    vector<UIVector3F> wps;
    UIZPlaneTransform::TransformRectPoints(transformMatrix, ps, pe, z, wps);
    
    Draw3DWorldRectSolid(wps[0], wps[1], wps[2], wps[3], colorLT, colorRT, colorLB, colorRB, alpha, renderLevel, UICameraUI3D::GetSingletonInstance());
}

void UIGraphicsSystem::Draw3DRectSolid(const UIVector2F& start, const UIVector2F& end, float z,
                                     const UIColor& color, UCHAR alpha, int renderLevel,
                                     const XMMATRIX& transformMatrix) {
    Draw3DRectSolid(start, end, z, color, color, color, color, alpha, renderLevel, transformMatrix);
}

void UIGraphicsSystem::Draw3DImage(size_t textureIndex, 
                                 UIRECT srcRect, UIVector2F dstStart, UIVector2F dstEnd, 
                                 float z, UCHAR alpha, int renderLevel,
                                 const XMMATRIX& transformMatrix) {
    // get the texture rect
    const UIRECT textureRect = _textureManager.Get2DTextureRect(_textureManager._textureResources[textureIndex]._texture);

    // if srcRect is NULL_RECT, use textureRect
    if (IsRectEmpty(&srcRect)) {
        srcRect = textureRect;
    }

    // if dstEnd is 0, calculate it
    if (dstEnd._x == 0 || dstEnd._y == 0) {
        dstEnd._x = dstStart._x + (float)(GetRectWidth()(srcRect));
        dstEnd._y = dstStart._y + (float)(GetRectHeight()(srcRect));
    }

    UIVector2F ps, pe;
    Calculate2DRectPoints(dstStart, dstEnd, ps, pe);
    vector<UIVector3F> wps;
    UIZPlaneTransform::TransformRectPoints(transformMatrix, ps, pe, z, wps);

    Draw3DWorldImage(textureIndex, srcRect, wps[0], wps[1], wps[2], wps[3], alpha, renderLevel, UICameraUI3D::GetSingletonInstance());
}

void UIGraphicsSystem::Draw3DImage(const wstring& dllPath, UINT id, const UIColor& colorKey,
				 const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd, 
				 float z, UCHAR alpha, int renderLevel,
				 const XMMATRIX& transformMatrix) {
    size_t textureIndex = 0; 
    if (!_textureManager.GetWICTextureIndexFromDLL(dllPath, id, colorKey, textureIndex)) {
        return;
    }

    Draw3DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha, renderLevel, transformMatrix);
}void UIGraphicsSystem::Draw3DImage(const wstring& filePath, const UIColor& colorKey,
				 const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd, 
				 float z, UCHAR alpha, int renderLevel,
				 const XMMATRIX& transformMatrix) {
    size_t textureIndex = 0;
    // check is dds file
    if (filePath.find(L".dds") != wstring::npos) {
        if (!_textureManager.GetDDSTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    } else {
        if (!_textureManager.GetWICTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    }

    Draw3DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha, renderLevel, transformMatrix);
}void UIGraphicsSystem::Draw3DWorldPoint(const UIVector3F& point, const UIColor& color, int renderLevel, UICameraBase3D* pCamera) {
    // Create point vertex
    vector<VertexPositionColor> vertices = {
        {{point._x, point._y, point._z}, color.ToXMVECTORF32()}
    };
    
    vector<uint16_t> indices = {0};
    
    // Register batch data instead of direct rendering
    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, vertices, indices, _graphicsDevice->_effectManager.p_pointEffect3D, GetCurrentClipRect(), pCamera, renderLevel);
}

void UIGraphicsSystem::Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& colorS, const UIColor& colorE, float lineWidth, int renderLevel, UICameraBase3D* pCamera) {
    if (lineWidth <= 1.0f) {
        Draw3DWorldLine(start, end, colorS, colorE, renderLevel, pCamera);
    } else {
        UIColor avgColor(
			(colorS._r + colorE._r) / 2,
			(colorS._g + colorE._g) / 2,
			(colorS._b + colorE._b) / 2,
			(colorS._a + colorE._a) / 2
		);
        Draw3DWorldThickLine(start, end, lineWidth, avgColor, renderLevel, pCamera);
    }
}

void UIGraphicsSystem::Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& color, float lineWidth, int renderLevel, UICameraBase3D* pCamera) {
    if (lineWidth <= 1.0f) {
        Draw3DWorldLine(start, end, color, renderLevel, pCamera);
    } else {
        Draw3DWorldThickLine(start, end, lineWidth, color, renderLevel, pCamera);
    }
}

void UIGraphicsSystem::Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& colorS, const UIColor& colorE, int renderLevel, UICameraBase3D* pCamera) {
    // Create line vertices with gradient colors
    vector<VertexPositionColor> vertices = {
        {{start._x, start._y, start._z}, colorS.ToXMVECTORF32()},  // Start vertex with start color
        {{end._x, end._y, end._z}, colorE.ToXMVECTORF32()}        // End vertex with end color
    };
    
    vector<uint16_t> indices = {0, 1};
    
    // Register batch data instead of direct rendering
    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_LINELIST, vertices, indices, _graphicsDevice->_effectManager.p_lineEffect3D, GetCurrentClipRect(), pCamera, renderLevel);
}
	
void UIGraphicsSystem::Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& color, int renderLevel, UICameraBase3D* pCamera) {
    Draw3DWorldLine(start, end, color, color, renderLevel, pCamera);
}

void UIGraphicsSystem::Draw3DWorldThickLine(const UIVector3F& start, const UIVector3F& end, float lineWidth, const UIColor& color, int renderLevel, UICameraBase3D* pCamera) {
    const int segments = 8; // Number of cylinder segments
    
    XMFLOAT3 startXM = start.ToXMFLOAT3();
    XMFLOAT3 endXM = end.ToXMFLOAT3();
    XMVECTOR startVec = XMLoadFloat3(&startXM);
    XMVECTOR endVec = XMLoadFloat3(&endXM);
    XMVECTOR direction = XMVector3Normalize(XMVectorSubtract(endVec, startVec));
    
    // Find two vectors perpendicular to the segment
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (abs(XMVectorGetY(direction)) > 0.9f) {
    up = XMVectorSet(1, 0, 0, 0); // If the segment is nearly vertical, use the X axis
    }
    
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(direction, up));
    up = XMVector3Cross(right, direction);
    
    // Convert pixel line width to world-space line width
    float worldRadius = CalculateWorldLengthFromPixelLength(lineWidth * 0.5f, start, pCamera);
    
    vector<VertexPositionColor> vertices;
    vector<uint16_t> indices;
    
    XMFLOAT4 uniformColor4;
    XMStoreFloat4(&uniformColor4, color.ToXMVECTORF32());
    
    // Generate cylinder vertices
    for (int i = 0; i < segments; ++i) {
        float angle = (float)i * XM_2PI / segments;
        float cosAngle = cosf(angle);
        float sinAngle = sinf(angle);
        
        XMVECTOR offset = XMVectorAdd(
            XMVectorScale(right, worldRadius * cosAngle),
            XMVectorScale(up, worldRadius * sinAngle)
        );
        
    // Start circle vertices
        XMFLOAT3 startVertex;
        XMStoreFloat3(&startVertex, XMVectorAdd(startVec, offset));
        vertices.emplace_back(startVertex, uniformColor4);
        
    // End circle vertices
        XMFLOAT3 endVertex;
        XMStoreFloat3(&endVertex, XMVectorAdd(endVec, offset));
        vertices.emplace_back(endVertex, uniformColor4);
    }
    
    // Generate triangle indices for the cylinder sides
    for (int i = 0; i < segments; ++i) {
        int current = i * 2;
        int next = ((i + 1) % segments) * 2;
        
        // First triangle
        indices.push_back(static_cast<uint16_t>(current));     // Current start
        indices.push_back(static_cast<uint16_t>(current + 1)); // Current end
        indices.push_back(static_cast<uint16_t>(next));        // Next start
            
        // Second triangle
        indices.push_back(static_cast<uint16_t>(current + 1)); // Current end
        indices.push_back(static_cast<uint16_t>(next + 1));    // Next end
        indices.push_back(static_cast<uint16_t>(next));        // Next start
    }
    
    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, 
                      _graphicsDevice->_effectManager.p_triangleEffect3D, GetCurrentClipRect(), pCamera, renderLevel);
}

void UIGraphicsSystem::Draw3DWorldCircle(const UIVector3F& center, float pixelRadius, const UIColor& color, UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    // Calculate camera-facing billboard
    Matrix view = pCamera->GetViewMatrix();
    XMVECTOR right = XMVectorSet(view.m[0][0], view.m[1][0], view.m[2][0], 0);
    XMVECTOR up = XMVectorSet(view.m[0][1], view.m[1][1], view.m[2][1], 0);
    
    // Convert pixel radius to world radius at given depth
    float worldRadius = CalculateWorldLengthFromPixelLength(pixelRadius, center, pCamera);
    XMFLOAT3 centerXM = center.ToXMFLOAT3();
    XMVECTOR centerVec = XMLoadFloat3(&centerXM);
    
    // Create very smooth circular geometry using even more triangles
    const int segments = 64; // Double the segments for ultra-smooth circle
    vector<VertexPositionColor> vertices;
    vector<uint16_t> indices;
    
    // Use uniform color for all vertices (no gradient)
    XMFLOAT4 uniformColor4;
    XMStoreFloat4(&uniformColor4, color.ToXMVECTORF32(alpha));
    
    // Add center vertex
    XMFLOAT3 centerPos;
    XMStoreFloat3(&centerPos, centerVec);
    vertices.emplace_back(centerPos, uniformColor4);
    
    // Add circle vertices with uniform color
    for (int i = 0; i < segments; ++i) {
        float angle = (float)i * XM_2PI / segments;
        float cosAngle = cosf(angle);
        float sinAngle = sinf(angle);
        
        XMVECTOR pos = XMVectorAdd(centerVec, 
                                   XMVectorAdd(XMVectorScale(right, worldRadius * cosAngle),
                                               XMVectorScale(up, worldRadius * sinAngle)));
        
        XMFLOAT3 vertexPos;
        XMStoreFloat3(&vertexPos, pos);
        vertices.emplace_back(vertexPos, uniformColor4);
    }
    
    // Create triangle indices (center + circle edge)
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0);                                    // Center vertex
        indices.push_back(static_cast<uint16_t>(i + 1));         // Current edge vertex
        indices.push_back(static_cast<uint16_t>(((i + 1) % segments) + 1)); // Next edge vertex
    }
    
    // Register batch data for triangle color rendering
    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices,
                      _graphicsDevice->_effectManager.p_triangleEffect3D, GetCurrentClipRect(), pCamera, renderLevel);
}

void UIGraphicsSystem::Draw3DWorldTriangle(const UIVector3F& p1, const UIVector3F& p2, const UIVector3F& p3, const UIColor& color, UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    vector<VertexPositionColor> vertices;
    vector<uint16_t> indices;
    
    // Convert color to XMFLOAT4
    XMFLOAT4 color4;
    XMStoreFloat4(&color4, color.ToXMVECTORF32(alpha));
    
    // Add triangle vertices with uniform color
    vertices.emplace_back(p1.ToXMFLOAT3(), color4);
    vertices.emplace_back(p2.ToXMFLOAT3(), color4);
    vertices.emplace_back(p3.ToXMFLOAT3(), color4);
    
    // Triangle indices (counter-clockwise for proper front-facing)
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    
    // Register batch data for triangle rendering
    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices,
                      _graphicsDevice->_effectManager.p_triangleEffect3D, GetCurrentClipRect(), pCamera, renderLevel);
}

void UIGraphicsSystem::Draw3DWorldTriangle(const UIVector3F& p1, const UIVector3F& p2, const UIVector3F& p3, 
                                           const UIColor& color1, const UIColor& color2, const UIColor& color3, UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    vector<VertexPositionColor> vertices;
    vector<uint16_t> indices;
    
    // Convert colors to XMFLOAT4
    XMFLOAT4 color4_1, color4_2, color4_3;
    XMStoreFloat4(&color4_1, color1.ToXMVECTORF32(alpha));
    XMStoreFloat4(&color4_2, color2.ToXMVECTORF32(alpha));
    XMStoreFloat4(&color4_3, color3.ToXMVECTORF32(alpha));
    
    // Add triangle vertices with individual colors for gradient effect
    vertices.emplace_back(p1.ToXMFLOAT3(), color4_1);
    vertices.emplace_back(p2.ToXMFLOAT3(), color4_2);
    vertices.emplace_back(p3.ToXMFLOAT3(), color4_3);
    
    // Triangle indices (counter-clockwise for proper front-facing)
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    
    // Register batch data for triangle rendering
    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices,
                      _graphicsDevice->_effectManager.p_triangleEffect3D, GetCurrentClipRect(), pCamera, renderLevel);
}

void UIGraphicsSystem::Draw3DWorldTextureTriangle(const UIVector3F& p1, const UIVector3F& p2, const UIVector3F& p3,
                                                  const XMFLOAT2& uv1, const XMFLOAT2& uv2, const XMFLOAT2& uv3,
                                                  size_t textureIndex, UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    // Get texture resource
    UITextureManager::TextureResource& resource = _textureManager._textureResources[textureIndex];
    
    // Create vertices with texture coordinates
    vector<VertexPositionTexture> vertices = {
        { {p1._x, p1._y, p1._z}, uv1 },
        { {p2._x, p2._y, p2._z}, uv2 },
        { {p3._x, p3._y, p3._z}, uv3 }
    };
    
    // Triangle indices
    vector<uint16_t> indices = { 0, 1, 2 };
    
    // Register batch data for rendering
    RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleTexturedEffect3D, 
                             resource._gpuDescriptor, _graphicsDevice->p_states->LinearClamp(), alpha, GetCurrentClipRect(), pCamera, renderLevel);
}

void UIGraphicsSystem::Draw3DWorldRectOutline(const UIVector3F& pLT, const UIVector3F& pRT, const UIVector3F& pLB, const UIVector3F& pRB,
                                            const UIColor& color, float lineWidth, int renderLevel, UICameraBase3D* pCamera) {
    Draw3DWorldLine(pLT, pRT, color, lineWidth, renderLevel, pCamera);  // Top edge
    Draw3DWorldLine(pRT, pRB, color, lineWidth, renderLevel, pCamera);  // Right edge
    Draw3DWorldLine(pRB, pLB, color, lineWidth, renderLevel, pCamera);  // Bottom edge
    Draw3DWorldLine(pLB, pLT, color, lineWidth, renderLevel, pCamera);  // Left edge
}

void UIGraphicsSystem::Draw3DWorldRectSolid(const UIVector3F& pLT, const UIVector3F& pRT, const UIVector3F& pLB, const UIVector3F& pRB,
                                          const UIColor& color, UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    Draw3DWorldRectSolid(pLT, pRT, pLB, pRB, color, color, color, color, alpha, renderLevel, pCamera);
}

void UIGraphicsSystem::Draw3DWorldRectSolid(const UIVector3F& pLT, const UIVector3F& pRT, const UIVector3F& pLB, const UIVector3F& pRB,
                                          const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB,
                                          UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    // Convert colors to XMFLOAT4
    XMFLOAT4 color4_LT, color4_RT, color4_LB, color4_RB;
    XMStoreFloat4(&color4_LT, colorLT.ToXMVECTORF32(alpha));
    XMStoreFloat4(&color4_RT, colorRT.ToXMVECTORF32(alpha));
    XMStoreFloat4(&color4_LB, colorLB.ToXMVECTORF32(alpha));
    XMStoreFloat4(&color4_RB, colorRB.ToXMVECTORF32(alpha));
    
    // Create vertices (pLT=LT, pRT=RT, pLB=LB, pRB=RB)
    vector<VertexPositionColor> vertices = {
        { {pLT._x, pLT._y, pLT._z}, color4_LT },  // Left-top
        { {pRT._x, pRT._y, pRT._z}, color4_RT },  // Right-top
        { {pLB._x, pLB._y, pLB._z}, color4_LB },  // Left-bottom
        { {pRB._x, pRB._y, pRB._z}, color4_RB }   // Right-bottom
    };
    
    // Define indices for two triangles (0=LT, 1=RT, 2=LB, 3=RB)
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };
    
    // Register batch data
    RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, 
                      _graphicsDevice->_effectManager.p_triangleEffect3D, GetCurrentClipRect(), pCamera, renderLevel);
}

void UIGraphicsSystem::Draw3DWorldImage(size_t textureIndex, const UIRECT& srcRect, const UIVector3F& pLT, const UIVector3F& pRT, 
					              const UIVector3F& pLB, const UIVector3F& pRB, UCHAR alpha, int renderLevel, UICameraBase3D* pCamera) {
    // Get texture resource
    UITextureManager::TextureResource& resource = _textureManager._textureResources[textureIndex];
    
    // Get original texture size 
    const UIRECT textureRect = _textureManager.Get2DTextureRect(resource._texture);
    
    // Use provided srcRect or full texture
    UIRECT actualSrcRect = srcRect;
    if (IsRectEmpty(&srcRect)) {
        actualSrcRect = textureRect;
    }
    
    // Calculate texture coordinates
    float texLeft = (float)actualSrcRect.left / (float)GetRectWidth()(textureRect);
    float texTop = (float)actualSrcRect.top / (float)GetRectHeight()(textureRect);
    float texRight = (float)actualSrcRect.right / (float)GetRectWidth()(textureRect);
    float texBottom = (float)actualSrcRect.bottom / (float)GetRectHeight()(textureRect);
    
    // Create vertices with texture coordinates (pLT=LT, pRT=RT, pLB=LB, pRB=RB)
    vector<VertexPositionTexture> vertices = {
        { {pLT._x, pLT._y, pLT._z}, XMFLOAT2(texLeft, texTop) },      // Left-top
        { {pRT._x, pRT._y, pRT._z}, XMFLOAT2(texRight, texTop) },     // Right-top
        { {pLB._x, pLB._y, pLB._z}, XMFLOAT2(texLeft, texBottom) },   // Left-bottom
        { {pRB._x, pRB._y, pRB._z}, XMFLOAT2(texRight, texBottom) }   // Right-bottom
    };    // Define indices for two triangles
    vector<uint16_t> indices = { 0, 1, 2, 1, 3, 2 };

    // Register batch texture data
    RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, indices, _graphicsDevice->_effectManager.p_triangleTexturedEffect3D,
                             resource._gpuDescriptor, _graphicsDevice->p_states->LinearClamp(), alpha, GetCurrentClipRect(), pCamera, renderLevel);
}

// Helper function to calculate world length from pixel length
float UIGraphicsSystem::CalculateWorldLengthFromPixelLength(float pixelLength, const UIVector3F& worldPosition, UICameraBase3D* pCamera) {
    // Get camera matrices
    XMMATRIX view = pCamera->GetViewMatrix();
    XMMATRIX proj = pCamera->GetProjectionMatrix();

    // Get viewport information
    D3D12_VIEWPORT viewport = pCamera->GetViewport();

    // Extract projection matrix elements using DirectX math functions
    XMFLOAT4X4 projMatrix;
    XMStoreFloat4x4(&projMatrix, proj);

    if (abs(projMatrix._34) > 0.0001f) {
        // Transform world position to view space
        XMFLOAT3 worldPosXM = worldPosition.ToXMFLOAT3();
        XMVECTOR worldPosVec = XMLoadFloat3(&worldPosXM);
        XMVECTOR viewPosVec = XMVector3Transform(worldPosVec, view);
        float viewZ = XMVectorGetZ(viewPosVec);
        
        // Prevent distance too close causing anomalies
        if (abs(viewZ) < 0.1f) {
            return pixelLength * 0.01f; // fallback value
        }

        // perspective projection
        float tanHalfFovY = 1.0f / projMatrix._22;
        float worldUnitsPerPixel = (2.0f * abs(viewZ) * tanHalfFovY) / viewport.Height;
        return pixelLength * worldUnitsPerPixel;
    } else {
        // orthographic projection
        return pixelLength / viewport.Width * dynamic_cast<UICameraCtrl*>(pCamera)->GetWorldWidth();
    }
}

// These are the resources that depend on the device.
void UIGraphicsDeviceDX12::CreateDeviceDependentResourcesXTK() {
    auto device = GetD3DDevice();

    p_graphicsMemory = make_unique<GraphicsMemory>(device);
    p_states = make_unique<CommonStates>(device);
    p_resourceDescriptors = make_unique<DescriptorHeap>(device, Descriptors::TotalDescriptorCount);
    p_batch = make_unique<PrimitiveBatch<VertexPositionColor>>(device);
    p_batchTexture = make_unique<PrimitiveBatch<VertexPositionTexture>>(device);

    // Create all effects through UIEffectManager
    _effectManager.Create(device, GetBackBufferFormat(), GetDepthBufferFormat());
}

// Allocate all memory resources that change on a window SizeChanged event.
void UIGraphicsDeviceDX12::CreateWindowSizeDependentResourcesXTK() {
    auto size = _outputSize;

    UICameraUI2D::GetSingletonInstance()->SetUpCamera({0, 0, size.right, size.bottom});
    UICameraUI3D::GetSingletonInstance()->SetUpCamera({0, 0, size.right, size.bottom});
    UICameraGame::GetSingletonInstance()->SetUpCamera({0, 0, size.right, size.bottom});
    
    // set the matrices for 2D drawing
    _effectManager.p_pointEffect2D->SetWorld(Matrix::Identity);
    _effectManager.p_pointEffect2D->SetView(UICameraUI2D::GetSingletonInstance()->GetViewMatrix());
    _effectManager.p_pointEffect2D->SetProjection(UICameraUI2D::GetSingletonInstance()->GetProjectionMatrix());

    _effectManager.p_lineEffect2D->SetWorld(Matrix::Identity);
    _effectManager.p_lineEffect2D->SetView(UICameraUI2D::GetSingletonInstance()->GetViewMatrix());
    _effectManager.p_lineEffect2D->SetProjection(UICameraUI2D::GetSingletonInstance()->GetProjectionMatrix());

    _effectManager.p_triangleEffect2D->SetWorld(Matrix::Identity);
    _effectManager.p_triangleEffect2D->SetView(UICameraUI2D::GetSingletonInstance()->GetViewMatrix());
    _effectManager.p_triangleEffect2D->SetProjection(UICameraUI2D::GetSingletonInstance()->GetProjectionMatrix());

    _effectManager.p_triangleTexturedEffect2D->SetWorld(Matrix::Identity);
    _effectManager.p_triangleTexturedEffect2D->SetView(UICameraUI2D::GetSingletonInstance()->GetViewMatrix());
    _effectManager.p_triangleTexturedEffect2D->SetProjection(UICameraUI2D::GetSingletonInstance()->GetProjectionMatrix());
}

void UIGraphicsDeviceDX12::ReleaseResources() {    
    // Release DirectX 12 core resources
    for (UINT n = 0; n < MAX_BACK_BUFFER_COUNT; n++) {
        _renderTargets[n].Reset();
        _commandAllocators[n].Reset();
        _fenceValues[n] = 0;
    }
    
    _depthStencil.Reset();
    _rtvDescriptorHeap.Reset();
    _dsvDescriptorHeap.Reset();
    _fence.Reset();
    
    _commandQueue.Reset();
    _commandList.Reset();
    _swapChain.Reset();
    _d3dDevice.Reset();
    _dxgiFactory.Reset();
}

void UIGraphicsDeviceDX12::ReleaseResourcesXTK() {
    //p_font.reset();
    p_batch.reset();
    p_batchTexture.reset();
    
    // Reset all effects through UIEffectManager
    _effectManager.Reset();
    
    //p_sprites.reset();
    p_resourceDescriptors.reset();
    p_states.reset();
    p_graphicsMemory.reset();
}

/*************************************************** Batch Rendering System Implementation ***************************************************/
// Batch registration functions
void UIGraphicsSystem::RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY topology,
                                       const vector<VertexPositionColor>& vertices, const vector<uint16_t>& indices, 
                                       unique_ptr<BasicEffect>& effect, const UIRECT& clipRect, UICameraBase* pCamera, int renderLevel) {
    // Create new batch data with key information
    BatchData newBatch;
    newBatch._batchID = 1;  // _graphicsDevice->p_batch
    newBatch._renderLevel = renderLevel;
    newBatch._pEffect = effect.get();
    newBatch._topology = topology;
    newBatch._clipRect = clipRect;
    newBatch._pCamera = pCamera;

    // search the same key
    for (auto& batch : _batchDataList) {
        if (newBatch.IsSameKey(batch)) {
            batch._indices.push_back(indices);
            batch._colorVertices.push_back(vertices);
            return;
        }
    }

    // no matching key found
    newBatch._indices.push_back(indices);
    newBatch._colorVertices.push_back(vertices);
    _batchDataList.push_back(move(newBatch));
}

void UIGraphicsSystem::RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY topology,
                                                const vector<VertexPositionTexture>& vertices, const vector<uint16_t>& indices,
                                                unique_ptr<BasicEffect>& effect, 
                                                D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor, UCHAR alpha,
                                                const UIRECT& clipRect, UICameraBase* pCamera, int renderLevel) {
    // Create new batch data with key information
    BatchData newBatch;
    newBatch._batchID = 2;  // _graphicsDevice->p_batchTexture
    newBatch._renderLevel = renderLevel;
    newBatch._pEffect = effect.get();
    newBatch._topology = topology;
    newBatch._srvDescriptor = srvDescriptor;
    newBatch._samplerDescriptor = samplerDescriptor;
    newBatch._alpha = alpha;
    newBatch._clipRect = clipRect;
    newBatch._pCamera = pCamera;
    
    // search the same key
    for (auto& batch : _batchDataList) {
        if (newBatch.IsSameKey(batch)) {
            batch._indices.push_back(indices);
            batch._textureVertices.push_back(vertices);
            return;
        }
    }

    // no matching key found, insert new element
    newBatch._indices.push_back(indices);
    newBatch._textureVertices.push_back(vertices);
    _batchDataList.push_back(move(newBatch));
}

// Batch execution functions
void UIGraphicsSystem::ExecuteAllBatches() {
    if (_batchDataList.empty()) {
        return;
    }

    // Sort batches by level (ascending order: smaller level draws first)
    std::sort(_batchDataList.begin(), _batchDataList.end(), 
        [](const BatchData& a, const BatchData& b) {
            return a._renderLevel < b._renderLevel;
        });

    auto commandList = _graphicsDevice->GetCommandList();
    
    // Execute each batch in level order
    for (const auto& batch : _batchDataList) {
        // Set clipping rectangle for this batch
        ExecuteClipRect(batch._clipRect);
        
        if (batch._batchID == 1) {
            // Execute color batch (_graphicsDevice->p_batch)
            ExecuteColorBatch(batch, commandList);
        } else if (batch._batchID == 2) {
            // Execute texture batch (_graphicsDevice->p_batchTexture)
            ExecuteTextureBatch(batch, commandList);
        }
    }
}

void UIGraphicsSystem::UpdateBatchState(const BatchData& batch, ID3D12GraphicsCommandList* commandList) {
    bool cameraChanged = (_pCurrentCamera != batch._pCamera);
    bool effectChanged = (_pCurrentEffect != batch._pEffect);

    if (cameraChanged || effectChanged) {
        if (batch._pCamera != UICameraUI2D::GetSingletonInstance()) {
            batch._pEffect->SetWorld(Matrix::Identity);
            batch._pEffect->SetView(batch._pCamera->GetViewMatrix());
            batch._pEffect->SetProjection(batch._pCamera->GetProjectionMatrix());
        }

        _pCurrentEffect = batch._pEffect;
    }
    
    if (cameraChanged) {
        if (_pCurrentCamera == nullptr || (&_pCurrentCamera->GetViewport(), &batch._pCamera->GetViewport(), sizeof(D3D12_VIEWPORT)) != 0) {
            commandList->RSSetViewports(1, &batch._pCamera->GetViewport());
        }

        _pCurrentCamera = batch._pCamera;
    }
}

void UIGraphicsSystem::ExecuteColorBatch(const BatchData& batch, ID3D12GraphicsCommandList* commandList) {
    if (batch._colorVertices.empty() || batch._indices.empty()) {
        return;
    }

    UpdateBatchState(batch, commandList);

    // Apply effect for this batch
    batch._pEffect->Apply(commandList);
    
    // Render each draw call in this batch
    _graphicsDevice->p_batch->Begin(commandList);
    for (size_t i = 0; i < batch._indices.size(); ++i) {
        const auto& indices = batch._indices[i];
        const auto& vertices = batch._colorVertices[i];
        
        _graphicsDevice->p_batch->DrawIndexed(batch._topology,
                                              indices.data(), indices.size(),
                                              vertices.data(), vertices.size());
    }
    _graphicsDevice->p_batch->End();
}

void UIGraphicsSystem::ExecuteTextureBatch(const BatchData& batch, ID3D12GraphicsCommandList* commandList) {
    if (batch._textureVertices.empty() || batch._indices.empty()) {
        return;
    }

    UpdateBatchState(batch, commandList);
    
    // Set texture and alpha for this batch
    XMVECTORF32 colorAlpha = { 1.0f, 1.0f, 1.0f, batch._alpha / 255.0f };
    batch._pEffect->SetTexture(batch._srvDescriptor, batch._samplerDescriptor);
    batch._pEffect->SetColorAndAlpha(colorAlpha);
    batch._pEffect->Apply(commandList);
    
    // Render each draw call in this batch
    _graphicsDevice->p_batchTexture->Begin(commandList);
    for (size_t i = 0; i < batch._indices.size(); ++i) {
        const auto& indices = batch._indices[i];
        const auto& vertices = batch._textureVertices[i];
        
        _graphicsDevice->p_batchTexture->DrawIndexed(batch._topology,
                                                     indices.data(), indices.size(),
                                                     vertices.data(), vertices.size());
    }
    _graphicsDevice->p_batchTexture->End();
}

// Batch clear functions
void UIGraphicsSystem::ClearAllBatches() {
    for (auto& batch : _batchDataList) {
        batch._indices.clear();
        batch._colorVertices.clear();
        batch._textureVertices.clear();
    }
    _batchDataList.clear();
}


bool UIGraphicsDeviceDX12::Initialize(const Desc& desc) {
    if (_backBufferCount < 2 || _backBufferCount > MAX_BACK_BUFFER_COUNT) {
        return false;
    }
    if (_d3dMinFeatureLevel < D3D_FEATURE_LEVEL_11_0) {
        return false;
    }

    SetWindow(desc.width, desc.height);

    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
    CreateDeviceDependentResourcesXTK();
    CreateWindowSizeDependentResourcesXTK();

    return true;
}

void UIGraphicsDeviceDX12::Shutdown() {
    WaitForIdle();
    ReleaseResources();
    ReleaseResourcesXTK();
}

void UIGraphicsDeviceDX12::BeginFrame() {
    PrepareCommandList();
    ClearRenderTargetViews();
    
    // Set the descriptor heaps
    auto commandList = GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { p_resourceDescriptors->Heap(), p_states->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
}

void UIGraphicsDeviceDX12::EndFrame() {
}

void UIGraphicsDeviceDX12::Present() {
    // Show the new frame.
    ExecutePresent(D3D12_RESOURCE_STATE_RENDER_TARGET);
    p_graphicsMemory->Commit(GetCommandQueue());
}

bool UIGraphicsDeviceDX12::HandleWindowResize(int width, int height) {
    UIRECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = width;
    newRc.bottom = height;
    if (newRc.left == _outputSize.left
        && newRc.top == _outputSize.top
        && newRc.right == _outputSize.right
        && newRc.bottom == _outputSize.bottom) {
        // Handle color space settings for HDR
        UpdateColorSpace();

        return false;
    }

    _outputSize = newRc;
    CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResourcesXTK();

    return true;
}

void UIGraphicsDeviceDX12::WaitForIdle() {
    if (_commandQueue && _fence && _fenceEvent.IsValid()) {
        // Find the maximum fence value among all pending frames
        UINT64 maxFenceValue = 0;
        for (UINT n = 0; n < _backBufferCount; n++) {
            maxFenceValue = max(maxFenceValue, _fenceValues[n]);
        }
        
        // Signal the command queue with a new fence value
        if (SUCCEEDED(_commandQueue->Signal(_fence.Get(), maxFenceValue))) {
            // Wait for this signal to complete (ensures all previous work is done)
            if (SUCCEEDED(_fence->SetEventOnCompletion(maxFenceValue, _fenceEvent.Get()))) {
                WaitForSingleObjectEx(_fenceEvent.Get(), INFINITE, FALSE);
            }
        }
    }
}

// Recreate all device resources and set them back to the current state.
void UIGraphicsDeviceDX12::HandleDeviceLost() {
    ReleaseResources();
    ReleaseResourcesXTK();

#ifdef _DEBUG 
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
    CreateDeviceDependentResourcesXTK();
    CreateWindowSizeDependentResourcesXTK();
}








