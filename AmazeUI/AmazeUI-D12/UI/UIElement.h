#pragma once

#include "UIUtility.h"
#include "UICamera.h"

// clipping region RAII handler
struct UIScreenClipRectGuard {
	UIScreenClipRectGuard(const RECT& clipRC, bool execute = false);
	~UIScreenClipRectGuard();

private:
	UIScreenClipRectGuard() = delete;	
	UIScreenClipRectGuard(const UIScreenClipRectGuard& other) = delete;
	UIScreenClipRectGuard& operator=(const UIScreenClipRectGuard& other) = delete;

	bool _execute = false;
};

// point
struct UIPoint {
	UIPoint(LONG x, LONG y, float z = 0.5, int renderLevel = 0);
	~UIPoint() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _point;
	float _z;
	int _renderLevel;
};

// point set
struct UIPoints {
	UIPoints(const std::vector<POINT>& points, float z = 0.5, int renderLevel = 0);
	~UIPoints() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	std::vector<DirectX::XMFLOAT2> _points;
	float _z;
	int _renderLevel;
};

// line
struct UILine {
	UILine(LONG beginX, LONG beginY, LONG endX, LONG endY, float z, float width = 1.0f, int renderLevel = 0);
	~UILine() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _start, _end;
	float _z, _width;
	int _renderLevel;
};

// Rectangle
struct UIRect {
	UIRect(LONG beginX, LONG beginY, LONG endX, LONG endY, float z, int renderLevel = 0);
	UIRect(const RECT& rect, float z, int renderLevel = 0);
	~UIRect() = default;
	
	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());					// draw rectangle outline
	void operator()(const UIColor& color, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());		// draw rectangle solid color with alpha
	void operator()(const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _start, _end;
	float _z;
	int _renderLevel;
};

