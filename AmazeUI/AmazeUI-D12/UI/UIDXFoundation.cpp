#include "UIDXFoundation.h"
#include "UIAnimation.h"
#include "UIElement.h"

using namespace std;

using namespace DirectX;
using namespace SimpleMath;
using namespace Shape2D;

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
};

// Constructor for UIDeviceResources.
UIDeviceResources::UIDeviceResources(
    DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    UINT backBufferCount,
    D3D_FEATURE_LEVEL minFeatureLevel,
    unsigned int flags) noexcept(false) :
        _backBufferIndex(0),
        _fenceValues{},
        _rtvDescriptorSize(0),
        _screenViewport{},
        _scissorRect{},
        _backBufferFormat(backBufferFormat),
        _depthBufferFormat(depthBufferFormat),
        _backBufferCount(backBufferCount),
        _d3dMinFeatureLevel(minFeatureLevel),
        _d3dFeatureLevel(D3D_FEATURE_LEVEL_11_0),
        _dxgiFactoryFlags(0),
        _outputSize{0, 0, 1, 1},
        _colorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709),
        _options(flags) {
    if (backBufferCount < 2 || backBufferCount > MAX_BACK_BUFFER_COUNT) {
        throw out_of_range("invalid backBufferCount");
    }

    if (minFeatureLevel < D3D_FEATURE_LEVEL_11_0) {
        throw out_of_range("minFeatureLevel too low");
    }
}

// Destructor for UIDeviceResources.
UIDeviceResources::~UIDeviceResources() {
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();
}

