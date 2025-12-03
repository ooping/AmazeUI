#include "UICamera.h"
#include "UIDXFoundation.h"
#include <algorithm>

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace UIShape2D;


bool UICameraUI2D::SetUpCamera(const UIRECT& viewRC) {
	if (GetRectWidth()(viewRC) == 0 || GetRectHeight()(viewRC) == 0) {
		return false;
	}

	// Set view matrix to identity (no transformation for 2D)
	_view = Matrix::Identity;

	// Create 2D orthographic projection matrix
	_projection = Matrix::CreateOrthographicOffCenter(
		0.0f, static_cast<float>(GetRectWidth()(viewRC)),
		static_cast<float>(GetRectHeight()(viewRC)), 0.0f,
		0.0f, 1.0f
	);

	_viewport = D3D12_VIEWPORT{
		(float)viewRC.left,
		(float)viewRC.top,
		(float)GetRectWidth()(viewRC),
		(float)GetRectHeight()(viewRC),
		D3D12_MIN_DEPTH,
		D3D12_MAX_DEPTH
	};

	return true;
}

float UICameraUI2D::CalculateNdcZByOrtho(float z_view) const {
	// For orthographic projection matrix, z transformation is linear
	// z_ndc = (z_view * m22 + m32) / 1.0  (because w component is always 1.0)
	return (z_view * _projection.m[2][2] + _projection.m[3][2]) / 1.0f;
}

float UICameraUI2D::CalculateViewZByOrtho(float z_ndc) const {
	// For orthographic projection matrix, z transformation is linear
	// z_view = (z_ndc - m32) / m22
	return (z_ndc - _projection.m[3][2]) / _projection.m[2][2];
}


