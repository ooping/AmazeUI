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
	UIPoint(LONG x, LONG y, float z=0.5);
	~UIPoint() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _point;
	float _z;
};

// point set
struct UIPoints {
	UIPoints(const std::vector<POINT>& points, float z=0.5);
	~UIPoints() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	std::vector<DirectX::XMFLOAT2> _points;
	float _z;
};

// line
struct UILine {
	UILine(LONG beginX, LONG beginY, LONG endX, LONG endY, float z, float width = 1.0f);
	~UILine() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _start, _end;
	float _z, _width;
};

// Rectangle
struct UIRect {
	UIRect(LONG beginX, LONG beginY, LONG endX, LONG endY, float z);
	UIRect(const RECT& rect, float z);
	~UIRect() = default;
	
	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());					// draw rectangle outline
	void operator()(const UIColor& color, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());		// draw rectangle solid color with alpha
	void operator()(const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _start, _end;
	float _z;
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
	UIImage(std::wstring imagePath, const UIColor& colorKey, float z);
	UIImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, float z);
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
	UISlicedImage(std::wstring imagePath, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z);
	UISlicedImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z);
	~UISlicedImage() = default;
	void operator()(LONG dstBeginX, LONG dstBeginY, LONG width, LONG height, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(const RECT& dstRC, UCHAR alphy = 255, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

private:
	UIImage _image;

	int _topBarHeight;
	int _bottomBarHeight;
	int _leftBarWidth;
	int _rightBarWidth;
};


struct UIFont {
	UIFont(float z, float fontSize);
	~UIFont() = default;

	SIZE GetDrawAreaSize(std::wstring str);

	void operator()(std::wstring text, const POINT& postion, const UIColor& color = UIColor::Black, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(std::wstring text, const RECT& rc, const UIColor& color = UIColor::Black, int posFlag = HLEFT_VCENTER, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	void operator()(std::wstring text, const POINT& postion, const UIColor& color, float lineSpacing, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());
	void operator()(std::wstring text, const RECT& rc, const UIColor& color, int posFlag, float lineSpacing, const DirectX::XMMATRIX& transformMatrix = DirectX::XMMatrixIdentity());

	enum FontPos {
		HLEFT_VTOP = 0,
		HLEFT_VCENTER = 0x04,
		HCENTER_VCENTER = 0x01|0x04,
		HRIGHT_VCENTER = 0x02|0x04,
		HCENTER_VTOP = 0x01,
	};

	float _z;
	float _fontSize;
};


// point
class UICameraBase3D;
struct UIPoint3D {
	UIPoint3D(float x, float y, float z);
	~UIPoint3D() = default;

	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _point;
};

// point set
struct UIPoints3D {
	UIPoints3D(const std::vector<UIPointFloat3>& points);
	~UIPoints3D() = default;

	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	std::vector<DirectX::XMFLOAT3> _points;
};

struct UILine3D {
	UILine3D(UIPointFloat3 start, UIPointFloat3 end, float width = 1.0f);
	~UILine3D() = default;

	void operator()(const UIColor& colorS, const UIColor& colorE, UICameraBase3D* pCamera);
	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _start, _end;
	float _width;
};

struct UICircle3D {
	UICircle3D(UIPointFloat3 center, float radius);
	~UICircle3D() = default;

	void operator()(const UIColor& color, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _center;
	float _radius;
};

struct UITriangle3D {
	UITriangle3D(UIPointFloat3 p1, UIPointFloat3 p2, UIPointFloat3 p3);
	~UITriangle3D() = default;

	void operator()(const UIColor& color1, const UIColor& color2, const UIColor& color3, UCHAR alpha, UICameraBase3D* pCamera);
	void operator()(const UIColor& color, UCHAR alpha, UICameraBase3D* pCamera);

	DirectX::XMFLOAT3 _p1, _p2, _p3;
};

struct UIFont3D {
	UIFont3D(float fontSize);
	~UIFont3D() = default;

	void operator()(std::wstring text, const UIPointFloat3& position, const UIColor& color, UICameraBase3D* pCamera);

	float _fontSize;
};
