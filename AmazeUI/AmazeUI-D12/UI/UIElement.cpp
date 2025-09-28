#include "UIElement.h"
#include "UIDXFoundation.h"
using namespace std;
using namespace DirectX;
using namespace UIShape2D;
using namespace SimpleMath;

UIScreenClipRectGuard::UIScreenClipRectGuard(const RECT& clipRC, bool execute) {
	_execute = execute;
	UIDXFoundation::GetSingletonInstance()->BeginScreenClipRect(clipRC, _execute);
}

UIScreenClipRectGuard::~UIScreenClipRectGuard() {
	UIDXFoundation::GetSingletonInstance()->EndScreenClipRect(_execute);
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
		if (_sourceFlag == 1)	{
			UIDXFoundation::GetSingletonInstance()->Draw3DImage(_path, _colorKey, srcRect,
														XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
														XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
														_z, alphy, transformMatrix);
		} else if (_sourceFlag == 2) {
			UIDXFoundation::GetSingletonInstance()->Draw3DImage(_path, _id, _colorKey, srcRect,
														XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
														XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
														_z, alphy, transformMatrix);
		}
	} else {
		if (_sourceFlag == 1)	{
			UIDXFoundation::GetSingletonInstance()->Draw2DImage(_path, _colorKey, srcRect,
														XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
														XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
														_z, alphy);
		} else if (_sourceFlag == 2) {
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
	this->operator()(NULL_RECT, scale == 1 ? dstRect : ScaleRect()(dstRect, scale), alphy, transformMatrix);
}


void UIImage::operator()(LONG dstCenterX, LONG dstCenterY, float scale, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	RECT textureRect;
	if (!GetSize(textureRect)) {
		return;
	}

	RECT dstRect = CreateRect()(UIShape2D::CreatePoint()((LONG)(dstCenterX - (textureRect.right * scale / 2)), (LONG)(dstCenterY - (textureRect.bottom * scale / 2))), 
										 CreateSize()((LONG)((textureRect.right - textureRect.left) * scale), (LONG)((textureRect.bottom - textureRect.top) * scale)));
	this->operator()(NULL_RECT, dstRect, alphy, transformMatrix);
}


bool UIImage::GetSize(RECT& textureRect) {
	if (_sourceFlag == 1) {
		return UIDXFoundation::GetSingletonInstance()->Get2DImageSize(_path, _colorKey, textureRect);
	} else if (_sourceFlag == 2) {
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
	if (!_image.GetSize(textureRect)) {
		return;
	}
	UINT imageWidth = GetRectWidth()(textureRect);
	UINT imageHeight = GetRectHeight()(textureRect);

	RECT rc1 = CreateRect()(UIShape2D::CreatePoint()(0, 0), CreateSize()(_leftBarWidth, _topBarHeight));
	_image(rc1, dstBeginX, dstBeginY, alphy, transformMatrix);

	RECT rc3 = CreateRect()(UIShape2D::CreatePoint()(imageWidth - _rightBarWidth , 0), CreateSize()(_rightBarWidth, _topBarHeight));
	_image(rc3, dstBeginX + width - _rightBarWidth, dstBeginY, alphy, transformMatrix);

	RECT rc4 = CreateRect()(UIShape2D::CreatePoint()(0, imageHeight - _bottomBarHeight), CreateSize()(_leftBarWidth, _bottomBarHeight));
	_image(rc4, dstBeginX, dstBeginY + height - _bottomBarHeight, alphy, transformMatrix);

	RECT rc6 = CreateRect()(UIShape2D::CreatePoint()(imageWidth - _rightBarWidth, imageHeight - _bottomBarHeight), CreateSize()(_rightBarWidth, _bottomBarHeight));
	_image(rc6, dstBeginX + width - _rightBarWidth, dstBeginY + height - _bottomBarHeight, alphy, transformMatrix);

	RECT rc2 = CreateRect()(UIShape2D::CreatePoint()(_leftBarWidth, 0) , CreateSize()(imageWidth - _leftBarWidth - _rightBarWidth, _topBarHeight));
	RECT rc2d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + _leftBarWidth, dstBeginY), CreateSize()(width - _leftBarWidth - _rightBarWidth, _topBarHeight));
	_image(rc2, rc2d, alphy, transformMatrix);

	RECT rc5 = CreateRect()(UIShape2D::CreatePoint()(_leftBarWidth, imageHeight - _bottomBarHeight), CreateSize()(imageWidth - _leftBarWidth - _rightBarWidth, _bottomBarHeight));
	RECT rc5d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + _leftBarWidth, dstBeginY + height - _bottomBarHeight), CreateSize()(width - _leftBarWidth - _rightBarWidth, _bottomBarHeight));
	_image(rc5, rc5d, alphy, transformMatrix);

	RECT rc7 = CreateRect()(UIShape2D::CreatePoint()(0, _topBarHeight), CreateSize()(_leftBarWidth, imageHeight - _topBarHeight - _bottomBarHeight));
	RECT rc7d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX, dstBeginY + _topBarHeight), CreateSize()(_leftBarWidth, height - _topBarHeight - _bottomBarHeight));
	_image(rc7, rc7d, alphy, transformMatrix);

	RECT rc8 = CreateRect()(UIShape2D::CreatePoint()(imageWidth - _rightBarWidth, _topBarHeight), CreateSize()(_rightBarWidth, imageHeight - _topBarHeight - _bottomBarHeight));
	RECT rc8d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + width - _rightBarWidth, dstBeginY + _topBarHeight), CreateSize()(_rightBarWidth, height - _topBarHeight - _bottomBarHeight));
	_image(rc8, rc8d, alphy, transformMatrix);
	
	RECT rc9 = CreateRect()(UIShape2D::CreatePoint()(_leftBarWidth, _topBarHeight), CreateSize()(imageWidth - _leftBarWidth - _rightBarWidth, imageHeight - _topBarHeight - _bottomBarHeight));
	RECT rc9d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + _leftBarWidth, dstBeginY + _topBarHeight), CreateSize()(width - _leftBarWidth - _rightBarWidth, height - _topBarHeight - _bottomBarHeight));
	_image(rc9, rc9d, alphy, transformMatrix);
}

