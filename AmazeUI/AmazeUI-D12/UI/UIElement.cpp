#include "UIElement.h"
#include "UIDXFoundation.h"
using namespace std;
using namespace DirectX;
using namespace Shape2D;
using namespace SimpleMath;

UIScreenClipRectGuard::UIScreenClipRectGuard(const RECT& clipRC) {
	UIDXFoundation::GetSingletonInstance()->BeginScreenClipRect(clipRC);
}

UIScreenClipRectGuard::~UIScreenClipRectGuard() {
	UIDXFoundation::GetSingletonInstance()->EndScreenClipRect();
}

UIPoint::UIPoint(LONG x, LONG y, float z) {
	_point = XMFLOAT2((float)x, (float)y);
	_z = z;
}

void UIPoint::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DPoint(_point, _z, color, 4, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DPoint(_point, _z, color);
	}
}

UIPoints::UIPoints(const vector<POINT>& points, float z) {
	_points.resize(points.size());
	for (size_t i = 0; i < points.size(); i++) {
		_points[i] = XMFLOAT2((float)points[i].x, (float)points[i].y);
	}
	_z = z;
}

void UIPoints::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {	
		UIDXFoundation::GetSingletonInstance()->Draw3DPoints(_points, _z, color, 4.f, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DPoints(_points, _z, color);
	}
}

UILine::UILine(LONG beginX, LONG beginY, LONG endX, LONG endY, float z, float width) {
	_start = XMFLOAT2((float)beginX, (float)beginY);
	_end = XMFLOAT2((float)endX, (float)endY);
	_z = z;
	_width = width;
}

void UILine::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DLine(_start, _end, _z, color, _width, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DLine(_start, _end, _z, color, _width);
	}
}

UIRect::UIRect(LONG beginX, LONG beginY, LONG endX, LONG endY, float z) {
	_start = XMFLOAT2((float)beginX, (float)beginY);
	_end = XMFLOAT2((float)endX, (float)endY);
	_z = z;
}

UIRect::UIRect(const RECT& rect, float z) {
	_start = XMFLOAT2((float)rect.left, (float)rect.top);
	_end = XMFLOAT2((float)(rect.right), (float)(rect.bottom));
	_z = z;
}

void UIRect::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DRectOutline(_start, _end, _z, color, 1.f, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DRectOutline(_start, _end, _z, color);
	}
}

void UIRect::operator()(const UIColor& color, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DRectSolid(_start, _end, _z, color, alpha, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DRectSolid(_start, _end, _z, color, alpha);
	}
}

void UIRect::operator()(const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DRectSolid(_start, _end, _z, colorLT, colorRT, colorLB, colorRB, alpha, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DRectSolid(_start, _end, _z, colorLT, colorRT, colorLB, colorRB, alpha);
	}
}

UIImage::UIImage() {
	_sourceFlag = 0;
}

UIImage::UIImage(std::wstring imagePath, const UIColor& colorKey, float z) {
	_sourceFlag = 1;
	_path = imagePath;
	_z = z;
	_colorKey = colorKey;
}

UIImage::UIImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, float z) {
	_sourceFlag = 2;
	_path = resDLLPath;
	_id = id;
	_z = z;
	_colorKey = colorKey;
}

void UIImage::operator()(const RECT& srcRect, const RECT& dstRect, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		if (_sourceFlag==1)	{
			UIDXFoundation::GetSingletonInstance()->Draw3DImage(_path, _colorKey, srcRect,
														XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
														XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
														_z, alphy, transformMatrix);
		} else if (_sourceFlag==2) {
			UIDXFoundation::GetSingletonInstance()->Draw3DImage(_path, _id, _colorKey, srcRect,
														XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
														XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
														_z, alphy, transformMatrix);
		}
	} else {
		if (_sourceFlag==1)	{
			UIDXFoundation::GetSingletonInstance()->Draw2DImage(_path, _colorKey, srcRect,
														XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
														XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
														_z, alphy);
		} else if (_sourceFlag==2) {
			UIDXFoundation::GetSingletonInstance()->Draw2DImage(_path, _id, _colorKey, srcRect,
														XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
														XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
														_z, alphy);
		}
	}
}

