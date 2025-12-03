#pragma once



#include "../Core/Common.h"
#include "UIUtility.h"
#include "UIElement.h"
#include "UIWindow.h"


// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


class UIGraphicsDeviceHAL {
public:
    // Graphics Device Configuration
    struct Desc {
        void* windowHandle = nullptr;
        int width = 1280;
        int height = 720;
        int backBufferCount = 2;
        bool enableVSync = true;
        bool enableHDR = false;
        bool allowTearing = false;
    };

    virtual ~UIGraphicsDeviceHAL() = default;

    // Lifecycle
    virtual bool Initialize(const Desc& desc) = 0;
    virtual void Shutdown() = 0;

    // Frame
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    // Window & Device
    virtual bool HandleWindowResize(int width, int height) = 0;
	virtual void WaitForIdle() = 0;
    virtual void HandleDeviceLost() = 0;

    // Query
    virtual const char* GetBackendName() const = 0;
    virtual UIRECT GetOutputSize() const = 0;
    virtual uint32_t GetCurrentFrameIndex() const = 0;
    virtual uint32_t GetBackBufferCount() const = 0;
};

class UIGraphicsDeviceDX12 : public UIGraphicsDeviceHAL {
	friend class UIGraphicsSystem;
	friend class UITextureManager;
	friend class UIModel;
	
/*************************************************** UIGraphicsDeviceHAL Interface ***************************************************/
public:
    // Lifecycle
    bool Initialize(const Desc& desc) override;
    void Shutdown() override;

    // Frame
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    // Window & Device
    bool HandleWindowResize(int width, int height) override;
    void WaitForIdle() override;
    void HandleDeviceLost() override;

    // Query
    const char* GetBackendName() const override { return "DirectX 12"; }
    UIRECT GetOutputSize() const override { return _outputSize; }
    uint32_t GetCurrentFrameIndex() const override { return _backBufferIndex; }
	uint32_t GetBackBufferCount() const override { return _backBufferCount; }

/*************************************************** Others ***************************************************/
protected:
    void SetWindow(int width, int height);

    UIRECT _outputSize = {0, 0, 1, 1};
    HWND _window = nullptr;

/*************************************************** DX12 ***************************************************/
public:
	ID3D12Device*               GetD3DDevice() const            { return _d3dDevice.Get(); }
	IDXGISwapChain3*            GetSwapChain() const            { return _swapChain.Get(); }
	IDXGIFactory4*              GetDXGIFactory() const          { return _dxgiFactory.Get(); }
	ID3D12CommandQueue*         GetCommandQueue() const         { return _commandQueue.Get(); }
	ID3D12GraphicsCommandList*  GetCommandList() const          { return _commandList.Get(); }
	ID3D12CommandAllocator*     GetCommandAllocator() const     { return _commandAllocators[_backBufferIndex].Get(); }	
	ID3D12Resource*             GetRenderTarget() const         { return _renderTargets[_backBufferIndex].Get(); }
	ID3D12Resource*             GetDepthStencil() const         { return _depthStencil.Get(); }
	D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const   { return _d3dFeatureLevel; }
	DXGI_FORMAT                 GetBackBufferFormat() const     { return _backBufferFormat; }
	DXGI_FORMAT                 GetDepthBufferFormat() const    { return _depthBufferFormat; }
	DXGI_COLOR_SPACE_TYPE       GetColorSpace() const           { return _colorSpace; }
	unsigned int                GetDeviceOptions() const        { return _options; }
	
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const {
		D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<INT>(_backBufferIndex) * _rtvDescriptorSize;
		return handle;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const { 
		return _dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); 
	}

protected:
	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void ReleaseResources();