bool UICameraBase3D::SetUpCamera(const UIRECT& viewRC) {
	if (GetRectWidth()(viewRC) == 0 || GetRectHeight()(viewRC) == 0) {
		return false;
	}

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

/*
      Y
      |
      |   Z (pointing into the screen)
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
void UICameraBase3D::SetViewMatrix() {
	XMVECTOR eye = XMVectorSet(_position._x, _position._y, _position._z, 1.0f);
    XMVECTOR focus = XMVectorSet(_target._x, _target._y, _target._z, 1.0f);
    XMVECTOR up = XMVectorSet(_up._x, _up._y, _up._z, 0.0f);
    
    _view = XMMatrixLookAtLH(eye, focus, up);  // left-handed coordinate system
}

void UICameraBase3D::SetProjectionMatrix() {
	_projection = XMMatrixPerspectiveFovLH(_fov, _aspectRatio, _nearPlane, _farPlane);
}

// Convert screen 2D to 3D
// screenPos: the 2D screen position in the window
// z: the z depth in the window [0,1]
UIVector3F UICameraBase3D::ConvertScreen2DTo3D(const UIVector3F& screenPos) {
    // Get device resources and viewport size
    float viewportWidth = static_cast<float>(_viewport.Width);
    float viewportHeight = static_cast<float>(_viewport.Height);

    // Convert screen 2D to NDC[-1,1]
    float ndcX = (2.0f * screenPos._x / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPos._y / viewportHeight);
    
    // Calculate target depth (interpolation from near plane to far plane)
    float depth = _nearPlane + screenPos._z * (_farPlane - _nearPlane);
    float totalDepth = -_position._z + depth;

    // Calculate in view space
    float tanHalfFovY = tanf(_fov / 2.0f);
    float viewX = ndcX * _aspectRatio * totalDepth * tanHalfFovY;
    float viewY = ndcY * totalDepth * tanHalfFovY;
    float viewZ = totalDepth; // view space Z includes camera retreat distance
	
    XMVECTOR viewPosVec = XMVectorSet(viewX, viewY, viewZ, 1.0f);
   
    // Build view space to world space transform matrix (inverse of view matrix)
    XMMATRIX worldTransform = XMMatrixSet(
        _right._x, _up._x, _forward._x, 0.0f,
        _right._y, _up._y, _forward._y, 0.0f,
        _right._z, _up._z, _forward._z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    
	// Calculate world position
    XMVECTOR worldPosVec = XMVector3Transform(viewPosVec, worldTransform);
	
	// Add camera position
    XMFLOAT3 posXM = _position.ToXMFLOAT3();
    XMVECTOR camPosVec = XMLoadFloat3(&posXM);
    worldPosVec = XMVectorAdd(worldPosVec, camPosVec);
    
    // Convert result to UIVector3F
    XMFLOAT3 worldPosXM;
    XMStoreFloat3(&worldPosXM, worldPosVec);
    
    return UIVector3F(worldPosXM);
}

// Convert 3D space coordinates to window 2D coordinates
// worldPos: the 3D world position
UIVector3F UICameraBase3D::Convert3DToScreen2D(const UIVector3F& worldPos) {
    // Get device resources and viewport size
    float viewportWidth = static_cast<float>(_viewport.Width);
    float viewportHeight = static_cast<float>(_viewport.Height);

    // Build world space to view space transform matrix
    XMMATRIX worldToViewTransform = XMMatrixSet(
        _right._x, _right._y, _right._z, 0.0f,
        _up._x, _up._y, _up._z, 0.0f,
        _forward._x, _forward._y, _forward._z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

	// Convert UIVector3F to XMVECTOR
	XMFLOAT3 worldPosXM = worldPos.ToXMFLOAT3();
	XMVECTOR worldPosVec = XMLoadFloat3(&worldPosXM);

	// Subtract camera position
	XMFLOAT3 posXM = _position.ToXMFLOAT3();
	XMVECTOR camPosVec = XMLoadFloat3(&posXM);
	worldPosVec = XMVectorSubtract(worldPosVec, camPosVec);

	// Calculate in view space
	XMVECTOR viewPosVec = XMVector3Transform(worldPosVec, worldToViewTransform);
	
	float viewX = XMVectorGetX(viewPosVec);
	float viewY = XMVectorGetY(viewPosVec);
	float viewZ = XMVectorGetZ(viewPosVec);

	float tanHalfFovY = tanf(_fov / 2.0f);
    float totalDepth = viewZ;
	
	// Calculate depth in the window
    float depth = totalDepth - viewportHeight / 2;

	// Calculate NDC
	float ndcX = viewX / (totalDepth * _aspectRatio * tanHalfFovY);
	float ndcY = viewY / (totalDepth * tanHalfFovY);

	UIVector3F screenPos;
	screenPos._x = (ndcX + 1.0f) * viewportWidth / 2.0f;
	screenPos._y = (1.0f - ndcY) * viewportHeight / 2.0f;
	screenPos._z = (depth - _nearPlane) / (_farPlane - _nearPlane);   // [0,1]

	return screenPos;
}

void UICameraBase3D::CalculateWorldDimensions() {
	// Rotation-aware world space calculation for Chart3D control
	// 1. Basic frustum dimensions (in camera space)
	float viewDistance = GetViewDistance();
	float tanHalfFOV = tan(_fov * 0.5f);
	float viewSpaceHeight = 2.0f * viewDistance * tanHalfFOV;
	float viewSpaceWidth = viewSpaceHeight * _aspectRatio;
	
	// 2. Get camera orientation vectors
	XMFLOAT3 forwardXM = _forward.ToXMFLOAT3();
	XMVECTOR forward = XMLoadFloat3(&forwardXM);
	XMFLOAT3 upXM = _up.ToXMFLOAT3();
	XMVECTOR up = XMLoadFloat3(&upXM);
	XMFLOAT3 rightXM = _right.ToXMFLOAT3();
	XMVECTOR right = XMLoadFloat3(&rightXM);
	
	// 3. Calculate frustum corner positions in world space
	XMFLOAT3 camPosXM = _position.ToXMFLOAT3();
	XMVECTOR camPos = XMLoadFloat3(&camPosXM);
	
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
	
	// Allow full rotation (removed pitch limit to enable camera flip)
	_pitch = max(-89.0f, min(89.0f, _pitch));  // Commented out to allow 360-degree rotation
	
	// Update camera position
	// Convert angles to radians
	float yawRad = XMConvertToRadians(_yaw);
	float pitchRad = XMConvertToRadians(_pitch);
	
	// Calculate camera position relative to target (spherical coordinate system)
	float x = viewDistance * cosf(pitchRad) * sinf(yawRad);
	float y = viewDistance * sinf(pitchRad);
	float z = -viewDistance * cosf(pitchRad) * cosf(yawRad);
	
	// Set new camera position
	_position = UIVector3F(_target._x + x, _target._y + y, _target._z + z);
	
	// Recalculate view matrix
	SetViewMatrix();
}







XMMATRIX UIZPlaneTransform::GetTransformMatrix(bool isRotationZ, float xByZ, float yByZ, float zAngle,
											   bool isRotationX, float yByX, float xAngle,
											   bool isRotationY, float xByY, float yAngle,
											   float z) {
	XMMATRIX finalMatrix = XMMatrixIdentity();

	// z axis rotation matrix in 2D space
	if (isRotationZ) {
		UIVector3F zPivot = UICameraUI3D::GetSingletonInstance()->ConvertScreen2DTo3D(UIVector3F(xByZ, yByZ, z));
		XMFLOAT3 zPivotXM = zPivot.ToXMFLOAT3();
		XMVECTOR pivotPoint = XMLoadFloat3(&zPivotXM);
		
		XMMATRIX toOrigin = XMMatrixTranslationFromVector(-pivotPoint);
		XMMATRIX rotation = XMMatrixRotationZ(zAngle);
		XMMATRIX fromOrigin = XMMatrixTranslationFromVector(pivotPoint);
		
		finalMatrix = finalMatrix * (toOrigin * rotation * fromOrigin);
	}

	// xy axis rotation matrix, in the same z plane
	if (isRotationX || isRotationY) {
		UIVector3F xyPivot = UICameraUI3D::GetSingletonInstance()->ConvertScreen2DTo3D(UIVector3F(isRotationY ? xByY : 0.f, isRotationX ? yByX : 0.f, z));
		XMFLOAT3 xyPivotXM = xyPivot.ToXMFLOAT3();
		XMVECTOR pivotPoint = XMLoadFloat3(&xyPivotXM);

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

void UIZPlaneTransform::TransformPoint(const XMMATRIX& transform, const UIVector2F& point, float z, UIVector3F& wp) {
	wp = UICameraUI3D::GetSingletonInstance()->ConvertScreen2DTo3D(UIVector3F(point._x, point._y, z));
	XMFLOAT3 wpXM = wp.ToXMFLOAT3();
	XMVECTOR v = XMVector3Transform(XMLoadFloat3(&wpXM), transform);
	XMFLOAT3 resultXM;
	XMStoreFloat3(&resultXM, v);
	wp = UIVector3F(resultXM);
}

void UIZPlaneTransform::TransformPoints(const XMMATRIX& transform, const vector<UIVector2F>& points, float z, vector<UIVector3F>& wps) {
	wps.clear();
	for (const auto& point : points) {
		TransformPoint(transform, point, z, wps.emplace_back());
	}
}

void UIZPlaneTransform::TransformLinePoints(const XMMATRIX& transform, const UIVector2F& ps, const UIVector2F& pe, float z, vector<UIVector3F>& wps) {
	TransformPoints(transform, {ps, pe}, z, wps);
}

void UIZPlaneTransform::TransformRectPoints(const XMMATRIX& transform, const UIVector2F& ps, const UIVector2F& pe, float z, vector<UIVector3F>& wps) {
	TransformPoints(transform, {ps, UIVector2F(pe._x, ps._y), UIVector2F(ps._x, pe._y), pe}, z, wps);
}

void UI3DRotation::SetRotationZ(bool isRotationZ, LONG xbyZ, LONG ybyZ, float zAngle) {
	_isRotationZ = isRotationZ;
	_XYByZ = UIVector2F((float)xbyZ, (float)ybyZ);
	_zAngle = zAngle;
}

void UI3DRotation::SetRotationX(bool isRotationX, LONG yByx, float xAngle) {
	_isRotationX = isRotationX;
	_XY._y = (float)yByx;
	_xAngle = xAngle;
}

void UI3DRotation::SetRotationY(bool isRotationY, LONG xByY, float yAngle) {
	_isRotationY = isRotationY;
	_XY._x = (float)xByY;
	_yAngle = yAngle;
}
