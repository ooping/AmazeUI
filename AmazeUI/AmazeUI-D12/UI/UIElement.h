#pragma once

#include "UIUtility.h"

// clipping region RAII handler
struct UIScreenClipRectGuard {
	UIScreenClipRectGuard(const RECT& clipRC);
	~UIScreenClipRectGuard();

private:
	UIScreenClipRectGuard() = delete;	
	UIScreenClipRectGuard(const UIScreenClipRectGuard& other) = delete;
	UIScreenClipRectGuard& operator=(const UIScreenClipRectGuard& other) = delete;
};

// point
struct UIPoint {
	UIPoint(LONG x, LONG y, float z=0.5);
	~UIPoint() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _point;
	float _z;
};

// point set
struct UIPoints {
	UIPoints(const std::vector<POINT>& points, float z=0.5);
	~UIPoints() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

	std::vector<DirectX::XMFLOAT2> _points;
	float _z;
};

// line
struct UILine {
	UILine(LONG beginX, LONG beginY, LONG endX, LONG endY, float z, float width=1.0f);
	~UILine() = default;

	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

	DirectX::XMFLOAT2 _start, _end;
	float _z, _width;
};

// Rectangle
struct UIRect {
	UIRect(LONG beginX, LONG beginY, LONG endX, LONG endY, float z);
	UIRect(const RECT& rect, float z);
	~UIRect() = default;
	
	void operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());					// draw rectangle outline
	void operator()(const UIColor& color, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());		// draw rectangle solid color with alpha
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
	void operator()(const RECT& srcRect, const RECT& dstRect, UCHAR alphy=255, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void operator()(const RECT& srcRect, LONG dstBeginX, LONG dstBeginY, UCHAR alphy=255, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void operator()(const RECT& dstRect, float scale=1.f, UCHAR alphy=255, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void operator()(LONG dstCenterX, LONG dstCenterY, float scale=1.f, UCHAR alphy=255, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

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
	void operator()(LONG dstBeginX, LONG dstBeginY, LONG width, LONG height, UCHAR alphy=255, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void operator()(const RECT& dstRC, UCHAR alphy=255, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

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

	void operator()(std::wstring text, const POINT& postion, const UIColor& color=UIColor::Black, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void operator()(std::wstring text, const RECT& rc, const UIColor& color=UIColor::Black, int posFlag=HLEFT_VCENTER, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

	SIZE GetDrawAreaSize(std::wstring str);

	enum FontPos {
		HLEFT_VTOP = 0,
		HLEFT_VCENTER = 0x04,
		HCENTER_VCENTER = 0x01|0x04,
		HRIGHT_VCENTER = 0x02|0x04,
	};

	float _z;
	float _fontSize;
};




class UICameraBase {
public:
	// Update camera matrices
	void SetViewMatrix();
	void SetProjectionMatrix();	
	// Matrix access
	const DirectX::SimpleMath::Matrix& GetViewMatrix() const { return _view; }
	const DirectX::SimpleMath::Matrix& GetProjectionMatrix() const { return _projection3D; }

	// Camera properties
	DirectX::XMFLOAT3 GetPosition() const { return _position; }
	DirectX::XMFLOAT3 GetTarget() const { return _target; }
	DirectX::XMFLOAT3 GetUpVector() const { return _up; }
	DirectX::XMFLOAT3 GetRightVector() const { return _right; }
	DirectX::XMFLOAT3 GetForwardVector() const { return _forward; }

protected:
	UICameraBase() = default;
	~UICameraBase() = default;

	// Camera vectors
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _target;
	DirectX::XMFLOAT3 _up;
	DirectX::XMFLOAT3 _right;
	DirectX::XMFLOAT3 _forward;

	// Camera properties
	float _fov = DirectX::XM_PIDIV2;
	float _aspectRatio = 1.0f;
	float _nearPlane = 0.01f;
	float _farPlane = 1000.0f;

	// Matrices
	DirectX::SimpleMath::Matrix _view;
    DirectX::SimpleMath::Matrix _projection3D;
};

class UICameraUI : public UICameraBase, public SingletonPattern<UICameraUI> {
	friend class SingletonPattern<UICameraUI>;

public:
	// Setup methods
	void SetCameraFor2D(float width, float height);

	// Convert screen 2D to 3D
	DirectX::XMFLOAT3 ConvertScreen2DTo3D(const DirectX::XMFLOAT3& screenPos);
	DirectX::XMFLOAT3 Convert3DToScreen2D(const DirectX::XMFLOAT3& viewPos);

private:
	UICameraUI() {
		_position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		_target = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		_up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
		_right = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
		_forward = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);

		_farPlane = 10000.0f;
	}
	~UICameraUI() = default;
};

class UICameraGame : public UICameraBase, public SingletonPattern<UICameraGame> {
	friend class SingletonPattern<UICameraGame>;

public:
	// Setup methods
	void SetCamera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);
	void SetAspectRatioAndProjectionMatrix(float aspectRatio);

private:
	UICameraGame() {
		_position = DirectX::XMFLOAT3(0.0f, 2.0f, -5.0f);
		_target = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
		_up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
		_right = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
		_forward = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);

		_farPlane = 1000.0f;
	}
	~UICameraGame() = default;
};


struct UIZPlaneTransform {
    static DirectX::XMMATRIX GetTransformMatrix(bool isRotationZ, float xByZ, float yByZ, float zAngle,
												bool isRotationX, float yByX, float xAngle,
												bool isRotationY, float xByY, float yAngle,
												float z);

	static void TransformPoint(const DirectX::XMMATRIX& transform, const DirectX::XMFLOAT2& point, float z, DirectX::XMFLOAT3& wp);
	static void TransformPoints(const DirectX::XMMATRIX& transform, const std::vector<DirectX::XMFLOAT2>& points, float z, std::vector<DirectX::XMFLOAT3>& wps);
	static void TransformLinePoints(const DirectX::XMMATRIX& transform, const DirectX::XMFLOAT2& ps, const DirectX::XMFLOAT2& pe, float z, std::vector<DirectX::XMFLOAT3>& wps);
	static void TransformRectPoints(const DirectX::XMMATRIX& transform, const DirectX::XMFLOAT2& ps, const DirectX::XMFLOAT2& pe, float z, std::vector<DirectX::XMFLOAT3>& wps);
};

// 3D rect rotation
// the rect is the same z plane
// the ratotion axis of x/y is in the same z plane
struct UI3DRotation {
	void SetRotationZ(bool isRotationZ, LONG xbyZ=0, LONG ybyZ=0, float zAngle=0.f);
	void SetRotationX(bool isRotationX, LONG yByx=0, float xAngle=0.f);
	void SetRotationY(bool isRotationY, LONG xByY=0, float yAngle=0.f);	

	bool _isRotationZ = false;
	DirectX::XMFLOAT2 _XYByZ;
	float _zAngle;

	bool _isRotationX = false;
	bool _isRotationY = false;
	DirectX::XMFLOAT2 _XY;
	float _xAngle;
	float _yAngle;
};
