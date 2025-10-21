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
	//D3D12_VIEWPORT              GetScreenViewport() const       { return _screenViewport; }
	//D3D12_RECT                  GetScissorRect() const          { return _scissorRect; }
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
	//D3D12_VIEWPORT                                      _screenViewport;
	//D3D12_RECT                                          _scissorRect;

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

struct UIEffectManager {
    void Create();
    void Reset();

    // 2D Effects
    std::unique_ptr<DirectX::BasicEffect> p_pointEffect2D;
    std::unique_ptr<DirectX::BasicEffect> p_lineEffect2D;
    std::unique_ptr<DirectX::BasicEffect> p_triangleEffect2D;
    std::unique_ptr<DirectX::BasicEffect> p_triangleTexturedEffect2D;

    // 3D Effects
    std::unique_ptr<DirectX::BasicEffect> p_pointEffect3D;
    std::unique_ptr<DirectX::BasicEffect> p_lineEffect3D;
    std::unique_ptr<DirectX::BasicEffect> p_triangleEffect3D;
    std::unique_ptr<DirectX::BasicEffect> p_triangleTexturedEffect3D;
    std::unique_ptr<DirectX::BasicEffect> p_shapeEffect3D;

    // Skeletal Animation
    std::unique_ptr<DirectX::SkinnedEffect> p_skinnedEffect3D;
};

class UITextureManager {
	friend class UIDXFoundation;

public:
	// Public interface - Get texture size
	bool Get2DImageSize(const std::wstring& imagePath, const UIColor& colorKey, RECT& textureRect);
	bool Get2DImageSize(const std::wstring& dllPath, UINT id, const UIColor& colorKey, RECT& textureRect);

private:
	// Internal methods - Texture loading
	bool GetWICTextureIndexFromDLL(const std::wstring& dllPath, UINT id, const UIColor& colorKey, size_t& textureIndex);
	bool GetWICTextureIndexFromFile(const std::wstring& filePath, const UIColor& colorKey, size_t& textureIndex);
	bool GetDDSTextureIndexFromFile(const std::wstring& filePath, const UIColor& colorKey, size_t& textureIndex);

	// Internal methods - Image conversion
	bool ConvertImageTransparencyByWIC(Microsoft::WRL::ComPtr<IWICImagingFactory>& wicFactory, 
									   Microsoft::WRL::ComPtr<IWICBitmapDecoder>& decoder, 
									   const UIColor& colorKey, std::vector<uint8_t>& imageData, 
									   UINT& width, UINT& height);
	bool ConvertImageTransparencyByWIC(const void* data, size_t size, const UIColor& colorKey, 
									   std::vector<uint8_t>& imageData, UINT& width, UINT& height);
	bool ConvertImageTransparencyByWIC(const std::wstring& filePath, const UIColor& colorKey, 
									   std::vector<uint8_t>& imageData, UINT& width, UINT& height);
	bool ConvertImageTransparencyByDDS(const std::wstring& filePath, const UIColor& colorKey, 
									   std::vector<uint8_t>& imageData, UINT& width, UINT& height);

	// Shared utility - Create texture from image data (used by both Image and FreeType)
	bool CreateTextureFromImageData(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, 
									Microsoft::WRL::ComPtr<ID3D12Resource>& texture, 
									const std::vector<uint8_t>& imageData, UINT width, UINT height);

	// Internal methods - Utility functions
	RECT Get2DTextureRect(ID3D12Resource* texture);
	RECT Get2DTextureRect(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture);

	// Data members - Texture resources
	struct TextureResource {
		Microsoft::WRL::ComPtr<ID3D12Resource> _texture;
		D3D12_GPU_DESCRIPTOR_HANDLE _gpuDescriptor;
	};

	std::vector<TextureResource> _textureResources;
	std::unordered_map<std::wstring, size_t> _textureResourceMap;
};


class UIDXFoundation : public SingletonPattern<UIDXFoundation> {
	friend class SingletonPattern<UIDXFoundation>;
	
	// helper classes
	friend struct UIEffectManager;
	friend class UITextureManager;

public:
    UIDXFoundation() noexcept(false);
    ~UIDXFoundation();