//	Image
/*
    How to embed images into DLL:
    1. If the image is in BMP format, it can be imported directly and will appear under "Bitmap" type in the resource explorer
    2. For other formats, first import with type RCDATA.
       Then open the .rc file and modify the type to "RCDATA DISCARDABLE".
       Example: IDR_JPG1    RCDATA DISCARDABLE    "texture.JPG"
*/
struct UIImage {
	UIImage();
	UIImage(std::wstring imagePath, const UIColor& colorKey, float z, int renderLevel = 0);
	UIImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, float z, int renderLevel = 0);
	~UIImage() = default;
	// 
	void operator()(const RECT& srcRect, const RECT& dstRect, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(const RECT& srcRect, LONG dstBeginX, LONG dstBeginY, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(const RECT& dstRect, float scale=1.f, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(LONG dstCenterX, LONG dstCenterY, float scale=1.f, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	bool GetSize(RECT& textureRect);

	// 
	int _sourceFlag;	// 1: imagePath, 2: resDLLPath
	std::wstring _path;
	UINT _id;
	float _z;
	UIColor _colorKey;
	int _renderLevel;
};

/*
	9-slice scalable UI border image renderer
	It was used to draw scalable UI border images, with the border areas remaining the original size and the middle area adapting to the scale.
	---------------------------------
	| 1 |			2			| 3 |
	|--------------------------------
	|	|						|	|
	|	|						|	|
	|	|						|	|
	| 7	|			9			| 8	|
	|	|						|	|
	|	|						|	|
	|	|						|	|
	|--------------------------------
	| 4	|			5			| 6 |									
	---------------------------------
	When drawing, it is divided into 9 areas
	Number	High									Width
	1		topBarHeight							leftBarWidth
	2		topBarHeight							width-leftBarWidth-rightBarWidth
	3		topBarHeight							rightBarWidth
	4		bottomBarHeight							leftBarWidth
	5		bottomBarHeight							width-leftBarWidth-rightBarWidth
	6		bottomBarHeight							rightBarWidth
	7		height-topBarHeight-bottomBarHeight		leftBarWidth
	8		height-topBarHeight-bottomBarHeight		rightBarWidth
	9		height-topBarHeight-bottomBarHeight		width-leftBarWidth-rightBarWidth
*/
class UISlicedImage {
public:
	UISlicedImage();
	UISlicedImage(std::wstring imagePath, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z, int renderLevel = 0);
	UISlicedImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z, int renderLevel = 0);
	~UISlicedImage() = default;
	void operator()(LONG dstBeginX, LONG dstBeginY, LONG width, LONG height, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(const RECT& dstRC, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

private:
	UIImage _image;

	int _topBarHeight;
	int _bottomBarHeight;
	int _leftBarWidth;
	int _rightBarWidth;
	int _renderLevel;
};

// Text alignment flags for FreeType text rendering
enum class UIFontPos : uint8_t {
	// Horizontal alignment (lower 2 bits)
	HLeft = 0b00,
	HCenter = 0b01,
	HRight = 0b10,

	// Vertical alignment (upper 2 bits)
	VTop = 0b0000,
	VMiddle = 0b0100,
	VBottom = 0b1000,

	// Combined constants
	TopLeft = VTop | HLeft,
	TopCenter = VTop | HCenter,
	TopRight = VTop | HRight,
	MiddleLeft = VMiddle | HLeft,
	MiddleCenter = VMiddle | HCenter,
	MiddleRight = VMiddle | HRight,
	BottomLeft = VBottom | HLeft,
	BottomCenter = VBottom | HCenter,
	BottomRight = VBottom | HRight
};

struct UIFont {
	UIFont(float z, float fontSize, int renderLevel = 0);
	~UIFont() = default;

	SIZE GetDrawAreaSize(std::wstring str);

	void operator()(std::wstring text, const POINT& postion, const UIColor& color = UIColor::Black, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(std::wstring text, const RECT& rc, const UIColor& color = UIColor::Black, UIFontPos posFlag = UIFontPos::MiddleLeft, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void operator()(std::wstring text, const POINT& postion, const UIColor& color, float lineSpacing, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(std::wstring text, const RECT& rc, const UIColor& color, UIFontPos posFlag, float lineSpacing, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	float _z;
	float _fontSize;
	int _renderLevel;
};


// point
class UICameraBase3D;
struct UIPoint3D {
	UIPoint3D(float x, float y, float z, int renderLevel = 0);
	~UIPoint3D() = default;

	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _point;
	int _renderLevel;
};

// point set
struct UIPoints3D {
	UIPoints3D(const std::vector<UIPointFloat3>& points, int renderLevel = 0);
	~UIPoints3D() = default;

	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	std::vector<DirectX::XMFLOAT3> _points;
	int _renderLevel;
};

struct UILine3D {
	UILine3D(UIPointFloat3 start, UIPointFloat3 end, float width = 1.0f, int renderLevel = 0);
	~UILine3D() = default;

	void operator()(const UIColor& colorS, const UIColor& colorE, UICameraBase3D* pCamera);
	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _start, _end;
	float _width;
	int _renderLevel;
};

struct UICircle3D {
	UICircle3D(UIPointFloat3 center, float radius, int renderLevel = 0);
	~UICircle3D() = default;

	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _center;
	float _radius;
	int _renderLevel;
};

struct UITriangle3D {
	UITriangle3D(UIPointFloat3 p1, UIPointFloat3 p2, UIPointFloat3 p3, int renderLevel = 0);
	~UITriangle3D() = default;

	void operator()(const UIColor& color1, const UIColor& color2, const UIColor& color3, UCHAR alpha, UICameraBase3D* pCamera);
	void operator()(const UIColor& color, UCHAR alpha, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _p1, _p2, _p3;
	int _renderLevel;
};

struct UIFont3D {
	UIFont3D(float fontSize, int renderLevel = 0);
	~UIFont3D() = default;

	void operator()(std::wstring text, const UIPointFloat3& position, const UIColor& color, UICameraBase3D* pCamera);

	float _fontSize;
	int _renderLevel;
};