void UISlicedImage::operator()(const RECT& dstRC, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	this->operator()(dstRC.left, dstRC.top, dstRC.right - dstRC.left, dstRC.bottom - dstRC.top, alphy, transformMatrix);
}

UIFont::UIFont(float z, float fontSize) {
	_z = z;
	_fontSize = fontSize;
}

void UIFont::operator()(std::wstring text, const POINT& position, const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextFT(text, XMFLOAT2{(float)position.x, (float)position.y}, _z - 0.005f, color, _fontSize, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextFT(text, XMFLOAT2{(float)position.x, (float)position.y}, _z, color, _fontSize);
	}
}

void UIFont::operator()(std::wstring text, const RECT& rc, const UIColor& color, int posFlag, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextFT(text, rc, posFlag, _z - 0.005f, color, _fontSize, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextFT(text, rc, posFlag, _z, color, _fontSize);
	}
}

void UIFont::operator()(std::wstring text, const POINT& position, const UIColor& color, float lineSpacing, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextMultiLineFT(text, XMFLOAT2{(float)position.x, (float)position.y}, _z - 0.005f, color, _fontSize, lineSpacing, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextMultiLineFT(text, XMFLOAT2{(float)position.x, (float)position.y}, _z, color, _fontSize, lineSpacing);
	}
}