	void ClearRenderTargetViews();
	void PrepareCommandList(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
	void ExecutePresent(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);

	void WaitForGpu() noexcept;
	void MoveToNextFrame();
	void GetAdapter(IDXGIAdapter1** ppAdapter);
	void UpdateColorSpace();

    static const size_t MAX_BACK_BUFFER_COUNT = 3;

    // Device & Factory
    Microsoft::WRL::ComPtr<ID3D12Device>                _d3dDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory4>               _dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain3>             _swapChain;

	// Command Objects
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          _commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   _commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      _commandAllocators[MAX_BACK_BUFFER_COUNT];

	// Render Targets
    Microsoft::WRL::ComPtr<ID3D12Resource>              _renderTargets[MAX_BACK_BUFFER_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource>              _depthStencil;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        _rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        _dsvDescriptorHeap;
    UINT                                                _rtvDescriptorSize = 0;

    // Synchronization
    Microsoft::WRL::ComPtr<ID3D12Fence>                 _fence;
    UINT64                                              _fenceValues[MAX_BACK_BUFFER_COUNT] = {};
    Microsoft::WRL::Wrappers::Event                     _fenceEvent;

	// Properties
    DXGI_FORMAT                                         _backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT                                         _depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    UINT                                                _backBufferCount = 2;
    UINT                                                _backBufferIndex = 0;
    D3D_FEATURE_LEVEL                                   _d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL                                   _d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    DWORD                                               _dxgiFactoryFlags = 0;
    DXGI_COLOR_SPACE_TYPE                               _colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    unsigned int                                        _options = 0;

/*************************************************** XTK ***************************************************/
protected:
	struct UIEffectManager {
		void Create(ID3D12Device* device, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat);
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

	void CreateDeviceDependentResourcesXTK();
	void CreateWindowSizeDependentResourcesXTK();
	void ReleaseResourcesXTK();

	UIEffectManager                                                           _effectManager;
	
	std::unique_ptr<DirectX::GraphicsMemory>                                  p_graphicsMemory;
	std::unique_ptr<DirectX::DescriptorHeap>                                  p_resourceDescriptors;
	std::unique_ptr<DirectX::CommonStates>                                    p_states;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>    p_batch;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>>  p_batchTexture;
};

class UITextureManager {
	friend class UIGraphicsSystem;

public:
	// Public interface - Get texture size
	bool Get2DImageSize(const std::wstring& imagePath, const UIColor& colorKey, UIRECT& textureRect);
	bool Get2DImageSize(const std::wstring& dllPath, UINT id, const UIColor& colorKey, UIRECT& textureRect);

	// public methods - Texture loading
	bool GetWICTextureIndexFromDLL(const std::wstring& dllPath, UINT id, const UIColor& colorKey, size_t& textureIndex);
	bool GetWICTextureIndexFromFile(const std::wstring& filePath, const UIColor& colorKey, size_t& textureIndex);
	bool GetDDSTextureIndexFromFile(const std::wstring& filePath, const UIColor& colorKey, size_t& textureIndex);

private:
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
	UIRECT Get2DTextureRect(ID3D12Resource* texture);
	UIRECT Get2DTextureRect(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture);

	// Data members - Texture resources
	struct TextureResource {
		Microsoft::WRL::ComPtr<ID3D12Resource> _texture;
		D3D12_GPU_DESCRIPTOR_HANDLE _gpuDescriptor;
	};

	std::vector<TextureResource> _textureResources;
	std::unordered_map<std::wstring, size_t> _textureResourceMap;
};

class UIGraphicsSystem : public SingletonPattern<UIGraphicsSystem> {
	friend class SingletonPattern<UIGraphicsSystem>;
	friend class UITextureManager;
	friend class UIModel;

/*************************************************** High-Level Render System API ***************************************************/
public:
    UIGraphicsSystem() noexcept(false);
    ~UIGraphicsSystem();
    
    // System Lifecycle
    bool Initialize(const UIGraphicsDeviceHAL::Desc& desc);
    void Shutdown();

    // Unified Render Interface
    void Render();

    // Window Events
    bool HandleWindowResize(int width, int height);
    void HandleDeviceLost();

    // Query
    UIRECT GetOutputSize() const;

    // Texture Query APIs (Encapsulated Interface)
    bool GetTextureSize(const std::wstring& imagePath, const UIColor& colorKey, UIRECT& textureRect) {
        return _textureManager.Get2DImageSize(imagePath, colorKey, textureRect);
    }
    bool GetTextureSize(const std::wstring& dllPath, UINT id, const UIColor& colorKey, UIRECT& textureRect) {
        return _textureManager.Get2DImageSize(dllPath, id, colorKey, textureRect);
    }

private:
    // Texture Manager Instance
    UITextureManager _textureManager;

    // HAL Backend (Composition)
    std::unique_ptr<UIGraphicsDeviceDX12> _graphicsDevice;

/*************************************************** clip rect ***************************************************/
public:
	void BeginScreenClipRect(const UIRECT& clipRC, bool execute = false);
	void EndScreenClipRect(bool execute = false);
	UIRECT GetCurrentClipRect() const;
	void ExecuteClipRect(const UIRECT& clipRC);
	void ResetClipRect();

private:
	std::stack<UIRECT> _clipRectStack;

/*************************************************** FreeType APIs ***************************************************/
public:
	// ========== 2D Screen Text ==========
	void Draw2DTextFT(const std::wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, int renderLevel = 0);
	void Draw2DTextFT(const std::wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, int renderLevel = 0);
	void Draw2DTextMultiLineFT(const std::wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0);
	void Draw2DTextMultiLineFT(const std::wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0);

	// ========== 3D Screen Text ==========
	void Draw3DTextFT(const std::wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DTextFT(const std::wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DTextMultiLineFT(const std::wstring& text, const UIVector2F& position, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DTextMultiLineFT(const std::wstring& text, const UIRECT& rc, UIFontPos alignment, float z, const UIColor& color, float fontSize, float lineSpacing = 1.2f, int renderLevel = 0,
		const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	// ========== 3D World Text ==========
	void Draw3DWorldTextFT(const std::wstring& text, const UIVector3F& worldPosition,
		const UIColor& color, float fontSize, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

	// ========== Utility Functions ==========
	UISIZE GetTextSizeFT(const std::wstring& text, float fontSize, float lineSpacing = 1.0f);

private:
	void CreateResourcesFT();
	void ResetResourcesFT();

	bool GetCharTextureResourceFT(const wchar_t& wch, float fontSize, const UIColor& color, size_t& textureIndex);

	void Draw2DCharTextureFT(size_t textureIndex, UIVector2F position, float z, float scale, UCHAR alpha, int renderLevel = 0);
	void Draw3DCharTextureFT(size_t textureIndex, UIVector2F position, float z, float scale, UCHAR alpha, int renderLevel = 0, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DWorldCharTextureFT(size_t textureIndex, UIVector3F worldPosition, float scale, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

	UIVector2F CalculateTextPosition(const std::wstring& text, const UIRECT& rc, UIFontPos alignment, float fontSize, float lineSpacing = 1.0f);
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
		Microsoft::WRL::ComPtr<ID3D12Resource> _texture;
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
	void Draw2DPoint(const UIVector2F& point, float z, const UIColor& color, float pointSize = 4, int renderLevel = 0);
	void Draw2DPoints(const std::vector<UIVector2F>& points, float z, const UIColor& color, float pointSize = 4, int renderLevel = 0);

	void Draw2DLine(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0);

	void Draw2DRectOutline(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0);
	void Draw2DRectSolid(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, UCHAR alpha, int renderLevel = 0);
	void Draw2DRectSolid(const UIVector2F& start, const UIVector2F& end, float z, 
						 const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel = 0);

	void Draw2DImage(const std::wstring& dllPath, UINT id, const UIColor& colorKey,
					 const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0);
	void Draw2DImage(const std::wstring& filePath, const UIColor& colorKey,
					 const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0);

private:
	void Draw2DImage(size_t textureIndex, 
					 UIRECT srcRect, UIVector2F dstStart, UIVector2F dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0);

	void Calculate2DPoint(const UIVector2F& point, UIVector2F& p);
	void Calculate2DLinePoints(const UIVector2F& start, const UIVector2F& end, UIVector2F& p1, UIVector2F& p2);
	void Calculate2DRectPoints(const UIVector2F& start, const UIVector2F& end, UIVector2F& ps, UIVector2F& pe);

/*************************************************** 3D UI APIs ***************************************************/
public:
	void Draw3DPoint(const UIVector2F& point, float z, const UIColor& color, float pointSize=4, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DPoints(const std::vector<UIVector2F>& points, float z, const UIColor& color, float pointSize=4, int renderLevel = 0,
					  const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DLine(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0,
					const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DRectOutline(const UIVector2F& start, const UIVector2F& end, float z, const UIColor& color, float lineWidth = 1.0f, int renderLevel = 0,
						   const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DRectSolid(const UIVector2F& start, const UIVector2F& end, float z,
						 const UIColor& color, UCHAR alpha, int renderLevel = 0,
						 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DRectSolid(const UIVector2F& start, const UIVector2F& end, float z,
             		     const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel = 0,
						 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void Draw3DImage(const std::wstring& dllPath, UINT id, const UIColor& colorKey,
					 const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void Draw3DImage(const std::wstring& filePath, const UIColor& colorKey,
					 const UIRECT& srcRect, const UIVector2F& dstStart, const UIVector2F& dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

private:
	void Draw3DImage(size_t textureIndex, 
					 UIRECT srcRect, UIVector2F dstStart, UIVector2F dstEnd, 
					 float z, UCHAR alpha, int renderLevel = 0,
					 const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

/*************************************************** 3D World APIs ***************************************************/
// Use UICameraBase (UICameraGame, UICameraCtrl, etc.) to render 3D world objects in 3D world space
public:
	void Draw3DWorldPoint(const UIVector3F& point, const UIColor& color, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& colorS, const UIColor& colorE, float lineWidth, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& color, float lineWidth, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldCircle(const UIVector3F& center, float pixelRadius, const UIColor& color, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldTriangle(const UIVector3F& p1, const UIVector3F& p2, const UIVector3F& p3, const UIColor& color, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldTriangle(const UIVector3F& p1, const UIVector3F& p2, const UIVector3F& p3, 
							 const UIColor& color1, const UIColor& color2, const UIColor& color3, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldTextureTriangle(const UIVector3F& pLT, const UIVector3F& pRT, const UIVector3F& pLB,
									const DirectX::XMFLOAT2& uv1, const DirectX::XMFLOAT2& uv2, const DirectX::XMFLOAT2& uv3,
									size_t textureIndex, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldRectOutline(const UIVector3F& pLT, const UIVector3F& pRT, const UIVector3F& pLB, const UIVector3F& pRB,
								const UIColor& color, float lineWidth, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldRectSolid(const UIVector3F& pLT, const UIVector3F& pRT, const UIVector3F& pLB, const UIVector3F& pRB,
							  const UIColor& color, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldRectSolid(const UIVector3F& pLT, const UIVector3F& pRT, const UIVector3F& pLB, const UIVector3F& pRB,
							  const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

	void Draw3DWorldImage(size_t textureIndex, const UIRECT& srcRect, const UIVector3F& pLT, const UIVector3F& pRT, 
						  const UIVector3F& pLB, const UIVector3F& pRB, UCHAR alpha, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);

private:
	void Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& colorS, const UIColor& colorE, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldLine(const UIVector3F& start, const UIVector3F& end, const UIColor& color, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	void Draw3DWorldThickLine(const UIVector3F& start, const UIVector3F& end, float lineWidth, const UIColor& color, int renderLevel = 0, UICameraBase3D* pCamera = nullptr);
	
	float CalculateWorldLengthFromPixelLength(float pixelLength, const UIVector3F& worldPosition, UICameraBase3D* pCamera);


/*************************************************** Batch Rendering System ***************************************************/
private:
	struct BatchData {
		int _batchID;														// 1: p_batch   2: p_batchTexture
		
		// as key
		int _renderLevel;													// rendering level, smaller value renders first
		DirectX::BasicEffect* _pEffect;										// p_batch & p_batchTexture
		UIRECT _clipRect;														// p_batch & p_batchTexture
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
				memcmp(&_clipRect, &other._clipRect, sizeof(UIRECT)) != 0 ||
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
						   std::unique_ptr<DirectX::BasicEffect>& effect, const UIRECT& clipRect, UICameraBase* pCamera, int renderLevel = 0);
	void RegisterBatchTextureData(D3D_PRIMITIVE_TOPOLOGY topology, 
								  const std::vector<DirectX::VertexPositionTexture>& vertices, const std::vector<uint16_t>& indices,
						   		  std::unique_ptr<DirectX::BasicEffect>& effect, 
						   		  D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor, UCHAR alpha,
						   		  const UIRECT& clipRect, UICameraBase* pCamera, int renderLevel = 0);

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