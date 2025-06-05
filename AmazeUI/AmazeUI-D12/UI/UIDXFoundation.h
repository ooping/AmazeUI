#pragma once

#include "UIUtility.h"
#include "UIElement.h"
#include "UIWindow.h"


using Microsoft::WRL::ComPtr;



// Controls all the DirectX device resources.
class UIDXFoundation;
class UIDeviceResources {
	friend class UIDXFoundation;

public:
	static const unsigned int c_AllowTearing    = 0x1;
	static const unsigned int c_EnableHDR       = 0x2;

	UIDeviceResources(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
					  DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
					  UINT backBufferCount = 2,
					  D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
					  unsigned int flags = 0) noexcept(false);
	~UIDeviceResources();

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	void SetWindowHWnd(int width, int height);
	bool HandleWindowSizeChanged(int width, int height);

	void HandleDeviceLost();

	//void RegisterDeviceNotify(IDeviceNotify* deviceNotify) { _deviceNotify = deviceNotify; }
	void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
	void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
	void WaitForGpu() noexcept;

	// Direct3D Accessors.
	ID3D12Device*               GetD3DDevice() const            { return _d3dDevice.Get(); }
	IDXGISwapChain3*            GetSwapChain() const            { return _swapChain.Get(); }
	IDXGIFactory4*              GetDXGIFactory() const          { return _dxgiFactory.Get(); }
	D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const   { return _d3dFeatureLevel; }
	ID3D12Resource*             GetRenderTarget() const         { return _renderTargets[_backBufferIndex].Get(); }
	ID3D12Resource*             GetDepthStencil() const         { return _depthStencil.Get(); }
	ID3D12CommandQueue*         GetCommandQueue() const         { return _commandQueue.Get(); }
	ID3D12CommandAllocator*     GetCommandAllocator() const     { return _commandAllocators[_backBufferIndex].Get(); }
	ID3D12GraphicsCommandList*  GetCommandList() const          { return _commandList.Get(); }
	DXGI_FORMAT                 GetBackBufferFormat() const     { return _backBufferFormat; }
	DXGI_FORMAT                 GetDepthBufferFormat() const    { return _depthBufferFormat; }
	D3D12_VIEWPORT              GetScreenViewport() const       { return _screenViewport; }
	D3D12_RECT                  GetScissorRect() const          { return _scissorRect; }
	UINT                        GetCurrentFrameIndex() const    { return _backBufferIndex; }
	UINT                        GetBackBufferCount() const      { return _backBufferCount; }
	DXGI_COLOR_SPACE_TYPE       GetColorSpace() const           { return _colorSpace; }
	unsigned int                GetDeviceOptions() const        { return _options; }

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const {
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			static_cast<INT>(_backBufferIndex), _rtvDescriptorSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const {
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}

private:
	void MoveToNextFrame();
	void GetAdapter(IDXGIAdapter1** ppAdapter);
	void UpdateColorSpace();

	static const size_t MAX_BACK_BUFFER_COUNT = 3;

	UINT                                                _backBufferIndex;

	// Direct3D objects.
	Microsoft::WRL::ComPtr<ID3D12Device>                _d3dDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>          _commandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   _commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      _commandAllocators[MAX_BACK_BUFFER_COUNT];

	// Swap chain objects.
	Microsoft::WRL::ComPtr<IDXGIFactory4>               _dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain3>             _swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource>              _renderTargets[MAX_BACK_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Resource>              _depthStencil;

	// Presentation fence objects.
	Microsoft::WRL::ComPtr<ID3D12Fence>                 _fence;
	UINT64                                              _fenceValues[MAX_BACK_BUFFER_COUNT];
	Microsoft::WRL::Wrappers::Event                     _fenceEvent;

	// Direct3D rendering objects.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        _rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        _dsvDescriptorHeap;
	UINT                                                _rtvDescriptorSize;
	D3D12_VIEWPORT                                      _screenViewport;
	D3D12_RECT                                          _scissorRect;

	// Direct3D properties.
	DXGI_FORMAT                                         _backBufferFormat;
	DXGI_FORMAT                                         _depthBufferFormat;
	UINT                                                _backBufferCount;
	D3D_FEATURE_LEVEL                                   _d3dMinFeatureLevel;

	// Cached device properties.
	D3D_FEATURE_LEVEL                                   _d3dFeatureLevel;
	DWORD                                               _dxgiFactoryFlags;
	RECT                                                _outputSize;

	// HDR Support
	DXGI_COLOR_SPACE_TYPE                               _colorSpace;

	// UIDeviceResources options (see flags above)
	unsigned int                                        _options;
};

class UIDXFoundation : public SingletonPattern<UIDXFoundation> {
	friend class SingletonPattern<UIDXFoundation>;
	//friend class UIImage;

public:
    UIDXFoundation() noexcept(false);
    ~UIDXFoundation();

    // Initialization and management
    void Initialize(int width, int height);

    // DirectXTK objects resources
    void CreateResources();
	void ResetResources();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void NewAudioDevice();

	void HandleWindowSizeChanged(int width, int height);

	void Render();
	void Render2D();
	void Render3D();
	void RenderAnimate();

	LONG GetOutputWidth() const;
	LONG GetOutputHeight() const;

private:
    void Clear();

    void CreateDeviceDependentResourcesXTK();
    void CreateWindowSizeDependentResourcesXTK();

    void XM_CALLCONV DrawGrid(DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis, DirectX::FXMVECTOR origin, size_t xdivs, size_t ydivs, DirectX::GXMVECTOR color);

    // Device resources.
    std::unique_ptr<UIDeviceResources>        								p_deviceResources;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>                                p_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>                                p_resourceDescriptors;
    std::unique_ptr<DirectX::CommonStates>                                  p_states;

	std::unique_ptr<DirectX::BasicEffect>                                   p_pointEffect2D;
	std::unique_ptr<DirectX::BasicEffect>                                   p_lineEffect2D;
	std::unique_ptr<DirectX::BasicEffect>                                   p_triangleEffect2D;
	std::unique_ptr<DirectX::BasicEffect>                                   p_triangleEffect3DUI;
	std::unique_ptr<DirectX::BasicEffect>                                   p_triangleTexturedEffect3DUI;

	std::unique_ptr<DirectX::SpriteBatch>                                   p_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    p_font;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  p_batch;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>>  p_batchTexture;
	//
	std::unique_ptr<DirectX::BasicEffect>                                   p_lineEffect3DGame;
    std::unique_ptr<DirectX::BasicEffect>                                   p_shapeEffectGame;
    std::unique_ptr<DirectX::GeometricPrimitive>                            p_shape;
    std::unique_ptr<DirectX::Model>                                         p_model;
    DirectX::Model::EffectCollection                                        _modelEffectsGame;
    std::unique_ptr<DirectX::EffectTextureFactory>                          p_modelResources;

    Microsoft::WRL::ComPtr<ID3D12Resource>                                  _texture1;

	DirectX::SimpleMath::Matrix 											_orthoMatrix2D;
	// 3D
	DirectX::SimpleMath::Matrix 											_world;



    // Descriptors
    enum Descriptors {
        WindowsLogo,
		MSYHFont,
        Count = 10000,
		Offset1 = 10,
		Offset2 = 1000
    };

/*************************************************** Texture ***************************************************/
public:
	bool Get2DImageSize(const std::wstring& imagePath, const UIColor& colorKey, RECT& textureRect);
	bool Get2DImageSize(const std::wstring& dllPath, UINT id, const UIColor& colorKey, RECT& textureRect);

private:
	bool GetWICTextureIndexFromDLL(const std::wstring& dllPath, UINT id, const UIColor& colorKey, size_t& textureIndex);
	bool GetWICTextureIndexFromFile(const std::wstring& filePath, const UIColor& colorKey, size_t& textureIndex);
	bool GetDDSTextureIndexFromFile(const std::wstring& filePath, const UIColor& colorKey, size_t& textureIndex);

	bool ConvertImageTransparencyByWIC(ComPtr<IWICImagingFactory>& wicFactory, ComPtr<IWICBitmapDecoder>& decoder, const UIColor& colorKey, std::vector<uint8_t>& imageData, UINT& width, UINT& height);
	bool ConvertImageTransparencyByWIC(const void* data, size_t size, const UIColor& colorKey, std::vector<uint8_t>& imageData, UINT& width, UINT& height);
	bool ConvertImageTransparencyByWIC(const std::wstring& filePath, const UIColor& colorKey, std::vector<uint8_t>& imageData, UINT& width, UINT& height);
	bool ConvertImageTransparencyByDDS(const std::wstring& filePath, const UIColor& colorKey, std::vector<uint8_t>& imageData, UINT& width, UINT& height);

	RECT Get2DTextureRect(ID3D12Resource* texture);
	RECT Get2DTextureRect(const ComPtr<ID3D12Resource>& texture);

	struct TextureResource {
        ComPtr<ID3D12Resource> _texture;
		D3D12_GPU_DESCRIPTOR_HANDLE _gpuDescriptor;
		//D3D12_CPU_DESCRIPTOR_HANDLE _cpuDescriptor;
    };

	std::vector<TextureResource> _textureResources;
	std::unordered_map<std::wstring, size_t> _textureResourceMap;

/*************************************************** FreeType APIs ***************************************************/
public:
	void Draw2DTextFT(const std::wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize);
	void Draw2DTextFT(const std::wstring& text, const RECT& rc, int posFlag, float z, const UIColor& color, float fontSize);
	void Draw3DTextFT(const std::wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize,
					  const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DTextFT(const std::wstring& text, const RECT& rc, int posFlag, float z, const UIColor& color, float fontSize,
					  const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	SIZE GetTextSizeFT(const std::wstring& text, float fontSize);

private:
	void CreateResourcesFT();
	void ResetResourcesFT();

	bool GetCharTextureResourceFT(const wchar_t& wch, float fontSize, const UIColor& color, size_t& textureIndex);

	void Draw2DCharTextureFT(size_t textureIndex, DirectX::XMFLOAT2 position, float z, float scale, UCHAR alpha);
	void Draw3DCharTextureFT(size_t textureIndex, DirectX::XMFLOAT2 position, float z, float scale, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix);

	struct FTSizeFont;
	bool GetFTSizeFont(float fontSize, FTSizeFont& ftSizeFont);

	FT_Library _ftLibrary;

	struct FTSizeFont {
		FT_Face _ftFace;
		FT_Face _ftFaceBackup;

		int _ftFontAscent;
		int _ftFontDescent;
		int _ftFontHeight;
	};
	//
	std::unordered_map<float, FTSizeFont> _ftSizeFontMap;

	struct CharTextureResource {
		ComPtr<ID3D12Resource> _texture;
		D3D12_GPU_DESCRIPTOR_HANDLE _gpuDescriptor;
		UINT _width, _height;
		// int _bearingX, _bearingY;
		int _advance;
		int _left, _top;
	};
	//
	std::vector<CharTextureResource> _charTextureResources;
	std::unordered_map<std::wstring, size_t> _charTextureResourceMap;

/*************************************************** 2D APIs ***************************************************/
public:
	void Draw2DPoint(const DirectX::XMFLOAT2& point, float z, const UIColor& color, float pointSize=4);
	void Draw2DPoints(const std::vector<DirectX::XMFLOAT2>& points, float z, const UIColor& color, float pointSize=4);

	void Draw2DLine(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f);

	void Draw2DRectOutline(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f);
	void Draw2DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, UCHAR alpha);
	void Draw2DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, 
						 const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha);

	void Draw2DImage(const std::wstring& dllPath, UINT id, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha);
	void Draw2DImage(const std::wstring& filePath, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha);

#if 0
	void Draw2DText(const std::wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize);
	void Draw2DText(const std::wstring& text, const RECT& rc, int posFlag, float z, const UIColor& color, float fontSize);
	SIZE GetTextSize(const std::wstring& text, float fontSize);
#endif

	// 2D clip rect
	void BeginScreenClipRect(const RECT& clipRC);
	void EndScreenClipRect();

private:
	void Draw2DImage(size_t textureIndex, 
					 RECT srcRect, DirectX::XMFLOAT2 dstStart, DirectX::XMFLOAT2 dstEnd, 
					 float z, UCHAR alpha);

	float CalculateNdcZByOrtho(float z_view);
	float CalculateViewZByOrtho(float z_ndc);

	void Calculate2DPoint(const DirectX::XMFLOAT2& point, DirectX::XMFLOAT2& p);
	void Calculate2DLinePoints(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, DirectX::XMFLOAT2& p1, DirectX::XMFLOAT2& p2);
	void Calculate2DRectPoints(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, DirectX::XMFLOAT2& ps, DirectX::XMFLOAT2& pe);

	std::stack<RECT> _clipRectStack;

/*************************************************** 3D APIs ***************************************************/
public:
	void Draw3DPoint(const DirectX::XMFLOAT2& point, float z, const UIColor& color, float pointSize=4, 
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DPoints(const std::vector<DirectX::XMFLOAT2>& points, float z, const UIColor& color, float pointSize=4, 
					  const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DLine(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f, 
					const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DRectOutline(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f, 
						   const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z,
						 const UIColor& color, UCHAR alpha,
						 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z,
             		     const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha,
						 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DImage(const std::wstring& dllPath, UINT id, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, 
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DImage(const std::wstring& filePath, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, 
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

private:
	void Draw3DImage(size_t textureIndex, 
					 RECT srcRect, DirectX::XMFLOAT2 dstStart, DirectX::XMFLOAT2 dstEnd, 
					 float z, UCHAR alpha, 
					 const DirectX::XMMATRIX& transformMatrix);
};