void UIFont::operator()(std::wstring text, const RECT& rc, const UIColor& color, int posFlag, float lineSpacing, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextMultiLineFT(text, rc, posFlag, _z - 0.005f, color, _fontSize, lineSpacing, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextMultiLineFT(text, rc, posFlag, _z, color, _fontSize, lineSpacing);
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

   Left-handed coordinate system:
   - X-axis: positive direction is to the right
   - Y-axis: positive direction is upward
   - Z-axis: positive direction is into the screen
*/
void UICameraBase::SetViewMatrix() {
	// _view = Matrix::CreateLookAt(Vector3(_position.x, _position.y, _position.z), // eye
	// 							 Vector3(_target.x, _target.y, _target.z),       // at
	// 							 Vector3(_up.x, _up.y, _up.z));                  // up								

	XMVECTOR eye = XMVectorSet(_position.x, _position.y, _position.z, 1.0f);
    XMVECTOR focus = XMVectorSet(_target.x, _target.y, _target.z, 1.0f);
    XMVECTOR up = XMVectorSet(_up.x, _up.y, _up.z, 0.0f);
    
    _view = XMMatrixLookAtLH(eye, focus, up);  // left-handed coordinate system
}

void UICameraBase::SetProjectionMatrix() {
	//_projection3D = Matrix::CreatePerspectiveFieldOfView(_fov, _aspectRatio, _nearPlane, _farPlane);

	_projection3D = XMMatrixPerspectiveFovLH(_fov, _aspectRatio, _nearPlane, _farPlane);
}

bool UICameraBase::SetUpCamera(const RECT& viewRC) {
	if (GetRectWidth()(viewRC) == 0 || GetRectHeight()(viewRC) == 0) {
		return false;
	}

	//float distance = (height / 2) / tanf(_fov / 2.0f);
	SetViewMatrix();

	_aspectRatio = (float)GetRectWidth()(viewRC) / (float)GetRectHeight()(viewRC);
	SetProjectionMatrix();

	_viewport = D3D12_VIEWPORT{
		(float)viewRC.left,
		(float)viewRC.top,
		(float)GetRectWidth()(viewRC),
		(float)GetRectHeight()(viewRC),
		D3D12_MIN_DEPTH,
		D3D12_MAX_DEPTH
	};

	// Calculate world space dimensions considering camera rotation
	CalculateWorldDimensions();

	return true;
}

// convert screen 2D to 3D
// screenPos: the 2D screen position in the window
// z: the z depth in the window [0,1]
XMFLOAT3 UICameraBase::ConvertScreen2DTo3D(const XMFLOAT3& screenPos) {
    // get device resources and viewport size
    float viewportWidth = static_cast<float>(_viewport.Width);
    float viewportHeight = static_cast<float>(_viewport.Height);

    // convert screen 2D to NDC[-1,1]
    float ndcX = (2.0f * screenPos.x / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPos.y / viewportHeight);
    
    // calculate target depth (interpolation from near plane to far plane)
    float depth = _nearPlane + screenPos.z * (_farPlane - _nearPlane);
    float totalDepth = -_position.z + depth;

    // calculate in view space
    float tanHalfFovY = tanf(_fov / 2.0f);
    float viewX = ndcX * _aspectRatio * totalDepth * tanHalfFovY;
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
XMFLOAT3 UICameraBase::Convert3DToScreen2D(const XMFLOAT3& worldPos) {
    // get device resources and viewport size
    float viewportWidth = static_cast<float>(_viewport.Width);
    float viewportHeight = static_cast<float>(_viewport.Height);

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
    float depth = totalDepth - viewportHeight / 2;

	// calculate ndc
	float ndcX = viewX / (totalDepth * _aspectRatio * tanHalfFovY);
	float ndcY = viewY / (totalDepth * tanHalfFovY);

	XMFLOAT3 screenPos;
	screenPos.x = (ndcX + 1.0f) * viewportWidth / 2.0f;
	screenPos.y = (1.0f - ndcY) * viewportHeight / 2.0f;
	screenPos.z = (depth - _nearPlane) / (_farPlane - _nearPlane);   // [0,1]

	return screenPos;
}

void UICameraBase::CalculateWorldDimensions() {
	// **Rotation-aware world space calculation for Chart3D control**
	// 1. Basic frustum dimensions (in camera space)
	float viewDistance = GetViewDistance();
	float tanHalfFOV = tan(_fov * 0.5f);
	float viewSpaceHeight = 2.0f * viewDistance * tanHalfFOV;
	float viewSpaceWidth = viewSpaceHeight * _aspectRatio;
	
	// 2. Get camera orientation vectors
	XMVECTOR forward = XMLoadFloat3(&_forward);
	XMVECTOR up = XMLoadFloat3(&_up);
	XMVECTOR right = XMLoadFloat3(&_right);
	
	// 3. Calculate frustum corner positions in world space
	XMVECTOR camPos = XMLoadFloat3(&_position);
	
	// Four corners of the frustum in camera space
	float halfWidth = viewSpaceWidth * 0.5f;
	float halfHeight = viewSpaceHeight * 0.5f;
	
	XMVECTOR corners[4] = {
		XMVectorSet(-halfWidth, -halfHeight, viewDistance, 1.0f), // Bottom-left
		XMVectorSet( halfWidth, -halfHeight, viewDistance, 1.0f), // Bottom-right
		XMVectorSet(-halfWidth,  halfHeight, viewDistance, 1.0f), // Top-left
		XMVectorSet( halfWidth,  halfHeight, viewDistance, 1.0f)  // Top-right
	};
	
	// 4. Convert camera space coordinates to world space coordinates
	XMVECTOR worldCorners[4];
	for (int i = 0; i < 4; i++) {
		// Camera space to world space transformation
		XMVECTOR localPos = corners[i];
		worldCorners[i] = camPos + 
						  XMVectorGetX(localPos) * right +
						  XMVectorGetY(localPos) * up +
						  XMVectorGetZ(localPos) * forward;
	}
	
	// 5. Calculate projection range on the XY plane
	float minX = FLT_MAX, maxX = -FLT_MAX;
	float minY = FLT_MAX, maxY = -FLT_MAX;
	
	for (int i = 0; i < 4; i++) {
		float x = XMVectorGetX(worldCorners[i]);
		float y = XMVectorGetY(worldCorners[i]);
		
		minX = min(minX, x);
		maxX = max(maxX, x);
		minY = min(minY, y);
		maxY = max(maxY, y);
	}
	
	// 6. Final world space dimensions
	_worldWidth = maxX - minX;
	_worldHeight = maxY - minY;
}

void UICameraCtrl::RotateCamera(float deltaYaw, float deltaPitch) {
	_yaw += deltaYaw;
	_pitch += deltaPitch;
	float viewDistance = GetViewDistance();
	
	// Limit pitch angle to avoid flipping
	_pitch = std::max(-89.0f, std::min(89.0f, _pitch));
	
	// Update camera position
	// Convert angles to radians
	float yawRad = DirectX::XMConvertToRadians(_yaw);
	float pitchRad = DirectX::XMConvertToRadians(_pitch);
	
	// Calculate camera position relative to target (spherical coordinate system)
	float x = viewDistance * cosf(pitchRad) * sinf(yawRad);
	float y = viewDistance * sinf(pitchRad);
	float z = -viewDistance * cosf(pitchRad) * cosf(yawRad);
	
	// Set new camera position
	_position = DirectX::XMFLOAT3(_target.x + x, _target.y + y, _target.z + z);
	
	// Recalculate view matrix
	SetViewMatrix();
	
	// Recalculate world space dimensions after rotation
	//CalculateWorldDimensions();
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
		XMFLOAT3 xyPivot = UICameraUI::GetSingletonInstance()->ConvertScreen2DTo3D(XMFLOAT3(isRotationY ? xByY : 0.f, isRotationX ? yByX : 0.f, z));
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

// UIPoint3D implementation
UIPoint3D::UIPoint3D(float x, float y, float z) : _point(x, y, z) {}

void UIPoint3D::operator()(const UIColor& color, UICameraBase* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldPoint(_point, color, pCamera);
}

// UIPoints3D implementation
UIPoints3D::UIPoints3D(const std::vector<UIPointFloat3>& points) {
	_points.reserve(points.size());
	for (const auto& point : points) {
		_points.emplace_back(point._x, point._y, point._z);
	}
}

void UIPoints3D::operator()(const UIColor& color, UICameraBase* pCamera) {
	for (const auto& point : _points) {
		UIDXFoundation::GetSingletonInstance()->Draw3DWorldPoint(point, color, pCamera);
	}
}

// UILine3D implementation
UILine3D::UILine3D(UIPointFloat3 start, UIPointFloat3 end, float width) 
	: _start(start._x, start._y, start._z), _end(end._x, end._y, end._z), _width(width) {}

void UILine3D::operator()(const UIColor& colorS, const UIColor& colorE, UICameraBase* pCamera) {
	if (_width <= 1.0f) {
		UIDXFoundation::GetSingletonInstance()->Draw3DWorldLine(_start, _end, colorS, colorE, pCamera);
	} else {
		// For thick lines, use the thick line function with interpolated color
		UIColor avgColor(
			(colorS._r + colorE._r) / 2,
			(colorS._g + colorE._g) / 2,
			(colorS._b + colorE._b) / 2,
			(colorS._a + colorE._a) / 2
		);
		UIDXFoundation::GetSingletonInstance()->Draw3DWorldThickLine(_start, _end, _width, avgColor, pCamera);
	}
}

void UILine3D::operator()(const UIColor& color, UICameraBase* pCamera) {
	if (_width <= 1.0f) {
		UIDXFoundation::GetSingletonInstance()->Draw3DWorldLine(_start, _end, color, pCamera);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw3DWorldThickLine(_start, _end, _width, color, pCamera);
	}
}

// UICircle3D implementation
UICircle3D::UICircle3D(UIPointFloat3 center, float radius) 
	: _center(center._x, center._y, center._z), _radius(radius) {}

void UICircle3D::operator()(const UIColor& color, UICameraBase* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldCircle(_center, _radius, color, 255, pCamera);
}

// UITriangle3D implementation
UITriangle3D::UITriangle3D(UIPointFloat3 p1, UIPointFloat3 p2, UIPointFloat3 p3) 
	: _p1(p1._x, p1._y, p1._z), _p2(p2._x, p2._y, p2._z), _p3(p3._x, p3._y, p3._z) {}

void UITriangle3D::operator()(const UIColor& color1, const UIColor& color2, const UIColor& color3, UCHAR alpha, UICameraBase* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldTriangle(_p1, _p2, _p3, color1, color2, color3, alpha, pCamera);
}

void UITriangle3D::operator()(const UIColor& color, UCHAR alpha, UICameraBase* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldTriangle(_p1, _p2, _p3, color, alpha, pCamera);
}

// UIFont3D implementation
UIFont3D::UIFont3D(float fontSize) : _fontSize(fontSize) {}

void UIFont3D::operator()(std::wstring text, const UIPointFloat3& position, const UIColor& color, UICameraBase* pCamera) {
	// Use UIDXFoundation's 3D world text rendering directly
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldTextFT(text, {position._x, position._y, position._z}, color, _fontSize, pCamera);
}