// Configures the Direct3D device, and stores handles to it and the device context.
void UIDeviceResources::CreateDeviceDependentResources() {
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
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

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
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

    _d3dDevice->SetName(L"UIDeviceResources");

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

    _commandQueue->SetName(L"UIDeviceResources");

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = _backBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(_rtvDescriptorHeap.ReleaseAndGetAddressOf())));

    _rtvDescriptorHeap->SetName(L"UIDeviceResources");

    _rtvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (_depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

        _dsvDescriptorHeap->SetName(L"UIDeviceResources");
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

    _commandList->SetName(L"UIDeviceResources");

    // Create a fence for tracking GPU execution progress.
    ThrowIfFailed(_d3dDevice->CreateFence(_fenceValues[_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())));
    _fenceValues[_backBufferIndex]++;

    _fence->SetName(L"UIDeviceResources");

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
void UIDeviceResources::CreateWindowSizeDependentResources() {
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
        }
        else {
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

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            _rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(n), _rtvDescriptorSize);
        _d3dDevice->CreateRenderTargetView(_renderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    _backBufferIndex = _swapChain->GetCurrentBackBufferIndex();

    if (_depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            _depthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1, // This depth stencil view has only one texture.
            1  // Use a single mipmap level.
            );
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

    // Set the 3D rendering viewport and scissor rectangle to target the entire window.
    _screenViewport.TopLeftX = _screenViewport.TopLeftY = 0.f;
    _screenViewport.Width = static_cast<float>(backBufferWidth);
    _screenViewport.Height = static_cast<float>(backBufferHeight);
    _screenViewport.MinDepth = D3D12_MIN_DEPTH;
    _screenViewport.MaxDepth = D3D12_MAX_DEPTH;

    _scissorRect.left = _scissorRect.top = 0;
    _scissorRect.right = static_cast<LONG>(backBufferWidth);
    _scissorRect.bottom = static_cast<LONG>(backBufferHeight);
}

// This method is called when the Win32 window is created (or re-created).
void UIDeviceResources::SetWindowHWnd(int width, int height) {
    _outputSize.left = _outputSize.top = 0;
    _outputSize.right = width;
    _outputSize.bottom = height;
}

// This method is called when the Win32 window changes size.
bool UIDeviceResources::HandleWindowSizeChanged(int width, int height) {
    RECT newRc;
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
    return true;
}

// Recreate all device resources and set them back to the current state.
void UIDeviceResources::HandleDeviceLost() {
    UIDXFoundation::GetSingletonInstance()->ResetResources();

    for (UINT n = 0; n < _backBufferCount; n++) {
        _commandAllocators[n].Reset();
        _renderTargets[n].Reset();
    }

    _depthStencil.Reset();
    _commandQueue.Reset();
    _commandList.Reset();
    _fence.Reset();
    _rtvDescriptorHeap.Reset();
    _dsvDescriptorHeap.Reset();
    _swapChain.Reset();
    _d3dDevice.Reset();
    _dxgiFactory.Reset();

#ifdef _DEBUG 
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();

    UIDXFoundation::GetSingletonInstance()->CreateResources();
}

// Prepare the command list and render target for rendering.
void UIDeviceResources::Prepare(D3D12_RESOURCE_STATES beforeState) {
    // Reset command list and allocator.
    ThrowIfFailed(_commandAllocators[_backBufferIndex]->Reset());
    ThrowIfFailed(_commandList->Reset(_commandAllocators[_backBufferIndex].Get(), nullptr));

    if (beforeState != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        // Transition the render target into the correct state to allow for drawing into it.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_backBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        _commandList->ResourceBarrier(1, &barrier);
    }
}

// Present the contents of the swap chain to the screen.
void UIDeviceResources::Present(D3D12_RESOURCE_STATES beforeState) {
    if (beforeState != D3D12_RESOURCE_STATE_PRESENT) {
        // Transition the render target to the state that allows it to be presented to the display.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_backBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_PRESENT);
        _commandList->ResourceBarrier(1, &barrier);
    }

    // Send the command list off to the GPU for processing.
    ThrowIfFailed(_commandList->Close());
    _commandQueue->ExecuteCommandLists(1, CommandListCast(_commandList.GetAddressOf()));

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
void UIDeviceResources::WaitForGpu() noexcept {
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
void UIDeviceResources::MoveToNextFrame() {
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
void UIDeviceResources::GetAdapter(IDXGIAdapter1** ppAdapter) {
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
void UIDeviceResources::UpdateColorSpace() {
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



UIDXFoundation::UIDXFoundation() noexcept(false) {
    p_deviceResources = make_unique<UIDeviceResources>();
}

UIDXFoundation::~UIDXFoundation() {
    if (p_deviceResources) {
        p_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void UIDXFoundation::Initialize(int width, int height) {
    p_deviceResources->SetWindowHWnd(width, height);

    p_deviceResources->CreateDeviceDependentResources();
    CreateDeviceDependentResourcesXTK();
    
    p_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResourcesXTK();
}

LONG UIDXFoundation::GetOutputWidth() const {
	return GetRectWidth()(p_deviceResources->_outputSize);
}

LONG UIDXFoundation::GetOutputHeight() const {
	return GetRectHeight()(p_deviceResources->_outputSize);
}

#pragma region Frame Render
void UIDXFoundation::RenderAnimate() {
    // static int flag = 0;
    // if (flag == 0) {
    //     flag = 1;
    //     UISetCaretPos(50, 50);
	// 	UIShowCaret(0.1, 20, 150, Colors::Red);
    // }
}

void UIDXFoundation::Render2D() {

    Draw2DPoint(XMFLOAT2(0, 0), 0.3, Colors::Blue, 1);
    Draw2DPoint(XMFLOAT2(1, 1), 0.5, Colors::Red, 1);
    Draw2DPoint(XMFLOAT2(2, 2), 0.5, Colors::Red, 1);
    Draw2DPoint(XMFLOAT2(3, 3), 0.5, Colors::Red, 1);
    Draw2DPoint(XMFLOAT2(4, 4), 0.5, Colors::Red, 1);
    Draw2DPoint(XMFLOAT2(5, 5), 0.5, Colors::Red, 1);

    vector<XMFLOAT2> points = {
        XMFLOAT2(0, 6),
        XMFLOAT2(1, 6),
        XMFLOAT2(2, 6),
        XMFLOAT2(3, 6),
        XMFLOAT2(4, 6),
        XMFLOAT2(5, 6)
    };  
    Draw2DPoints(points, 0.5, Colors::Blue, 1);

    Draw2DLine(XMFLOAT2(0, 7), XMFLOAT2(5, 7), 0.5, Colors::Red, 1);
    Draw2DLine(XMFLOAT2(6, 0), XMFLOAT2(6, 5), 0.5, Colors::Green, 1);

    //Draw2DRectSolid(XMFLOAT2(0, 8), XMFLOAT2(5, 9), 0.5, Colors::Blue, 255);
    Draw2DRectOutline(XMFLOAT2(0, 8), XMFLOAT2(5, 10), 0.5, Colors::Blue, 1);




    Draw2DImage(L"C:\\cat.png", UIColor::Invalid, NULL_RECT, XMFLOAT2(0, 12), XMFLOAT2(5, 13), 0.5, 255);


    //Draw2DPoint(XMFLOAT2(300, 5), 0.5, Colors::Red, 1);


    // UIPoint(300, 5, 0.5)(Colors::Red);

    // vector<POINT> points = {
    //     POINT{310, 6},
    //     POINT{320, 7},
    //     POINT{330, 8},
    //     POINT{340, 9},
    // };
    // UIPoints(points, 0.5)(Colors::Blue);

    //UILine(10, 10, 200, 10, 0.2)(Colors::Blue);
    // UILine(10, 10, 200, 200, 0.5)(Colors::Blue);
    // UILine(10, 10, 10, 200, 0.2)(Colors::Blue);

    // // load image from dll

    // UIImage(L"C:\\GUIResource.dll", 80003, UIColor(255, 0, 255, 0), 0.3)(NULL_RECT, RECT{ 100, 75, 0, 0 });
    // UIImage(L"C:\\cat.png", UIColor::Invalid, 0.5)(NULL_RECT, RECT{ 100, 100, 0, 0 });
    // UIImage(L"C:\\checkbox_nor.bmp", UIColor(255, 0, 255, 0), 0.5)(NULL_RECT, RECT{ 100, 500, 300, 800 });
 
    // //Draw2DRect(XMFLOAT2(500, 10), XMFLOAT2(600, 200), Colors::Pink, 0.5, 100);
    // //Draw2DRect(XMFLOAT2(700, 10), XMFLOAT2(800, 200), Colors::Aquamarine, 0.5);
    // {
    //     UIScreenClipRectGuard clipRect(RECT{ 450, 50, 550, 150 });
    //     UIRect(RECT{ 500, 10, 600, 200 }, 0.5)(Colors::Pink, 100);
    // }
    // UIRect(RECT{ 700, 10, 800, 200 }, 0.3)(Colors::Red);
    // //UIRect(RECT{ 700, 10, 800, 200 }, 0.4)(Colors::Blue, 200);
    // // UILine(700, 10, 800, 200, 0.5)(Colors::Blue);
    // // UILine(700, 10, 700, 200, 0.3)(Colors::Blue);
    // // UILine(700, 10, 800, 10, 0.2)(Colors::Red);
    // // UILine(800, 10, 800, 200, 0.3)(Colors::Blue);
    // // UILine(700, 200, 800, 200, 0.2)(Colors::Red);
    
    
    // // Draw2DText(L"DirectXTK Simple UIDXFoundation", XMFLOAT2(100, 10), Colors::Black, 0.5, 18);
    // // Draw2DText(L"AmazeUI\n *?@&%$#", RECT{ 700, 10, 800, 200 }, 0x04, Colors::Blue, 0.5, 18);
    // //UIFont(0.5, 14)(L"DirectXTK Simple UIDXFoundation", POINT{100, 10});
    // UIFont(0.1, 14)(L"VAIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII", RECT{ 700, 10, 800, 200 }, Colors::Blue);

    // UIRect(RECT{ 100, 10, 500, 200 }, 0.3)(Colors::Red);
    // //Draw2DTextFT(L"DirectXTK Simple UIDXFoundation", XMFLOAT2(100, 10), 0.3, Colors::Black, 24);
    // Draw2DTextFT(L"���� DirectXTK Simple UIDXFoundation", RECT{ 100, 10, 500, 200 }, 0x04, 0.3, Colors::Black, 24);

    // UISlicedImage(L"C:\\GUIResource.dll", IDB_BUTTON1_NORMAL, UIColor(255, 0, 255, 0), 3, 3, 3, 3, 0.5)(RECT{ 850, 50, 1050, 100 });
    // UIImage(L"C:\\GUIResource.dll", IDB_BUTTON1_NORMAL, UIColor(255, 0, 255, 0), 0.5)(RECT{ 850, 130, 1050, 180 });
}

void UIDXFoundation::Render3D() {
    // 3D test
     //XMFLOAT3 worldPos1 = UICameraUI::GetSingletonInstance()->ConvertScreen2DTo3D(XMFLOAT3(100, 100, 0.01f));
     //XMFLOAT3 worldPos2 = UICameraUI::GetSingletonInstance()->ConvertScreen2DTo3D(XMFLOAT3(100, 100, 0.5f));
     //XMFLOAT3 worldPos3 = UICameraUI::GetSingletonInstance()->ConvertScreen2DTo3D(XMFLOAT3(100, 100, 1.0f));

     //XMFLOAT3 screenPos11 = UICameraUI::GetSingletonInstance()->Convert3DToScreen2D(worldPos1);
     //XMFLOAT3 screenPos22 = UICameraUI::GetSingletonInstance()->Convert3DToScreen2D(worldPos2);
     //XMFLOAT3 screenPos33 = UICameraUI::GetSingletonInstance()->Convert3DToScreen2D(worldPos3);

    //Draw2DRectOutline(XMFLOAT2(400, 100), XMFLOAT2(1000, 300), 0.9, Colors::Red);
    //Draw3DRectOutline(XMFLOAT2(400, 100), XMFLOAT2(1000, 300), 0.5, Colors::Black);
    //Draw3DRectSolid(XMFLOAT2(400, 100), XMFLOAT2(1000, 300), 1.0, Colors::Red, Colors::Blue, Colors::Green, Colors::Yellow, 255);


    // Draw3DRectOutline(XMFLOAT2(100, 10), XMFLOAT2(500, 200), 0.5, Colors::Red);
    // RECT rc = RECT{ 100, 10, 500, 200 };
    // POINT center = GetRectCenter()(rc);
    // XMMATRIX transform = UIZPlaneTransform::GetTransformMatrix(false, 0, 0, XM_PI/32, true, center.y, XM_PI/32, true, center.x, XM_PI/32, 0.5);
    // Draw3DTextFT(L"DirectXTK Simple UIDXFoundation", XMFLOAT2(100, 10), 0.3, Colors::Black, 24, transform);





    {
        //p_lineEffect3DGame->SetWorld(Matrix::Identity);

        // //Draw procedurally generated dynamic 3D grid
        // const XMVECTORF32 xaxis = { 20.f, 0.f, 0.f };
        // const XMVECTORF32 yaxis = { 0.f, 0.f, 20.f };
        // DrawGrid(xaxis, yaxis, g_XMZero, 20, 20, Colors::Gray);

        auto commandList = p_deviceResources->GetCommandList();

    static int ii = 0;
    _world = Matrix::CreateRotationY(float(++ii * XM_PIDIV4));

    // Draw 3D object
    // Draw teapot
    XMMATRIX local = _world * Matrix::CreateTranslation(-2.f, -2.f, 4.f);
    p_shapeEffectGame->SetWorld(local);
    p_shapeEffectGame->Apply(commandList);
    p_shape->Draw(commandList);

    //Draw model
        // ID3D12DescriptorHeap* heaps[] = { p_modelResources->Heap(), p_states->Heap() };
        // commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        // const XMVECTORF32 scale = { 0.01f, 0.01f, 0.01f };
        // const XMVECTORF32 translate = { 3.f, -2.f, 4.f };
        // XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(XM_PI / 2.f, 0.f, -XM_PI / 2.f);
        // local = _world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);
        // Model::UpdateEffectMatrices(_modelEffectsGame, local, UICameraGame::GetSingletonInstance()->GetViewMatrix(), UICameraGame::GetSingletonInstance()->GetProjectionMatrix());
        
        // p_model->Draw(commandList, _modelEffectsGame.begin());
    }
}

// Draws the scene.
void UIDXFoundation::Render() {
    // Prepare the command list to render a new frame.
    p_deviceResources->Prepare();
    Clear();

    // Set the descriptor heaps
    auto commandList = p_deviceResources->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { p_resourceDescriptors->Heap(), p_states->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    //RenderAnimate();
    //Render2D();
    Render3D();

    // draw top container
    UIFrame::GetSingletonInstance()->GetTopUIContainer()->Draw();

    // draw animations
    UIAnimationManage::GetSingletonInstance()->DrawAnimations();

    // Show the new frame.
    p_deviceResources->Present();
    p_graphicsMemory->Commit(p_deviceResources->GetCommandQueue());
}

// Get texture rect
RECT UIDXFoundation::Get2DTextureRect(ID3D12Resource* texture) {
    RECT rect = NULL_RECT;
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
RECT UIDXFoundation::Get2DTextureRect(const ComPtr<ID3D12Resource>& texture) {
    return Get2DTextureRect(texture.Get());
}

bool UIDXFoundation::Get2DImageSize(const wstring& imagePath, const UIColor& colorKey, RECT& textureRect) {
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

bool UIDXFoundation::Get2DImageSize(const wstring& dllPath, UINT id, const UIColor& colorKey, RECT& textureRect) {
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
bool UIDXFoundation::ConvertImageTransparencyByWIC(ComPtr<IWICImagingFactory>& wicFactory, ComPtr<IWICBitmapDecoder>& decoder, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) {
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

bool UIDXFoundation::ConvertImageTransparencyByWIC(const void* data, size_t size, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) {   
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

bool UIDXFoundation::ConvertImageTransparencyByWIC(const wstring& filePath, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) {
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

bool UIDXFoundation::ConvertImageTransparencyByDDS(const wstring& filePath, const UIColor& colorKey, vector<uint8_t>& imageData, UINT& width, UINT& height) { 
    ComPtr<ID3D12Resource> texture;
    unique_ptr<uint8_t[]> ddsData;
    vector<D3D12_SUBRESOURCE_DATA> subresources;
    DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;
    bool isCubeMap = false;

    // Load DDS texture
    HRESULT hr = LoadDDSTextureFromFile(
        p_deviceResources->GetD3DDevice(),
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

// create texture
bool CreateTextureFromImageData(ID3D12Device* device, ResourceUploadBatch& resourceUpload, ComPtr<ID3D12Resource>& texture, vector<uint8_t>& imageData, UINT& width, UINT& height)
{
    // create texture description
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
    
    // create texture resource
    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
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

    // create subresource data
    D3D12_SUBRESOURCE_DATA subResData = {};
    subResData.pData = imageData.data();
    subResData.RowPitch = width * 4;
    subResData.SlicePitch = subResData.RowPitch * height;

    // upload texture data
    resourceUpload.Upload(
        texture.Get(),
        0,
        &subResData,
        1
    );

    // transition resource to pixel shader resource state
    resourceUpload.Transition(
        texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    return true;
}

bool UIDXFoundation::GetWICTextureIndexFromFile(const wstring& filePath, const UIColor& colorKey, size_t& textureIndex) {
    // use filePath as key
    wstring resourceKey = filePath + (colorKey.IsValid() ? L"#" + colorKey.ToWStringForU32() : L"");

    auto it = _textureResourceMap.find(resourceKey);
    if (it == _textureResourceMap.end()) {
        auto device = p_deviceResources->GetD3DDevice();
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
        }
        else {
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
        size_t descriptorIndex = _textureResourceMap.size()+Descriptors::Offset1;
        
        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), p_resourceDescriptors->GetCpuHandle(descriptorIndex));
        
        // save GPU descriptor handle
        resource._gpuDescriptor = p_resourceDescriptors->GetGpuHandle(descriptorIndex);
        
        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(p_deviceResources->GetCommandQueue());
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

bool UIDXFoundation::GetWICTextureIndexFromDLL(const wstring& dllPath, UINT id, const UIColor& colorKey, size_t& textureIndex) {
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

        auto device = p_deviceResources->GetD3DDevice();
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
        }
        else {
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
        size_t descriptorIndex = _textureResourceMap.size()+Descriptors::Offset1;
            
        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), p_resourceDescriptors->GetCpuHandle(descriptorIndex));

        // save GPU descriptor handle
        resource._gpuDescriptor = p_resourceDescriptors->GetGpuHandle(descriptorIndex);

        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(p_deviceResources->GetCommandQueue());
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

bool UIDXFoundation::GetDDSTextureIndexFromFile(const wstring& filePath, const UIColor& colorKey, size_t& textureIndex)
{
    // use filePath as key
    wstring resourceKey = filePath + (colorKey.IsValid() ? L"#" + colorKey.ToWStringForU32() : L"");

    auto it = _textureResourceMap.find(resourceKey);
    if (it == _textureResourceMap.end()) {
        auto device = p_deviceResources->GetD3DDevice();
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
        }
        else {
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
        size_t descriptorIndex = _textureResourceMap.size()+Descriptors::Offset1;
        
        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), p_resourceDescriptors->GetCpuHandle(descriptorIndex));
        
        // save GPU descriptor handle
        resource._gpuDescriptor = p_resourceDescriptors->GetGpuHandle(descriptorIndex);
        
        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(p_deviceResources->GetCommandQueue());
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

float UIDXFoundation::CalculateNdcZByOrtho(float z_view) {
    // get current orthographic projection matrix
    Matrix ortho = _orthoMatrix2D;
    
    // for orthographic projection matrix, z transformation is linear
    // z_ndc = (z_view * m22 + m32) / 1.0  (because w component is always 1.0)
    float z_ndc = (z_view * ortho.m[2][2] + ortho.m[3][2]) / 1.0f;
    
    // output result
    //OutputDebugStringA(("View z: " + to_string(z_view) + " -> NDC z: " + to_string(z_ndc) + "\n").c_str());
    return z_ndc;
}

float UIDXFoundation::CalculateViewZByOrtho(float z_ndc) {
    // get current orthographic projection matrix
    Matrix ortho = _orthoMatrix2D;
    
    // for orthographic projection matrix, z transformation is linear
    // z_view = (z_ndc - m32) / m22
    float z_view = (z_ndc - ortho.m[3][2]) / ortho.m[2][2];
    
    //OutputDebugStringA(("NDC z: " + to_string(z_ndc) + " -> View z: " + to_string(z_view) + "\n").c_str());
    return z_view;
}

void UIDXFoundation::Calculate2DPoint(const DirectX::XMFLOAT2& point, DirectX::XMFLOAT2& p) {
    p.x = point.x + 0.5f;
    p.y = point.y + 0.5f;
}

void UIDXFoundation::Calculate2DLinePoints(const XMFLOAT2& start, const XMFLOAT2& end, XMFLOAT2& p1, XMFLOAT2& p2) {
    // for the "Top-Left Rule" in the triangle rending
    // Horizontal line: Move 1 pixel down, add 1 pixel right  
    // Vertical line: Move 1 pixel right, add 1 pixel down
    // Diagonal line (right endpoint): Move 1 pixel right  
    // Diagonal line (bottom endpoint): Move 1 pixel down 
    p1 = start;
    p2 = end;
    if (fabs(p1.x-p2.x)<0.001f) {
        p1.x += 1.0f;
        p2.x += 1.0f;
        p1.y > p2.y ? p1.y+=1.0f : p2.y+=1.0f;

    } else if (fabs(p1.y-p2.y)<0.001f) {
        p1.y += 1.0f;
        p2.y += 1.0f;
        p1.x > p2.x ? p1.x+=1.0f : p2.x+=1.0f;
    } else {
        p1.x > p2.x ? p1.x+=1.0f : p2.x+=1.0f;
        p1.y > p2.y ? p1.y+=1.0f : p2.y+=1.0f;
    }
}

void UIDXFoundation::Calculate2DRectPoints(const XMFLOAT2& start, const XMFLOAT2& end, XMFLOAT2& ps, XMFLOAT2& pe) {
    ps.x = start.x<=end.x ? start.x:end.x;
    ps.y = start.y<=end.y ? start.y:end.y;
    pe.x = start.x>end.x ? start.x:end.x;
    pe.y = start.y>end.y ? start.y:end.y;
    pe.x += 1.0f;
    pe.y += 1.0f;
}

void UIDXFoundation::BeginScreenClipRect(const RECT& clipRC) {
    auto commandList = p_deviceResources->GetCommandList();

    RECT tempRC = NULL_RECT;

    if (clipRC.left>clipRC.right || clipRC.top>clipRC.bottom) {
        tempRC = NULL_RECT;
    } else {
        if (_clipRectStack.size() > 0) {
            RECT preClipRC = _clipRectStack.top();

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
        tempRC.left>1 ? tempRC.left-1 : 0,
        tempRC.top>1 ? tempRC.top-1 : 0,
        tempRC.right+1,
        tempRC.bottom+1
    };
    _clipRectStack.push(scissorRect);

    commandList->RSSetScissorRects(1, &scissorRect);
}

void UIDXFoundation::EndScreenClipRect() {
    auto commandList = p_deviceResources->GetCommandList();

    _clipRectStack.pop();
    if (_clipRectStack.size() > 0) {
        RECT preClipRC = _clipRectStack.top();
        commandList->RSSetScissorRects(1, &preClipRC);
    }
    else {
        // if it's the last clip rect, reset to full screen
        D3D12_RECT fullscreenRect = {0, 0, LONG_MAX, LONG_MAX};
        commandList->RSSetScissorRects(1, &fullscreenRect);
    }
}

void UIDXFoundation::Draw2DPoint(const XMFLOAT2& point, float z, const UIColor& color, float pointSize) {
    float viewZ = CalculateViewZByOrtho(z);

    XMFLOAT2 p;
    Calculate2DPoint(point, p);

    // Define the point size (2x2 pixels)
    const float halfSize = pointSize/2;
    
    // Create a small square with 4 vertices
    VertexPositionColor vertices[4] = {
        {{ p.x - halfSize, p.y - halfSize, viewZ }, color.ToXMVECTORF32()},
        {{ p.x - halfSize, p.y + halfSize, viewZ }, color.ToXMVECTORF32()},
        {{ p.x + halfSize, p.y - halfSize, viewZ }, color.ToXMVECTORF32()},
        {{ p.x + halfSize, p.y + halfSize, viewZ }, color.ToXMVECTORF32()}
    };
    // Define the indexes of the two triangles
    uint16_t indices[6] = { 
        0, 1, 2,  // 1st triangle
        1, 3, 2   // 2nd triangle
    };
    
    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleEffect2D->Apply(commandList);
    //  
    p_batch->Begin(commandList);
    // Draw the suqare bulilt by 2 triangles
    p_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batch->End();




    // only one vertex    (1,1) is on the pixel (0,0)
    // VertexPositionColor vertex = { { point.x, point.y, viewZ }, color.ToXMVECTORF32() };
    
    // auto commandList = p_deviceResources->GetCommandList();
       
    // p_pointEffect2D->Apply(commandList);
    // p_batch->Begin(commandList);
    // p_batch->Draw(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, &vertex, 1);
    // p_batch->End();
}

void UIDXFoundation::Draw2DPoints(const vector<XMFLOAT2>& points, float z, const UIColor& color, float pointSize) {
    if (points.empty()) {
        return;
    }
    
    float viewZ = CalculateViewZByOrtho(z);

    vector<XMFLOAT2> pList;
    for (const auto& point : points) {
        XMFLOAT2 p;
        Calculate2DPoint(point, p);
        pList.push_back(p);
    }

    // Pre-allocate vertex and index arrays
    const size_t pointCount = points.size();
    vector<VertexPositionColor> vertices;
    vector<uint16_t> indices;
    vertices.reserve(pointCount * 4);  // 4 vertices per poin
    indices.reserve(pointCount * 6);   // 6 indices per point (2 triangles)
    
    // Calculate half size for each point
    const float halfSize = pointSize / 2.0f;
    
    // Create a small square for each point
    for (size_t i = 0; i < pointCount; ++i) {
        const XMFLOAT2& point = pList[i];
        
        // Add 4 vertices
        vertices.push_back({{point.x - halfSize, point.y - halfSize, viewZ}, color.ToXMVECTORF32()}); // Top-left
        vertices.push_back({{point.x - halfSize, point.y + halfSize, viewZ}, color.ToXMVECTORF32()}); // Bottom-left
        vertices.push_back({{point.x + halfSize, point.y - halfSize, viewZ}, color.ToXMVECTORF32()}); // Top-right
        vertices.push_back({{point.x + halfSize, point.y + halfSize, viewZ}, color.ToXMVECTORF32()}); // Bottom-right
        
        // Add 6 indices (2 triangles)
        const uint16_t baseIndex = static_cast<uint16_t>(i * 4);
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 3);
        indices.push_back(baseIndex + 2);
    }
    
    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleEffect2D->Apply(commandList);
    //
    p_batch->Begin(commandList);
    p_batch->DrawIndexed(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        indices.data(),
        static_cast<uint32_t>(indices.size()),
        vertices.data(),
        static_cast<uint32_t>(vertices.size())
    );
    p_batch->End();
}

void UIDXFoundation::Draw2DLine(const XMFLOAT2& start, const XMFLOAT2& end, float z, const UIColor& color, float lineWidth) {
    XMVECTORF32 finalColor = color.ToXMVECTORF32();

    XMFLOAT2 p1, p2;
    Calculate2DLinePoints(start, end, p1, p2);
    // calculate line direction vector
    XMFLOAT2 dir = { p2.x - p1.x, p2.y - p1.y };
    float length = sqrt(dir.x * dir.x + dir.y * dir.y);
    if (length < 0.0001f) {
        return;
    }
    // normalize and calculate normal vector
    dir.x /= length;
    dir.y /= length;
    XMFLOAT2 normal = { -dir.y, dir.x };
    // calculate four vertices, using transformed viewZ
    float halfWidth = max(1.0f, lineWidth) * 0.5f;

    // get viewZ
    float viewZ = CalculateViewZByOrtho(z);

    // calculate four vertices
    VertexPositionColor vertices[4] = {
        {{ p1.x + normal.x * halfWidth, p1.y + normal.y * halfWidth, viewZ }, finalColor},
        {{ p1.x - normal.x * halfWidth, p1.y - normal.y * halfWidth, viewZ }, finalColor},
        {{ p2.x + normal.x * halfWidth, p2.y + normal.y * halfWidth, viewZ }, finalColor},
        {{ p2.x - normal.x * halfWidth, p2.y - normal.y * halfWidth, viewZ }, finalColor}
    };
    // draw two triangles
    uint16_t indices[6] = { 0, 1, 2, 1, 3, 2 };

    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleEffect2D->Apply(commandList);
    //
    p_batch->Begin(commandList);
    p_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batch->End();
}

// Draw a 2D rectangle with specified color, hollow style
void UIDXFoundation::Draw2DRectOutline(const XMFLOAT2& start, const XMFLOAT2& end, float z, const UIColor& color, float lineWidth) {    
    Draw2DLine(XMFLOAT2(start.x, start.y), XMFLOAT2(end.x, start.y), z, color, lineWidth); // up
    Draw2DLine(XMFLOAT2(end.x, start.y), XMFLOAT2(end.x, end.y), z, color, lineWidth); //right
    Draw2DLine(XMFLOAT2(start.x, end.y), XMFLOAT2(end.x, end.y), z, color, lineWidth); // down
    Draw2DLine(XMFLOAT2(start.x, start.y), XMFLOAT2(start.x, end.y), z, color, lineWidth); // left
}

void UIDXFoundation::Draw2DRectSolid(const XMFLOAT2& start, const XMFLOAT2& end, float z, 
						        const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha) {
    XMVECTORF32  finalColorLT = colorLT.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorRT = colorRT.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorLB = colorLB.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorRB = colorRB.ToXMVECTORF32(alpha);
    
    // ps: real start point
    // pe: real end point
    XMFLOAT2 ps, pe;
    Calculate2DRectPoints(start, end, ps, pe);

    float viewZ = CalculateViewZByOrtho(z);

    // Create vertices for the rectangle corners
    VertexPositionColor vertices[4] = {
        {{ ps.x, ps.y, viewZ }, finalColorLT},         // Top-left (0)
        {{ pe.x, ps.y, viewZ }, finalColorRT},         // Top-right (1)
        {{ ps.x, pe.y, viewZ }, finalColorLB},         // Bottom-left (2)
        {{ pe.x, pe.y, viewZ }, finalColorRB}          // Bottom-right (3)
    };
    // Define indices for two triangles
    uint16_t indices[6] = { 0, 1, 2, 1, 3, 2 };

    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleEffect2D->Apply(commandList);
    //
    p_batch->Begin(commandList);
    p_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batch->End();
}

// Draw a 2D rectangle with specified color, alpha transparency and z-depth
void UIDXFoundation::Draw2DRectSolid(const XMFLOAT2& start, const XMFLOAT2& end, float z, const UIColor& color, UCHAR alpha) {
    Draw2DRectSolid(start, end, z, color, color, color, color, alpha);
}

void UIDXFoundation::Draw2DImage(size_t textureIndex, 
                            RECT srcRect, XMFLOAT2 dstStart, XMFLOAT2 dstEnd, 
                            float z, UCHAR alpha) {
    // set color
    XMVECTORF32 color = { 1.0f, 1.0f, 1.0f, alpha / 255.0f };
    
    // get texture resource
    TextureResource& resource = _textureResources[textureIndex];

    // get original texture size 
    const RECT textureRect = Get2DTextureRect(resource._texture);

    // if srcRect is NULL_RECT, use textureRect
    if (IsRectEmpty(&srcRect)) {
        srcRect = textureRect;
    }

    // if dstEnd is 0, calculate it
    if (dstEnd.x == 0 || dstEnd.y == 0) {
        dstEnd.x = dstStart.x + (float)(GetRectWidth()(srcRect)) - 1;
        dstEnd.y = dstStart.y + (float)(GetRectHeight()(srcRect)) - 1;
    }

    XMFLOAT2 scale(
        (dstEnd.x - dstStart.x + 1) / (float)(GetRectWidth()(srcRect)),
        (dstEnd.y - dstStart.y + 1) / (float)(GetRectHeight()(srcRect))
    );
    
    // begin drawing
    auto comandList = p_deviceResources->GetCommandList();
    p_sprites->Begin(comandList);
    
    // set render state
    RenderTargetState rtState(p_deviceResources->GetBackBufferFormat(), p_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    p_sprites->SetViewport(p_deviceResources->GetScreenViewport());

    p_sprites->Draw(
        resource._gpuDescriptor,
        GetTextureSize(resource._texture.Get()), 
        dstStart,
        &srcRect,
        color,
        0.0f,  // rotation
        XMFLOAT2(0, 0),  // origin
        scale,
        SpriteEffects_None,
        z
    );

    p_sprites->End();
}

void UIDXFoundation::Draw2DImage(const wstring& filePath, const UIColor& colorKey,
                            const RECT& srcRect, const XMFLOAT2& dstStart, const XMFLOAT2& dstEnd,
                            float z, UCHAR alpha) {
    size_t textureIndex = 0;
    // check is dds file
    if (filePath.find(L".dds") != wstring::npos) {
        if (!GetDDSTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    } else {
        if (!GetWICTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    }
    Draw2DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha);
}

void UIDXFoundation::Draw2DImage(const wstring& dllPath, UINT id, const UIColor& colorKey,
                            const RECT& srcRect, const XMFLOAT2& dstStart, const XMFLOAT2& dstEnd,
                            float z, UCHAR alpha) {
    size_t textureIndex = 0; 
    if (!GetWICTextureIndexFromDLL(dllPath, id, colorKey, textureIndex)) {
        return;
    }
    Draw2DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha);
}

#if 0
void UIDXFoundation::Draw2DText(const wstring& text, const XMFLOAT2& position, float z, const UIColor& color, float fontSize) {
    if (text.empty()) {
        return;
    }

     // calculate font scale factor
    float fontScale = fontSize / gLoadFontSize;

    // begin drawing
    auto comandList = p_deviceResources->GetCommandList();
    p_sprites->Begin(comandList);
    p_font->DrawString(
        p_sprites.get(),
        text.c_str(),
        position,
        color.ToXMVECTORF32(),
        0.0f,           // rotation
        XMFLOAT2(0, 0), // origin offset
        XMFLOAT2(fontScale, fontScale), // scale
        SpriteEffects_None,
        z               // z depth
    );
    p_sprites->End();
}

// posFlag: 0x01 - horizontal center, 0x02 - right, 0x04 - vertical center, 0x08 - bottom
void UIDXFoundation::Draw2DText(const wstring& text, const RECT& rc, int posFlag, float z, const UIColor& color, float fontSize) {
    if (text.empty()) {
        return;
    }

    // calculate font scale factor
    float fontScale = fontSize / gLoadFontSize;
    
    // measure text size
    XMVECTOR textSize = p_font->MeasureString(text.c_str());
    float textWidth = XMVectorGetX(textSize) * fontScale;
    float textHeight = XMVectorGetY(textSize) * fontScale;
    
    // calculate text position
    float x = static_cast<float>(rc.left);
    float y = static_cast<float>(rc.top);
    
    // horizontal alignment
    if (posFlag & 0x01) {  // center
        x = rc.left + (rc.right - rc.left - textWidth) / 2;
    } else if (posFlag & 0x02) {  // right
        x = rc.right - textWidth;
    }

    // vertical alignment
    if (posFlag & 0x04) { // center
        y = rc.top + (rc.bottom - rc.top - textHeight) / 2;
    } else if (posFlag & 0x08) {  // bottom
        y = rc.bottom - textHeight;
    }

    // clip text
    UIScreenClipRectGuard clipRect(rc);

    // begin drawing
    auto comandList = p_deviceResources->GetCommandList();
    p_sprites->Begin(comandList);
    p_font->DrawString(
        p_sprites.get(),
        text.c_str(),
        XMFLOAT2(x, y),
        color.ToXMVECTORF32(),
        0.0f,           // rotation
        XMFLOAT2(0, 0), // origin offset
        XMFLOAT2(fontScale, fontScale), // scale
        SpriteEffects_None,
        z               // z depth
    );
    p_sprites->End();
}

SIZE UIDXFoundation::GetTextSize(const wstring& text, float fontSize) {
    if (text.empty()) {
        return SIZE();
    }
    
    // calculate font scale factor
    float fontScale = fontSize / gLoadFontSize;

    // measure text size, ignore whitespace
    XMVECTOR textSize = p_font->MeasureString(text.c_str(), false);
    float width = XMVectorGetX(textSize);
    float height = XMVectorGetY(textSize);

    // return size
    return CreateSize()(static_cast<LONG>(width * fontScale), static_cast<LONG>(height * fontScale));
}
#endif

void UIDXFoundation::Draw3DPoint(const XMFLOAT2& point, float z, const UIColor& color, float pointSize, const XMMATRIX& transformMatrix) {
    XMVECTORF32 finalColor = color.ToXMVECTORF32();

    XMFLOAT2 p;
    Calculate2DPoint(point, p);

    // Define the point size (2x2 pixels)
    const float halfSize = pointSize/2;

    vector<XMFLOAT2> quadPoints = {
        { p.x - halfSize, p.y - halfSize },
        { p.x - halfSize, p.y + halfSize },
        { p.x + halfSize, p.y - halfSize },
        { p.x + halfSize, p.y + halfSize }
    };

    vector<XMFLOAT3> wps;
    UIZPlaneTransform::TransformPoints(transformMatrix, quadPoints, z, wps);

    // Create a small square with 4 vertices
    VertexPositionColor vertices[4] = {
        { { wps[0].x, wps[0].y, wps[0].z }, finalColor },  // left top
        { { wps[1].x, wps[1].y, wps[1].z }, finalColor },  // left bottom
        { { wps[2].x, wps[2].y, wps[2].z }, finalColor },  // right top
        { { wps[3].x, wps[3].y, wps[3].z }, finalColor }   // right bottom
    };
    // Define the indexes of the two triangles
    uint16_t indices[6] = { 
        0, 1, 2,  // 1st triangle
        1, 3, 2   // 2nd triangle
    };
    
    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleEffect3DUI->Apply(commandList);
    //
    p_batch->Begin(commandList);
    p_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batch->End();
}

void UIDXFoundation::Draw3DPoints(const vector<XMFLOAT2>& points, float z, const UIColor& color, float pointSize, 
                             const XMMATRIX& transformMatrix) {
    if (points.empty()) {
        return;
    }

    XMVECTORF32 finalColor = color.ToXMVECTORF32();

    // Calculate the number of vertices and indices required
    size_t pointCount = points.size();
    size_t vertexCount = pointCount * 4;  // 4 vertices per point
    size_t indexCount = pointCount * 6;   // 6 indices per point (2 triangles)

    // fix points to screen space
    vector<XMFLOAT2> pList(pointCount);
    for (size_t i = 0; i < pointCount; i++) {
        Calculate2DPoint(points[i], pList[i]);
    }

    const float halfSize = pointSize / 2;

    // Pre-allocate vertices and indices arrays
    vector<VertexPositionColor> vertices(vertexCount);
    vector<uint16_t> indices(indexCount);

    // create quadrilateral geometry for each point (in screen space)
    vector<XMFLOAT2> quadPoints(vertexCount);
    for (size_t i = 0; i < pointCount; i++) {
        const XMFLOAT2& p = pList[i];
        size_t baseIndex = i * 4;
        
        // define the four corners of the quadrilateral
        quadPoints[baseIndex] = { p.x - halfSize, p.y - halfSize };     // left top
        quadPoints[baseIndex + 1] = { p.x - halfSize, p.y + halfSize }; // left bottom
        quadPoints[baseIndex + 2] = { p.x + halfSize, p.y - halfSize }; // right top
        quadPoints[baseIndex + 3] = { p.x + halfSize, p.y + halfSize }; // right bottom
    }

    // transform the quadrilateral from screen space to 3D space
    vector<XMFLOAT3> wps(vertexCount);
    UIZPlaneTransform::TransformPoints(transformMatrix, quadPoints, z, wps);

    // create vertices and indices buffer
    for (size_t i = 0; i < pointCount; i++) {
        size_t vertexIdx = i * 4;
        size_t indexIdx = i * 6;
        
        // set vertices data
        vertices[vertexIdx] = { { wps[vertexIdx].x, wps[vertexIdx].y, wps[vertexIdx].z }, finalColor };     // left top
        vertices[vertexIdx + 1] = { { wps[vertexIdx + 1].x, wps[vertexIdx + 1].y, wps[vertexIdx + 1].z }, finalColor }; // left bottom
        vertices[vertexIdx + 2] = { { wps[vertexIdx + 2].x, wps[vertexIdx + 2].y, wps[vertexIdx + 2].z }, finalColor }; // right top
        vertices[vertexIdx + 3] = { { wps[vertexIdx + 3].x, wps[vertexIdx + 3].y, wps[vertexIdx + 3].z }, finalColor }; // right bottom
        
        // set indices data (each point has two triangles)
        uint16_t vertexBase = static_cast<uint16_t>(vertexIdx);
        indices[indexIdx] = vertexBase;            // first triangle: left top
        indices[indexIdx + 1] = vertexBase + 1;    // first triangle: left bottom
        indices[indexIdx + 2] = vertexBase + 2;    // first triangle: right top
        indices[indexIdx + 3] = vertexBase + 1;    // second triangle: left bottom
        indices[indexIdx + 4] = vertexBase + 3;    // second triangle: right bottom
        indices[indexIdx + 5] = vertexBase + 2;    // second triangle: right top
    }

    // execute drawing
    auto commandList = p_deviceResources->GetCommandList();
    
    p_triangleEffect3DUI->Apply(commandList);
    
    p_batch->Begin(commandList);
    p_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices.data(), static_cast<uint32_t>(indexCount), 
                         vertices.data(), static_cast<uint32_t>(vertexCount));
    p_batch->End();
}

void UIDXFoundation::Draw3DLine(const XMFLOAT2& start, const XMFLOAT2& end, float z, const UIColor& color, float lineWidth, const XMMATRIX& transformMatrix) {
    XMVECTORF32 finalColor = color.ToXMVECTORF32();

    XMFLOAT2 p1, p2;
    Calculate2DLinePoints(start, end, p1, p2);
    // calculate line direction vector
    XMFLOAT2 dir = { p2.x - p1.x, p2.y - p1.y };
    float length = sqrt(dir.x * dir.x + dir.y * dir.y);
    if (length < 0.0001f) {
        return;
    }
    // normalize and calculate normal vector
    dir.x /= length;
    dir.y /= length;
    XMFLOAT2 normal = { -dir.y, dir.x };
    // calculate four vertices, using transformed viewZ
    float halfWidth = max(1.0f, lineWidth) * 0.5f;

    vector<XMFLOAT2> points = {
        { p1.x + normal.x * halfWidth, p1.y + normal.y * halfWidth },
        { p1.x - normal.x * halfWidth, p1.y - normal.y * halfWidth },
        { p2.x + normal.x * halfWidth, p2.y + normal.y * halfWidth },
        { p2.x - normal.x * halfWidth, p2.y - normal.y * halfWidth }
    };

    vector<XMFLOAT3> wps;
    UIZPlaneTransform::TransformPoints(transformMatrix, points, z, wps);

     // create rectangle vertices
    VertexPositionColor vertices[4] = {
        { { wps[0].x, wps[0].y, wps[0].z }, finalColor },  // left top
        { { wps[1].x, wps[1].y, wps[1].z }, finalColor },  // right top
        { { wps[2].x, wps[2].y, wps[2].z }, finalColor },  // left bottom
        { { wps[3].x, wps[3].y, wps[3].z }, finalColor }   // right bottom
    };
    // Define indices for two triangles
    uint16_t indices[6] = { 0, 1, 2, 1, 3, 2 };

    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleEffect3DUI->Apply(commandList);
    //
    p_batch->Begin(commandList);
    p_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batch->End();
}

void UIDXFoundation::Draw3DRectOutline(const XMFLOAT2& start, const XMFLOAT2& end, float z, const UIColor& color, float lineWidth, const XMMATRIX& transformMatrix) {    
    Draw3DLine(XMFLOAT2(start.x, start.y), XMFLOAT2(end.x, start.y), z, color, lineWidth, transformMatrix); // up
    Draw3DLine(XMFLOAT2(end.x, start.y), XMFLOAT2(end.x, end.y), z, color, lineWidth, transformMatrix); //right
    Draw3DLine(XMFLOAT2(start.x, end.y), XMFLOAT2(end.x, end.y), z, color, lineWidth, transformMatrix); // down
    Draw3DLine(XMFLOAT2(start.x, start.y), XMFLOAT2(start.x, end.y), z, color, lineWidth, transformMatrix); // left
}

// Draw a 3D rectangle with specified color, alpha transparency and z-depth
void UIDXFoundation::Draw3DRectSolid(const XMFLOAT2& start, const XMFLOAT2& end, float z,
                                const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha,
                                const XMMATRIX& transformMatrix) {
    // create final color with alpha
    XMVECTORF32  finalColorLT = colorLT.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorRT = colorRT.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorLB = colorLB.ToXMVECTORF32(alpha);
    XMVECTORF32  finalColorRB = colorRB.ToXMVECTORF32(alpha);

    XMFLOAT2 ps, pe;
    Calculate2DRectPoints(start, end, ps, pe);
    vector<XMFLOAT3> wps;
    UIZPlaneTransform::TransformRectPoints(transformMatrix, ps, pe, z, wps);
    
    // create rectangle vertices
    VertexPositionColor vertices[4] = {
        { { wps[0].x, wps[0].y, wps[0].z }, finalColorLT },  // left top
        { { wps[1].x, wps[1].y, wps[1].z }, finalColorRT },  // right top
        { { wps[2].x, wps[2].y, wps[2].z }, finalColorLB },  // left bottom
        { { wps[3].x, wps[3].y, wps[3].z }, finalColorRB }   // right bottom
    };
    // Define indices for two triangles
    uint16_t indices[6] = { 0, 1, 2, 1, 3, 2 };

    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleEffect3DUI->Apply(commandList);
    //
    p_batch->Begin(commandList);
    p_batch->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batch->End();
}

void UIDXFoundation::Draw3DRectSolid(const XMFLOAT2& start, const XMFLOAT2& end, float z,
                                const UIColor& color, UCHAR alpha,
                                const XMMATRIX& transformMatrix) {
    Draw3DRectSolid(start, end, z, color, color, color, color, alpha, transformMatrix);
}

void UIDXFoundation::Draw3DImage(size_t textureIndex, 
                            RECT srcRect, XMFLOAT2 dstStart, XMFLOAT2 dstEnd, 
                            float z, UCHAR alpha, 
                            const XMMATRIX& transformMatrix) {
    XMVECTOR color = XMVectorSet(1.0f, 1.0f, 1.0f, alpha / 255.0f);
    
    // get the texture rect
    const RECT textureRect = Get2DTextureRect(_textureResources[textureIndex]._texture);

    // if srcRect is NULL_RECT, use textureRect
    if (IsRectEmpty(&srcRect)) {
        srcRect = textureRect;
    }

    // if dstEnd is 0, calculate it
    if (dstEnd.x == 0 || dstEnd.y == 0) {
        dstEnd.x = dstStart.x + (float)(GetRectWidth()(srcRect));
        dstEnd.y = dstStart.y + (float)(GetRectHeight()(srcRect));
    }

    XMFLOAT2 ps, pe;
    Calculate2DRectPoints(dstStart, dstEnd, ps, pe);
    vector<XMFLOAT3> wps;
    UIZPlaneTransform::TransformRectPoints(transformMatrix, ps, pe, z, wps);

    // create vertices with texture coordinates
    VertexPositionTexture vertices[4] = {
        { {wps[0].x, wps[0].y, wps[0].z}, XMFLOAT2((float)srcRect.left/GetRectWidth()(textureRect), (float)srcRect.top/GetRectHeight()(textureRect)) },
        { {wps[1].x, wps[1].y, wps[1].z}, XMFLOAT2((float)srcRect.right/GetRectWidth()(textureRect), (float)srcRect.top/GetRectHeight()(textureRect)) },
        { {wps[2].x, wps[2].y, wps[2].z}, XMFLOAT2((float)srcRect.left/GetRectWidth()(textureRect), (float)srcRect.bottom/GetRectHeight()(textureRect)) },
        { {wps[3].x, wps[3].y, wps[3].z}, XMFLOAT2((float)srcRect.right/GetRectWidth()(textureRect), (float)srcRect.bottom/GetRectHeight()(textureRect)) }
    };
    // Define indices for two triangles
    uint16_t indices[6] = { 0, 1, 2, 1, 3, 2 };

    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleTexturedEffect3DUI->SetTexture(_textureResources[textureIndex]._gpuDescriptor, p_states->LinearClamp());
    p_triangleTexturedEffect3DUI->SetColorAndAlpha(color);
    p_triangleTexturedEffect3DUI->Apply(commandList);
    //
    p_batchTexture->Begin(commandList);
    p_batchTexture->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batchTexture->End();
}

void UIDXFoundation::Draw3DImage(const wstring& dllPath, UINT id, const UIColor& colorKey,
					 const RECT& srcRect, const XMFLOAT2& dstStart, const XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, 
					 const XMMATRIX& transformMatrix) {
    size_t textureIndex = 0; 
    if (!GetWICTextureIndexFromDLL(dllPath, id, colorKey, textureIndex)) {
        return;
    }

    Draw3DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha, transformMatrix);
}

void UIDXFoundation::Draw3DImage(const wstring& filePath, const UIColor& colorKey,
					 const RECT& srcRect, const XMFLOAT2& dstStart, const XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, 
					 const XMMATRIX& transformMatrix) {
    size_t textureIndex = 0;
    // check is dds file
    if (filePath.find(L".dds") != wstring::npos) {
        if (!GetDDSTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    } else {
        if (!GetWICTextureIndexFromFile(filePath, colorKey, textureIndex)) {
            return;
        }
    }

    Draw3DImage(textureIndex, srcRect, dstStart, dstEnd, z, alpha, transformMatrix);
}

// Helper method to clear the back buffers.
void UIDXFoundation::Clear() {
    auto commandList = p_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = p_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = p_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, UIColor::White.ToXMVECTORF32(), 0, nullptr);   //CornflowerBlue
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = p_deviceResources->GetScreenViewport();
    auto scissorRect = p_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void XM_CALLCONV UIDXFoundation::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color) {
    auto commandList = p_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Draw grid");

    p_lineEffect3DGame->Apply(commandList);

    p_batch->Begin(commandList);

    xdivs = max<size_t>(1, xdivs);
    ydivs = max<size_t>(1, ydivs);

    for (size_t i = 0; i <= xdivs; ++i) {
        float fPercent = float(i) / float(xdivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
        VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
        p_batch->DrawLine(v1, v2);
    }

    for (size_t i = 0; i <= ydivs; i++) {
        float fPercent = float(i) / float(ydivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
        VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
        p_batch->DrawLine(v1, v2);
     }

    p_batch->End();
    
    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void UIDXFoundation::OnActivated()
{
}

void UIDXFoundation::OnDeactivated()
{
}

void UIDXFoundation::OnSuspending()
{
}

void UIDXFoundation::OnResuming()
{
}

void UIDXFoundation::HandleWindowSizeChanged(int width, int height)
{
    if (!p_deviceResources->HandleWindowSizeChanged(width, height)) {
        return;
    }

    CreateWindowSizeDependentResourcesXTK();
}

void UIDXFoundation::NewAudioDevice()
{
}

#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void UIDXFoundation::CreateDeviceDependentResourcesXTK() {
    auto device = p_deviceResources->GetD3DDevice();

    p_graphicsMemory = make_unique<GraphicsMemory>(device);
    p_states = make_unique<CommonStates>(device);
    p_resourceDescriptors = make_unique<DescriptorHeap>(device, Descriptors::Count);
    p_batch = make_unique<PrimitiveBatch<VertexPositionColor>>(device);
    p_batchTexture = make_unique<PrimitiveBatch<VertexPositionTexture>>(device);
    p_shape = GeometricPrimitive::CreateTeapot(4.f, 8);


    // SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"tiny.sdkmesh");

    wchar_t txtPath[MAX_PATH] = {};
    {
        wchar_t drive[_MAX_DRIVE];
        wchar_t path[_MAX_PATH];

        if (_wsplitpath_s(strFilePath, drive, _MAX_DRIVE, path, _MAX_PATH, nullptr, 0, nullptr, 0)) {
            throw exception("_wsplitpath_s");
        }

        if (_wmakepath_s(txtPath, _MAX_PATH, drive, path, nullptr, nullptr)) {
            throw exception("_wmakepath_s");
        }
    }

    p_model = Model::CreateFromSDKMESH(device, strFilePath);

    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        RenderTargetState rtState(p_deviceResources->GetBackBufferFormat(), p_deviceResources->GetDepthBufferFormat());

        p_model->LoadStaticBuffers(device, resourceUpload);

        p_modelResources = p_model->LoadTextures(device, resourceUpload, txtPath);

        {
            EffectPipelineStateDescription psd(
                nullptr,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullClockwise,    // Using RH coordinates, and SDKMESH is in LH coordiantes
                rtState);

            EffectPipelineStateDescription alphapsd(
                nullptr,
                CommonStates::NonPremultiplied, // Using straight alpha
                CommonStates::DepthRead,
                CommonStates::CullClockwise,    // Using RH coordinates, and SDKMESH is in LH coordiantes
                rtState);

            _modelEffectsGame = p_model->CreateEffects(psd, alphapsd, p_modelResources->Heap(), p_states->Heap());
        }

        DX::FindMediaFile(strFilePath, MAX_PATH, L"windowslogo.dds");
        ThrowIfFailed(
            CreateDDSTextureFromFile(device, resourceUpload, strFilePath, _texture1.ReleaseAndGetAddressOf())
        );
        CreateShaderResourceView(device, _texture1.Get(), p_resourceDescriptors->GetCpuHandle(Descriptors::WindowsLogo));

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

        // create common depth description
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depthStencilDesc.StencilEnable = FALSE;

        {
            SpriteBatchPipelineStateDescription pd(rtState);
            pd.blendDesc = transparentBlendDesc;
            pd.depthStencilDesc = depthStencilDesc;

            p_sprites = make_unique<SpriteBatch>(device, resourceUpload, pd);
        }

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
            p_lineEffect3DGame = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
        }

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
            p_triangleEffect3DUI = make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
        }

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

        {
            EffectPipelineStateDescription pd(
                &VertexPositionTexture::InputLayout,
                CommonStates::AlphaBlend,
                CommonStates::DepthDefault,
                CommonStates::CullNone,
                rtState,
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

            pd.blendDesc = transparentBlendDesc;
            pd.depthStencilDesc = depthStencilDesc;

            p_triangleTexturedEffect3DUI = make_unique<BasicEffect>(device, EffectFlags::Texture, pd);
        }

        {
            EffectPipelineStateDescription pd(
                &GeometricPrimitive::VertexType::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullNone,
                rtState);

            p_shapeEffectGame = make_unique<BasicEffect>(device, EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
            p_shapeEffectGame->EnableDefaultLighting();
            p_shapeEffectGame->SetTexture(p_resourceDescriptors->GetGpuHandle(Descriptors::WindowsLogo), p_states->AnisotropicWrap());
        }

        // // load MSYHFont
        // DX::FindMediaFile(strFilePath, MAX_PATH, format(L"MSYH_{}.spritefont", gLoadFontSize).c_str());
        // p_font = make_unique<SpriteFont>(device, resourceUpload,
        //     strFilePath,
        //     p_resourceDescriptors->GetCpuHandle(Descriptors::MSYHFont),
        //     p_resourceDescriptors->GetGpuHandle(Descriptors::MSYHFont));

        // Upload the resources to the GPU.
        auto uploadResourcesFinished = resourceUpload.End(p_deviceResources->GetCommandQueue());

        // Wait for the upload thread to terminate
        uploadResourcesFinished.wait();
    }

    CreateResourcesFT();
}

// Allocate all memory resources that change on a window SizeChanged event.
void UIDXFoundation::CreateWindowSizeDependentResourcesXTK() {
    auto size = p_deviceResources->_outputSize;

    // 2D resources
    auto viewport = p_deviceResources->GetScreenViewport();
    p_sprites->SetViewport(viewport);
    
    // create 2D orthographic projection matrix
    _orthoMatrix2D = Matrix::CreateOrthographicOffCenter(
        0.0f, static_cast<float>(size.right),
        static_cast<float>(size.bottom), 0.0f,
        0.0f, 1.0f //-1.0f, 1.0f
    );
    
    // set the matrices for 2D drawing
    p_pointEffect2D->SetView(Matrix::Identity);
    p_pointEffect2D->SetWorld(Matrix::Identity);
    p_pointEffect2D->SetProjection(_orthoMatrix2D);

    p_lineEffect2D->SetView(Matrix::Identity);
    p_lineEffect2D->SetWorld(Matrix::Identity);
    p_lineEffect2D->SetProjection(_orthoMatrix2D);

    p_triangleEffect2D->SetView(Matrix::Identity);
    p_triangleEffect2D->SetWorld(Matrix::Identity);
    p_triangleEffect2D->SetProjection(_orthoMatrix2D);

    // 3D resources
    UICameraUI::GetSingletonInstance()->SetCameraFor2D(float(size.right), float(size.bottom));
    //
    p_triangleEffect3DUI->SetView(UICameraUI::GetSingletonInstance()->GetViewMatrix());
    p_triangleEffect3DUI->SetProjection(UICameraUI::GetSingletonInstance()->GetProjectionMatrix());
    p_triangleTexturedEffect3DUI->SetView(UICameraUI::GetSingletonInstance()->GetViewMatrix());
    p_triangleTexturedEffect3DUI->SetProjection(UICameraUI::GetSingletonInstance()->GetProjectionMatrix());
    
    UICameraGame::GetSingletonInstance()->SetAspectRatioAndProjectionMatrix(float(size.right) / float(size.bottom));
    UICameraGame::GetSingletonInstance()->SetViewMatrix();
    //
    p_lineEffect3DGame->SetView(UICameraGame::GetSingletonInstance()->GetViewMatrix());
    p_lineEffect3DGame->SetProjection(UICameraGame::GetSingletonInstance()->GetProjectionMatrix());
    p_shapeEffectGame->SetView(UICameraGame::GetSingletonInstance()->GetViewMatrix());
    p_shapeEffectGame->SetProjection(UICameraGame::GetSingletonInstance()->GetProjectionMatrix());
}

void UIDXFoundation::CreateResources() {
    CreateDeviceDependentResourcesXTK();
    CreateWindowSizeDependentResourcesXTK();
}

void UIDXFoundation::ResetResources() {
    _texture1.Reset();

    //p_font.reset();
    p_batch.reset();
    p_batchTexture.reset();
    p_shape.reset();
    p_model.reset();
    p_lineEffect3DGame.reset();
    p_lineEffect2D.reset();
    p_triangleEffect2D.reset();
    p_pointEffect2D.reset();
    p_triangleEffect3DUI.reset();
    p_triangleTexturedEffect3DUI.reset();
    p_shapeEffectGame.reset();
    _modelEffectsGame.clear();
    p_modelResources.reset();
    p_sprites.reset();
    p_resourceDescriptors.reset();
    p_states.reset();
    p_graphicsMemory.reset();

    stack<RECT>().swap(_clipRectStack);

    ResetResourcesFT();
}

// create FreeType resources and load frequently used characters
void UIDXFoundation::CreateResourcesFT() {
    // create FreeType library
    if (FT_Init_FreeType(&_ftLibrary)) {
        OutputDebugString(L"FreeType initialization failed\n");
        return;
    }
    
    // load frequently used characters
}

// reset FreeType resources
void UIDXFoundation::ResetResourcesFT() {
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

bool UIDXFoundation::GetFTSizeFont(float fontSize, FTSizeFont& ftSizeFont) {
    // check if the font size already exists
    auto it = _ftSizeFontMap.find(fontSize);
    if (it == _ftSizeFontMap.end()) {
        // create new font
        FTSizeFont ftSizeFontCache;

        // load font
        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"times.ttf");
        if (FT_New_Face(_ftLibrary, WSTR_TO_STR(strFilePath).c_str(), 0, &ftSizeFontCache._ftFace)) {
            return false;
        }

        memset(strFilePath, 0, MAX_PATH);
        DX::FindMediaFile(strFilePath, MAX_PATH, L"simsun.ttc");
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
                       bitmap.width
                    <-------------->
                    ┌──────────────┐       ┬
                    │              │       │
                    │              │       │ bitmap.rows
      baseline      │    glyph     │       │
      ──────────────┼──────────────┼───────┼────────────────────────
                    │              │       │
                    └──────────────┘       ┴
                    ^              ^
                    │              │
                    │              └─── right edge of bitmap
                    │
                    └─── left edge of bitmap

      <-bitmap_left->|              |<----------advance.x---------->|
      
      ^
      │
      origin point       


    horiBearingX >> 6: from the origin point to the left edge of the glyph bitmap
    horiBearingY >> 6: from the baseline to the top edge of the glyph bitmap
    advance.x >> 6: the horizontal distance after rendering this glyph
    bitmap_left: the horizontal distance from the left edge of the glyph bitmap to the origin point
    bitmap_top: the vertical distance from the top edge of the glyph bitmap to the baseline

    for space char：
    bitmap.width & bitmap.rows:0
    bitmap_left & bitmap_top:0
    advance.x >> 6 will decide the width of the space
    Note：>> 6 is a displacement operation that converts FreeType's 1/64 pixel units to integer pixel units.
*/
bool UIDXFoundation::GetCharTextureResourceFT(const wchar_t& wch, float fontSize, const UIColor& color, size_t& textureIndex) {
    // use wchar+color as the key
    wstring resourcekey = wch + (L"#" + to_wstring(fontSize)) + (color.IsValid() ? L"#" + color.ToWStringForU32() : L"");

    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);

    // check if the resource already exists
    auto it = _charTextureResourceMap.find(resourcekey);
    if (it == _charTextureResourceMap.end()) {
        auto device = p_deviceResources->GetD3DDevice();
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

                if (grey > 0) {
                    // convert to RGBA format
                    UINT index = (y * width + x) * 4;
                    imageData[index] = color._r; // R
                    imageData[index + 1] = color._g; // G
                    imageData[index + 2] = color._b; // B
                    imageData[index + 3] = (grey * color._a) / 255; // A
                }
            }
        }

        if (!CreateTextureFromImageData(device, resourceUpload, resource._texture, imageData, width, height)) {
            return false;
        }

        // get next available descriptor index
        size_t descriptorIndex = _charTextureResources.size()+Descriptors::Offset2;
        
        // create shader resource view
        CreateShaderResourceView(device, resource._texture.Get(), p_resourceDescriptors->GetCpuHandle(descriptorIndex));
        
        // save GPU descriptor handle
        resource._gpuDescriptor = p_resourceDescriptors->GetGpuHandle(descriptorIndex);
        
        // wait for resource upload finished
        auto uploadResourcesFinished = resourceUpload.End(p_deviceResources->GetCommandQueue());
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

SIZE UIDXFoundation::GetTextSizeFT(const wstring& text, float fontSize) {
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);
    
    SIZE size;
    size.cx = 0;
    size.cy = ftSizeFont._ftFontHeight;

    // render each character
    for (wchar_t ch : text) {      
        // get or create texture
        size_t textureIndex;
        if (!GetCharTextureResourceFT(ch, fontSize, UIColor::Black, textureIndex)) {
            continue; // skip characters that cannot be loaded
        }

        // get character resource
        CharTextureResource& resource = _charTextureResources[textureIndex];
        
        // update pen position
        size.cx += resource._advance;
    }
    
    return size;
}

void UIDXFoundation::Draw2DCharTextureFT(size_t textureIndex, XMFLOAT2 position, float z, float scale, UCHAR alpha) {
    position.x = round(position.x);
    //position.y = round(position.y);
    
    // get texture resource
    CharTextureResource& resource = _charTextureResources[textureIndex];
    
    // get original texture size 
    const RECT textureRect = Get2DTextureRect(resource._texture);
    
    // set color
    XMVECTORF32 color = { 1.0f, 1.0f, 1.0f, alpha / 255.0f };
    
    // begin drawing
    auto comandList = p_deviceResources->GetCommandList();
    p_sprites->Begin(comandList);
    
    // set render state
    RenderTargetState rtState(p_deviceResources->GetBackBufferFormat(), p_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    p_sprites->SetViewport(p_deviceResources->GetScreenViewport());

    p_sprites->Draw(
        resource._gpuDescriptor,
        GetTextureSize(resource._texture.Get()), 
        position,
        &textureRect,
        color,
        0.0f,  // rotation
        XMFLOAT2(0, 0),  // origin
        scale,
        SpriteEffects_None,
        z
    );

    p_sprites->End();
}

void UIDXFoundation::Draw3DCharTextureFT(size_t textureIndex, DirectX::XMFLOAT2 position, float z, float scale, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix)
{
    position.x = round(position.x);
    //position.y = round(position.y);

    // set color
    XMVECTORF32 color = { 1.0f, 1.0f, 1.0f, alpha / 255.0f };
    
    // get original texture size 
    const RECT textureRect = Get2DTextureRect(_charTextureResources[textureIndex]._texture);

    XMFLOAT2 dstEnd;
    dstEnd.x = position.x + (float)(GetRectWidth()(textureRect))*scale;
    dstEnd.y = position.y + (float)(GetRectHeight()(textureRect))*scale;

    XMFLOAT2 ps, pe;
    Calculate2DRectPoints(position, dstEnd, ps, pe);
    vector<XMFLOAT3> wps;
    UIZPlaneTransform::TransformRectPoints(transformMatrix, ps, pe, z, wps);

    // create vertices with texture coordinates
    VertexPositionTexture vertices[4] = {
        { {wps[0].x, wps[0].y, wps[0].z}, XMFLOAT2(0.f, 0.f) },
        { {wps[1].x, wps[1].y, wps[1].z}, XMFLOAT2(1.f, 0.f) },
        { {wps[2].x, wps[2].y, wps[2].z}, XMFLOAT2(0.f, 1.f) },
        { {wps[3].x, wps[3].y, wps[3].z}, XMFLOAT2(1.f, 1.f) }
    };
    // Define indices for two triangles
    uint16_t indices[6] = { 0, 1, 2, 1, 3, 2 };

    auto commandList = p_deviceResources->GetCommandList();
    //
    p_triangleTexturedEffect3DUI->SetTexture(_charTextureResources[textureIndex]._gpuDescriptor, p_states->LinearClamp());
    p_triangleTexturedEffect3DUI->SetColorAndAlpha(color);
    p_triangleTexturedEffect3DUI->Apply(commandList);
    //
    p_batchTexture->Begin(commandList);
    p_batchTexture->DrawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indices, 6, vertices, 4);
    p_batchTexture->End();
}

void UIDXFoundation::Draw2DTextFT(const wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize) {
    float x = position.x;
    float y = position.y;

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
        Draw2DCharTextureFT(textureIndex, XMFLOAT2(charX, charY), z, 1.0f, 255);
        
        // update pen position
        x += resource._advance;
    }
}

void UIDXFoundation::Draw2DTextFT(const wstring& text, const RECT& rc, int posFlag, float z, const UIColor& color, float fontSize) {
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);
    
    // measure text size
    SIZE textSize = GetTextSizeFT(text, fontSize);
    float textWidth = (float)textSize.cx;
    //float textHeight = (float)textSize.cy;
    
    // calculate text position
    float x = static_cast<float>(rc.left);
    //float y = static_cast<float>(rc.top);

    // horizontal alignment
    if (posFlag & 0x01) {  // center
        x = rc.left + (rc.right - rc.left - textWidth) / 2;
    } else if (posFlag & 0x02) {  // right
        x = rc.right - textWidth;
    }

    // vertical alignment
    int baselineY = 0;
    if (posFlag & 0x04) { // center
        baselineY = rc.top + (GetRectHeight()(rc) - (int)(ftSizeFont._ftFontHeight))/2 + (int)(ftSizeFont._ftFontAscent);
    } else if (posFlag & 0x08) {  // bottom
        baselineY = rc.bottom - (int)(ftSizeFont._ftFontDescent);
    } else {
        baselineY = rc.top + (int)(ftSizeFont._ftFontAscent);
    }

    // clip text
    UIScreenClipRectGuard clipRect(rc);

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
        float charY = baselineY - resource._top;
        
        // draw character
        Draw2DCharTextureFT(textureIndex, XMFLOAT2(charX, charY), z, 1.0f, 255);
        
        // update pen position
        x += resource._advance;
    }
}

void UIDXFoundation::Draw3DTextFT(const wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize,
					         const DirectX::XMMATRIX& transformMatrix) {
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);

    float x = position.x;
    float y = position.y;

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
        Draw3DCharTextureFT(textureIndex, XMFLOAT2(charX, charY), z, 1.0f, 255, transformMatrix);
        
        // update pen position
        x += resource._advance;
    }
}

void UIDXFoundation::Draw3DTextFT(const wstring& text, const RECT& rc, int posFlag, float z, const UIColor& color, float fontSize,
					         const DirectX::XMMATRIX& transformMatrix) {
    FTSizeFont ftSizeFont;
    GetFTSizeFont(fontSize, ftSizeFont);
    
    // measure text size
    SIZE textSize = GetTextSizeFT(text, fontSize);
    float textWidth = (float)textSize.cx;
    //float textHeight = (float)textSize.cy;
    
    // calculate text position
    float x = static_cast<float>(rc.left);
    //float y = static_cast<float>(rc.top);

    // horizontal alignment
    if (posFlag & 0x01) {  // center
        x = rc.left + (rc.right - rc.left - textWidth) / 2;
    } else if (posFlag & 0x02) {  // right
        x = rc.right - textWidth;
    }

    // vertical alignment
    int baselineY = 0;
    if (posFlag & 0x04) { // center
        baselineY = rc.top + (GetRectHeight()(rc) - (int)(ftSizeFont._ftFontHeight))/2 + (int)(ftSizeFont._ftFontAscent);
    } else if (posFlag & 0x08) {  // bottom
        baselineY = rc.bottom - (int)(ftSizeFont._ftFontDescent);
    } else {
        baselineY = rc.top + (int)(ftSizeFont._ftFontAscent);
    }

    // clip text
    //UIScreenClipRectGuard clipRect(rc);

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
        float charY = baselineY - resource._top;
        
        // draw character
        Draw3DCharTextureFT(textureIndex, XMFLOAT2(charX, charY), z, 1.0f, 255, transformMatrix);
        
        // update pen position
        x += resource._advance;
    }
}