void UIImage::operator()(const RECT& srcRect, LONG dstBeginX, LONG dstBeginY, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {	
	RECT dstRect = {dstBeginX, dstBeginY, 0, 0};
	this->operator()(srcRect, dstRect, alphy, transformMatrix);
}

void UIImage::operator()(const RECT& dstRect, float scale, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	this->operator()(NULL_RECT, scale==1 ? dstRect : ScaleRect()(dstRect, scale), alphy, transformMatrix);
}


void UIImage::operator()(LONG dstCenterX, LONG dstCenterY, float scale, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	RECT textureRect;
	if (!GetSize(textureRect)) {
		return;
	}

	RECT dstRect = CreateRect()(Shape2D::CreatePoint()((LONG)(dstCenterX-textureRect.right*scale/2), (LONG)(dstCenterY-textureRect.bottom*scale/2)), 
										 CreateSize()((LONG)((textureRect.right-textureRect.left)*scale), (LONG)((textureRect.bottom-textureRect.top)*scale)));
	this->operator()(NULL_RECT, dstRect, alphy, transformMatrix);
}


bool UIImage::GetSize(RECT& textureRect) {
	if (_sourceFlag==1) {
		return UIDXFoundation::GetSingletonInstance()->Get2DImageSize(_path, _colorKey, textureRect);
	} else if (_sourceFlag==2) {
		return UIDXFoundation::GetSingletonInstance()->Get2DImageSize(_path, _id, _colorKey, textureRect);
	}
	return false;
}

UISlicedImage::UISlicedImage() {
	_image._sourceFlag = 0;
}

UISlicedImage::UISlicedImage(std::wstring imagePath, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z) {
	_image = UIImage(imagePath, colorKey, z);

	_topBarHeight = topBarHeight;
	_bottomBarHeight = bottomBarHeight;
	_leftBarWidth = leftBarWidth;
	_rightBarWidth = rightBarWidth;
}

UISlicedImage::UISlicedImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z) {
	_image = UIImage(resDLLPath, id, colorKey, z);

	_topBarHeight = topBarHeight;
	_bottomBarHeight = bottomBarHeight;
	_leftBarWidth = leftBarWidth;
	_rightBarWidth = rightBarWidth;
}

void UISlicedImage::operator()(LONG dstBeginX, LONG dstBeginY, LONG width, LONG height, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	RECT textureRect;
	if( !_image.GetSize(textureRect) ) {
		return;
	}
	UINT imageWidth = GetRectWidth()(textureRect);
	UINT imageHeight = GetRectHeight()(textureRect);

	RECT rc1 = CreateRect()(Shape2D::CreatePoint()(0, 0), CreateSize()(_leftBarWidth, _topBarHeight));
	_image(rc1, dstBeginX, dstBeginY, alphy, transformMatrix);

	RECT rc3 = CreateRect()(Shape2D::CreatePoint()(imageWidth-_rightBarWidth , 0), CreateSize()(_rightBarWidth, _topBarHeight));
	_image(rc3, dstBeginX+width-_rightBarWidth, dstBeginY, alphy, transformMatrix);

	RECT rc4 = CreateRect()(Shape2D::CreatePoint()(0, imageHeight-_bottomBarHeight), CreateSize()(_leftBarWidth, _bottomBarHeight));
	_image(rc4, dstBeginX, dstBeginY+height-_bottomBarHeight, alphy, transformMatrix);

	RECT rc6 = CreateRect()(Shape2D::CreatePoint()(imageWidth-_rightBarWidth, imageHeight-_bottomBarHeight), CreateSize()(_rightBarWidth, _bottomBarHeight));
	_image(rc6, dstBeginX+width-_rightBarWidth, dstBeginY+height-_bottomBarHeight, alphy, transformMatrix);

	RECT rc2 = CreateRect()(Shape2D::CreatePoint()(_leftBarWidth, 0) , CreateSize()(imageWidth-_leftBarWidth-_rightBarWidth, _topBarHeight));
	RECT rc2d = CreateRect()(Shape2D::CreatePoint()(dstBeginX+_leftBarWidth, dstBeginY), CreateSize()(width-_leftBarWidth-_rightBarWidth, _topBarHeight));
	_image(rc2, rc2d, alphy, transformMatrix);

	RECT rc5 = CreateRect()(Shape2D::CreatePoint()(_leftBarWidth, imageHeight-_bottomBarHeight), CreateSize()(imageWidth-_leftBarWidth-_rightBarWidth, _bottomBarHeight));
	RECT rc5d = CreateRect()(Shape2D::CreatePoint()(dstBeginX+_leftBarWidth, dstBeginY+height-_bottomBarHeight), CreateSize()(width-_leftBarWidth-_rightBarWidth, _bottomBarHeight));
	_image(rc5, rc5d, alphy, transformMatrix);

	RECT rc7 = CreateRect()(Shape2D::CreatePoint()(0, _topBarHeight), CreateSize()(_leftBarWidth, imageHeight-_topBarHeight-_bottomBarHeight));
	RECT rc7d = CreateRect()(Shape2D::CreatePoint()(dstBeginX, dstBeginY+_topBarHeight), CreateSize()(_leftBarWidth, height-_topBarHeight-_bottomBarHeight));
	_image(rc7, rc7d, alphy, transformMatrix);

	RECT rc8 = CreateRect()(Shape2D::CreatePoint()(imageWidth-_rightBarWidth, _topBarHeight), CreateSize()(_rightBarWidth, imageHeight-_topBarHeight-_bottomBarHeight));
	RECT rc8d = CreateRect()(Shape2D::CreatePoint()(dstBeginX+width-_rightBarWidth, dstBeginY+_topBarHeight), CreateSize()(_rightBarWidth, height-_topBarHeight-_bottomBarHeight));
	_image(rc8, rc8d, alphy, transformMatrix);
	
	RECT rc9 = CreateRect()(Shape2D::CreatePoint()(_leftBarWidth, _topBarHeight), CreateSize()(imageWidth-_leftBarWidth-_rightBarWidth, imageHeight-_topBarHeight-_bottomBarHeight));
	RECT rc9d = CreateRect()(Shape2D::CreatePoint()(dstBeginX+_leftBarWidth, dstBeginY+_topBarHeight), CreateSize()(width-_leftBarWidth-_rightBarWidth, height-_topBarHeight-_bottomBarHeight));
	_image(rc9, rc9d, alphy, transformMatrix);
}