    // Initialization and management
    void Initialize(int width, int height);

    // DirectXTK objects resources
    void CreateResources();
	void ResetResources();

	void HandleWindowSizeChanged(int width, int height);

	void Render();
	void Render3D();

	RECT GetOutputSize() const;
	LONG GetOutputWidth() const;
	LONG GetOutputHeight() const;

	ID3D12Device* GetD3DDevice() const;
	UIEffectManager* GetEffectManager() const;
	UITextureManager* GetTextureManager() const;
	UIDeviceResources* GetDeviceResources() const;
	DirectX::CommonStates* GetCommonStates() const;
	DirectX::DescriptorHeap* GetDescriptorHeap() const;

    // Descriptor allocation strategy for GPU resource views
    enum Descriptors {
        // System reserved descriptors (0-9)
        WindowsLogo = 0,
		MSYHFont = 1,
        SystemReservedCount = 10,
        
        // UI image textures (10-9999)
        UITexturesStart = 10,
        UITexturesCount = 9990,
        
        // Font character textures (10000-19999)
        FontTexturesStart = 10000,
        FontTexturesCount = 10000,
        
        // 3D model textures (20000-29999)
        ModelTexturesStart = 20000,
        ModelTexturesCount = 10000,
        
        // Total descriptor heap size
        TotalDescriptorCount = 30000
    };

private:
    void Clear();

    void CreateDeviceDependentResourcesXTK();
    void CreateWindowSizeDependentResourcesXTK();

	// Device resources.
    std::unique_ptr<UIDeviceResources>        								p_deviceResources;
	std::unique_ptr<UIEffectManager>								        p_effects;
	std::unique_ptr<UITextureManager>                                       p_textures;
    std::unique_ptr<DirectX::GraphicsMemory>                                p_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>                                p_resourceDescriptors;
    std::unique_ptr<DirectX::CommonStates>                                  p_states;

    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  p_batch;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>>  p_batchTexture;
	//std::unique_ptr<DirectX::SpriteBatch>                                   p_sprites;
    //std::unique_ptr<DirectX::SpriteFont>                                    p_font;

/*************************************************** clip rect ***************************************************/
public:
	void BeginScreenClipRect(const RECT& clipRC, bool execute = false);
	void EndScreenClipRect(bool execute = false);
	RECT GetCurrentClipRect() const;
	void ExecuteClipRect(const RECT& clipRC);
	void ResetClipRect();

private:
	std::stack<RECT> _clipRectStack;

/*************************************************** FreeType APIs ***************************************************/
public:
	// ========== 2D Screen Text ==========
	void Draw2DTextFT(const std::wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize, int renderLevel = 0);
	void Draw2DTextFT(const std::wstring& text, const RECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, int renderLevel = 0);
	void Draw2DTextMultiLineFT(const std::wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0);
	void Draw2DTextMultiLineFT(const std::wstring& text, const RECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0);