void UISlicedImage::operator()(const RECT& dstRC, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	this->operator()(dstRC.left, dstRC.top, dstRC.right-dstRC.left, dstRC.bottom-dstRC.top, alphy, transformMatrix);
}

UIFont::UIFont(float z, float fontSize) {
	_z = z;
	_fontSize = fontSize;
}

void UIFont::operator()(std::wstring text, const POINT& position, const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextFT(text, XMFLOAT2{(float)position.x, (float)position.y}, _z, color, _fontSize, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextFT(text, XMFLOAT2{(float)position.x, (float)position.y}, _z, color, _fontSize);
	}
}

void UIFont::operator()(std::wstring text, const RECT& rc, const UIColor& color, int posFlag, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextFT(text, rc, posFlag, _z, color, _fontSize, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextFT(text, rc, posFlag, _z, color, _fontSize);
	}
}

SIZE UIFont::GetDrawAreaSize(std::wstring text) {
	return UIDXFoundation::GetSingletonInstance()->GetTextSizeFT(text, _fontSize);
}





/*
      Y
      |
      |   Z (pointing into the screen ?)
      |  /
      | /
      |/_____ X
     / target(0,1,0)
    /
   eye(0,2,-5)

   Right-handed coordinate system:
   - X-axis: positive direction is to the right
   - Y-axis: positive direction is upward
   - Z-axis: positive direction is into the screen
*/
void UICameraBase::SetViewMatrix() {
	_view = Matrix::CreateLookAt(
		Vector3(_position.x, _position.y, _position.z), // eye
		Vector3(_target.x, _target.y, _target.z),       // at
		Vector3(_up.x, _up.y, _up.z)                    // up
	);
}
	
void UICameraBase::SetProjectionMatrix() {
	_projection3D = Matrix::CreatePerspectiveFieldOfView(
		_fov,
		_aspectRatio,
		_nearPlane,
		_farPlane
	);
}

// let the camera can see the whole screen
void UICameraUI::SetCameraFor2D(float width, float height) {
	_position = XMFLOAT3(0.0f, 0.0f, -height/2.0f);
	_target = XMFLOAT3(0.0f, 0.0f, 0.0f);
	_up = XMFLOAT3(0.0f, -1.0f, 0.0f);
	//
	SetViewMatrix();

	//
	_fov = XM_PIDIV2;
	_aspectRatio = width/height;
	//
	SetProjectionMatrix();
}

// convert screen 2D to 3D
// screenPos: the 2D screen position in the window
// z: the z depth in the window [0,1]
XMFLOAT3 UICameraUI::ConvertScreen2DTo3D(const XMFLOAT3& screenPos) {
    // get device resources and viewport size
    float viewportWidth = static_cast<float>(UIDXFoundation::GetSingletonInstance()->GetOutputWidth());
    float viewportHeight = static_cast<float>(UIDXFoundation::GetSingletonInstance()->GetOutputHeight());
    
    // convert screen 2D to NDC[-1,1]
    float ndcX = (2.0f * screenPos.x / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPos.y / viewportHeight);
    
    // calculate target depth (interpolation from near plane to far plane)
    float depth = _nearPlane + screenPos.z * (_farPlane - _nearPlane);
    
    // calculate in view space
    float tanHalfFovY = tanf(_fov / 2.0f);
    float totalDepth = viewportHeight/2 + depth;
    float viewX = ndcX * totalDepth * _aspectRatio * tanHalfFovY;
    float viewY = ndcY * totalDepth * tanHalfFovY;
    float viewZ = totalDepth; // view space Z includes camera retreat distance
	// 
    XMVECTOR viewPosVec = XMVectorSet(viewX, viewY, viewZ, 1.0f);
    
    // build view space to world space transform matrix (inverse of view matrix)
    XMVECTOR rightVec = XMLoadFloat3(&_right);
    XMVECTOR upVec = XMLoadFloat3(&_up);
    XMVECTOR forwardVec = XMLoadFloat3(&_forward);
    XMVECTOR camPosVec = XMLoadFloat3(&_position);
    
    // build view space to world space transform matrix (inverse of view matrix)
    XMMATRIX worldTransform = XMMatrixSet(
        XMVectorGetX(rightVec), XMVectorGetX(upVec), XMVectorGetX(forwardVec), 0.0f,
        XMVectorGetY(rightVec), XMVectorGetY(upVec), XMVectorGetY(forwardVec), 0.0f,
        XMVectorGetZ(rightVec), XMVectorGetZ(upVec), XMVectorGetZ(forwardVec), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
	// calculate world position
    XMVECTOR worldPosVec = XMVector3Transform(viewPosVec, worldTransform);
	
	// add camera position
    worldPosVec = XMVectorAdd(worldPosVec, camPosVec);
    
    // convert result to XMFLOAT3
    XMFLOAT3 worldPos;
    XMStoreFloat3(&worldPos, worldPosVec);
    
    return worldPos;
}

// convert 3D space coordinates to window 2D coordinates
// worldPos: the 3D world position
XMFLOAT3 UICameraUI::Convert3DToScreen2D(const XMFLOAT3& worldPos) {
    // get device resources and viewport size
    float viewportWidth = static_cast<float>(UIDXFoundation::GetSingletonInstance()->GetOutputWidth());
    float viewportHeight = static_cast<float>(UIDXFoundation::GetSingletonInstance()->GetOutputHeight());

	// build world space to view space transform matrix
    XMVECTOR rightVec = XMLoadFloat3(&_right);
    XMVECTOR upVec = XMLoadFloat3(&_up);
    XMVECTOR forwardVec = XMLoadFloat3(&_forward);
    XMVECTOR camPosVec = XMLoadFloat3(&_position);
    
    // build world space to view space transform matrix
    XMMATRIX worldToViewTransform = XMMatrixSet(
		XMVectorGetX(rightVec), XMVectorGetY(rightVec), XMVectorGetZ(rightVec), 0.0f,
		XMVectorGetX(upVec), XMVectorGetY(upVec), XMVectorGetZ(upVec), 0.0f,
		XMVectorGetX(forwardVec), XMVectorGetY(forwardVec), XMVectorGetZ(forwardVec), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// convert XMFLOAT3 to XMVECTOR
	XMVECTOR worldPosVec = XMLoadFloat3(&worldPos);

	// subtract camera position
	worldPosVec = XMVectorSubtract(worldPosVec, camPosVec);

	// calculate in view space
	XMVECTOR viewPosVec = XMVector3Transform(worldPosVec, worldToViewTransform); //??
	//
	float viewX = XMVectorGetX(viewPosVec);
	float viewY = XMVectorGetY(viewPosVec);
	float viewZ = XMVectorGetZ(viewPosVec);

	float tanHalfFovY = tanf(_fov / 2.0f);
    float totalDepth = viewZ;
	
	// calculate depth in the window
    float depth = totalDepth - viewportHeight/2;

	// calculate ndc
	float ndcX = viewX / (totalDepth * _aspectRatio * tanHalfFovY);
	float ndcY = viewY / (totalDepth * tanHalfFovY);

	XMFLOAT3 screenPos;
	screenPos.x = (ndcX + 1.0f) * viewportWidth / 2.0f;
	screenPos.y = (1.0f - ndcY) * viewportHeight / 2.0f;
	screenPos.z = (depth - _nearPlane) / (_farPlane - _nearPlane);   // [0,1]

	return screenPos;
}

void UICameraGame::SetCamera(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up) {
	_position = position;
	_target = target;
	_up = up;

	SetViewMatrix();
}

void UICameraGame::SetAspectRatioAndProjectionMatrix(float aspectRatio) {
	_aspectRatio = aspectRatio;
	SetProjectionMatrix();
}

XMMATRIX UIZPlaneTransform::GetTransformMatrix(bool isRotationZ, float xByZ, float yByZ, float zAngle,
											   bool isRotationX, float yByX, float xAngle,
											   bool isRotationY, float xByY, float yAngle,
											   float z) {
	XMMATRIX finalMatrix = XMMatrixIdentity();

	// z axis rotation matrix in 2D space
	if (isRotationZ) {
		XMFLOAT3 zPivot = UICameraUI::GetSingletonInstance()->ConvertScreen2DTo3D(XMFLOAT3(xByZ, yByZ, z));
		XMVECTOR pivotPoint = XMLoadFloat3(&zPivot);
		
		XMMATRIX toOrigin = XMMatrixTranslationFromVector(-pivotPoint);
		XMMATRIX rotation = XMMatrixRotationZ(zAngle);
		XMMATRIX fromOrigin = XMMatrixTranslationFromVector(pivotPoint);
		
		finalMatrix = finalMatrix * (toOrigin * rotation * fromOrigin);
	}

	// xy axis rotation matrix, in the same z plane
	if (isRotationX || isRotationY) {
		XMFLOAT3 xyPivot = UICameraUI::GetSingletonInstance()->ConvertScreen2DTo3D(XMFLOAT3(isRotationY?xByY:0.f, isRotationX?yByX:0.f, z));
		XMVECTOR pivotPoint = XMLoadFloat3(&xyPivot);

		// move to origin
		XMMATRIX toOrigin = XMMatrixTranslationFromVector(-pivotPoint);

		// rotate
		XMMATRIX rotation = XMMatrixIdentity();         
		if (isRotationX) {
			rotation = rotation * XMMatrixRotationX(xAngle);
		}
		if (isRotationY) {
			rotation = rotation * XMMatrixRotationY(yAngle);
		}
		
		// move back
		XMMATRIX fromOrigin = XMMatrixTranslationFromVector(pivotPoint);
		
		finalMatrix = finalMatrix * (toOrigin * rotation * fromOrigin);
	}

	return finalMatrix;
}

void UIZPlaneTransform::TransformPoint(const XMMATRIX& transform, const XMFLOAT2& point, float z, XMFLOAT3& wp) {
	wp = UICameraUI::GetSingletonInstance()->ConvertScreen2DTo3D(XMFLOAT3(point.x, point.y, z));
	XMVECTOR v = XMVector3Transform(XMLoadFloat3(&wp), transform);
	XMStoreFloat3(&wp, v);
}

void UIZPlaneTransform::TransformPoints(const XMMATRIX& transform, const vector<XMFLOAT2>& points, float z, vector<XMFLOAT3>& wps) {
	wps.clear();
	for (const auto& point : points) {
		TransformPoint(transform, point, z, wps.emplace_back());
	}
}

void UIZPlaneTransform::TransformLinePoints(const XMMATRIX& transform, const XMFLOAT2& ps, const XMFLOAT2& pe, float z, vector<XMFLOAT3>& wps) {
	wps.resize(2);
	TransformPoints(transform, {ps, pe}, z, wps);
}

void UIZPlaneTransform::TransformRectPoints(const XMMATRIX& transform, const XMFLOAT2& ps, const XMFLOAT2& pe, float z, vector<XMFLOAT3>& wps) {
	wps.resize(4);
	TransformPoints(transform, {ps, {pe.x, ps.y}, {ps.x, pe.y}, pe}, z, wps);
}

void UI3DRotation::SetRotationZ(bool isRotationZ, LONG xbyZ, LONG ybyZ, float zAngle) {
	_isRotationZ = isRotationZ;
	_XYByZ = XMFLOAT2((float)xbyZ, (float)ybyZ);
	_zAngle = zAngle;
}

void UI3DRotation::SetRotationX(bool isRotationX, LONG yByx, float xAngle) {
	_isRotationX = isRotationX;
	_XY.y = (float)yByx;
	_xAngle = xAngle;
}

void UI3DRotation::SetRotationY(bool isRotationY, LONG xByY, float yAngle) {
	_isRotationY = isRotationY;
	_XY.x = (float)xByY;
	_yAngle = yAngle;
}