	// ========== 3D Screen Text ==========
	void Draw3DTextFT(const std::wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DTextFT(const std::wstring& text, const RECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DTextMultiLineFT(const std::wstring& text, const DirectX::XMFLOAT2& position, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DTextMultiLineFT(const std::wstring& text, const RECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	// ========== 3D World Text ==========
	void Draw3DWorldTextFT(const std::wstring& text, const DirectX::XMFLOAT3& worldPosition,
		const UIColor& color, float fontSize, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

	// ========== Utility Functions ==========
	SIZE GetTextSizeFT(const std::wstring& text, float fontSize, float lineSpacing = 1.0f);

private:
	void CreateResourcesFT();
	void ResetResourcesFT();

	bool GetCharTextureResourceFT(const wchar_t& wch, float fontSize, const UIColor& color, size_t& textureIndex);

	void Draw2DCharTextureFT(size_t textureIndex, DirectX::XMFLOAT2 position, float z, float scale, UCHAR alpha, int renderLevel = 0);
	void Draw3DCharTextureFT(size_t textureIndex, DirectX::XMFLOAT2 position, float z, float scale, UCHAR alpha, int renderLevel = 0, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DWorldCharTextureFT(size_t textureIndex, DirectX::XMFLOAT3 worldPosition, float scale, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

	DirectX::XMFLOAT2 CalculateTextPosition(const std::wstring& text, const RECT& rc, UIFontPos alignment, float fontSize, float lineSpacing = 1.0f);
	std::vector<std::wstring> SplitTextIntoLines(const std::wstring& text);

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

/*************************************************** 2D UI APIs ***************************************************/
public:
	void Draw2DPoint(const DirectX::XMFLOAT2& point, float z, const UIColor& color, float pointSize = 4, int renderLevel = 0);
	void Draw2DPoints(const std::vector<DirectX::XMFLOAT2>& points, float z, const UIColor& color, float pointSize = 4, int renderLevel = 0);

	void Draw2DLine(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0);

	void Draw2DRectOutline(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0);
	void Draw2DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, UCHAR alpha, int renderLevel = 0);
	void Draw2DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, 
						 const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel = 0);

	void Draw2DImage(const std::wstring& dllPath, UINT id, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0);
	void Draw2DImage(const std::wstring& filePath, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0);

private:
	void Draw2DImage(size_t textureIndex, 
					 RECT srcRect, DirectX::XMFLOAT2 dstStart, DirectX::XMFLOAT2 dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0);

	void Calculate2DPoint(const DirectX::XMFLOAT2& point, DirectX::XMFLOAT2& p);
	void Calculate2DLinePoints(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, DirectX::XMFLOAT2& p1, DirectX::XMFLOAT2& p2);
	void Calculate2DRectPoints(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, DirectX::XMFLOAT2& ps, DirectX::XMFLOAT2& pe);

/*************************************************** 3D UI APIs ***************************************************/
public:
	void Draw3DPoint(const DirectX::XMFLOAT2& point, float z, const UIColor& color, float pointSize=4, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DPoints(const std::vector<DirectX::XMFLOAT2>& points, float z, const UIColor& color, float pointSize=4, int renderLevel = 0,
					  const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DLine(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0,
					const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DRectOutline(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0,
						   const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z,
						 const UIColor& color, UCHAR alpha, int renderLevel = 0,
						 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DRectSolid(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float z,
             		     const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel = 0,
						 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DImage(const std::wstring& dllPath, UINT id, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DImage(const std::wstring& filePath, const UIColor& colorKey,
					 const RECT& srcRect, const DirectX::XMFLOAT2& dstStart, const DirectX::XMFLOAT2& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

private:
	void Draw3DImage(size_t textureIndex, 
					 RECT srcRect, DirectX::XMFLOAT2 dstStart, DirectX::XMFLOAT2 dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

/*************************************************** 3D World APIs ***************************************************/
// Use UICameraBase (UICameraGame, UICameraCtrl, etc.) to render 3D world objects in 3D world space
public:
	void Draw3DWorldPoint(const DirectX::XMFLOAT3& point, const UIColor& color, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const UIColor& colorS, const UIColor& colorE, float lineWidth, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const UIColor& color, float lineWidth, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldCircle(const DirectX::XMFLOAT3& center, float pixelRadius, const UIColor& color, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldTriangle(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& p3, const UIColor& color, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldTriangle(const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& p3, 
							 const UIColor& color1, const UIColor& color2, const UIColor& color3, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldTextureTriangle(const DirectX::XMFLOAT3& pLT, const DirectX::XMFLOAT3& pRT, const DirectX::XMFLOAT3& pLB,
									const DirectX::XMFLOAT2& uv1, const DirectX::XMFLOAT2& uv2, const DirectX::XMFLOAT2& uv3,
									size_t textureIndex, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldRectOutline(const DirectX::XMFLOAT3& pLT, const DirectX::XMFLOAT3& pRT, const DirectX::XMFLOAT3& pLB, const DirectX::XMFLOAT3& pRB,
								const UIColor& color, float lineWidth, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldRectSolid(const DirectX::XMFLOAT3& pLT, const DirectX::XMFLOAT3& pRT, const DirectX::XMFLOAT3& pLB, const DirectX::XMFLOAT3& pRB,
							  const UIColor& color, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldRectSolid(const DirectX::XMFLOAT3& pLT, const DirectX::XMFLOAT3& pRT, const DirectX::XMFLOAT3& pLB, const DirectX::XMFLOAT3& pRB,
							  const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

	void Draw3DWorldImage(size_t textureIndex, const RECT& srcRect, const DirectX::XMFLOAT3& pLT, const DirectX::XMFLOAT3& pRT, 
						  const DirectX::XMFLOAT3& pLB, const DirectX::XMFLOAT3& pRB, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

private:
	void Draw3DWorldLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const UIColor& colorS, const UIColor& colorE, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const UIColor& color, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldThickLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float lineWidth, const UIColor& color, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	
	float CalculateWorldLengthFromPixelLength(float pixelLength, const DirectX::XMFLOAT3& worldPosition, UICameraBase3D* pCamera);


/*************************************************** Batch Rendering System ***************************************************/
private:
	struct BatchData {
		int _batchID;														// 1: p_batch   2: p_batchTexture
		
		// as key
		int _renderLevel;													// rendering level, smaller value renders first
		DirectX::BasicEffect* _pEffect;										// p_batch & p_batchTexture
		RECT _clipRect;														// p_batch & p_batchTexture
		D3D12_GPU_DESCRIPTOR_HANDLE _srvDescriptor;							// p_batchTexture
		D3D12_GPU_DESCRIPTOR_HANDLE _samplerDescriptor;						// p_batchTexture
		UCHAR _alpha;														// p_batchTexture
		UICameraBase* _pCamera;												// p_batch & p_batchTexture

		// data
		D3D_PRIMITIVE_TOPOLOGY _topology;									// p_batch & p_batchTexture
		std::vector<std::vector<uint16_t>> _indices;						// p_batch & p_batchTexture
		std::vector<std::vector<DirectX::VertexPositionColor>> _colorVertices;    	// p_batch
		std::vector<std::vector<DirectX::VertexPositionTexture>> _textureVertices; 	// p_batchTexture

		// Helper function to compare if two BatchData have the same key
		bool IsSameKey(const BatchData& other) const {
			if (_batchID != other._batchID || 
				_renderLevel != other._renderLevel ||
				_pEffect != other._pEffect || 
				memcmp(&_clipRect, &other._clipRect, sizeof(RECT)) != 0 ||
				_pCamera != other._pCamera) {
				return false;
			}
			
			if (_batchID == 2) {
				if (_srvDescriptor.ptr != other._srvDescriptor.ptr || 
					_samplerDescriptor.ptr != other._samplerDescriptor.ptr ||
					_alpha != other._alpha) {
					return false;
				}
			}
			
			return true;
		}
	};

	// Batch registration functions
	void RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY topology, 
						   const std::vector<DirectX::VertexPositionColor>& vertices, const std::vector<uint16_t>& indices, 
						   std::unique_ptr<DirectX::BasicEffect>& effect, const RECT& clipRect, UICameraBase* pCamera, int renderLevel = 0);
	void RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY topology, 
								  const std::vector<DirectX::VertexPositionTexture>& vertices, const std::vector<uint16_t>& indices,
						   		  std::unique_ptr<DirectX::BasicEffect>& effect, 
						   		  D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor, UCHAR alpha,
						   		  const RECT& clipRect, UICameraBase* pCamera, int renderLevel = 0);

	// Batch execution functions
	void ExecuteAllBatches();
	void ExecuteColorBatch(const BatchData& batch, ID3D12GraphicsCommandList* commandList);
	void ExecuteTextureBatch(const BatchData& batch, ID3D12GraphicsCommandList* commandList);
	void UpdateBatchState(const BatchData& batch, ID3D12GraphicsCommandList* commandList);

	// Batch clear functions
	void ClearAllBatches();

	std::vector<BatchData> _batchDataList;

	UICameraBase* _pCurrentCamera = nullptr;
	DirectX::BasicEffect* _pCurrentEffect = nullptr;
};