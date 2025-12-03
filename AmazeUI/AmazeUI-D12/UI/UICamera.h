#pragma once

#include "UIUtility.h"


/*
 * UICamera.h
 * 
 * Centralized camera system for AmazeUI-D12 framework
 * Contains all camera classes for both 2D and 3D rendering
 * 
 * Hierarchy:
 *   UICameraBase (abstract base)
 *   ├── UICameraUI2D (2D orthographic camera for UI)
 *   └── UICameraBase3D (base for 3D perspective cameras)
 *       ├── UICameraUI3D (3D UI overlay camera)
 *       ├── UICameraGame (game world camera)
 *       └── UICameraCtrl (Chart3D control camera)
 */

/*------------------------------------------------------- UICameraBase -------------------------------------------------------*/
// Abstract base class for all cameras
// Provides common interface for view/projection matrices and viewport
class UICameraBase {
public:
    UICameraBase() = default;
	virtual ~UICameraBase() = default;

	// Setup camera (must be implemented by derived classes)
	virtual bool SetUpCamera(const UIRECT& viewRC) = 0;

	// Get matrices (pure virtual, implemented by derived classes)
	const DirectX::SimpleMath::Matrix& GetViewMatrix() { return _view; }
	const DirectX::SimpleMath::Matrix& GetProjectionMatrix() { return _projection; }

	// Get viewport
	const D3D12_VIEWPORT& GetViewport() { return _viewport; }

protected:
    DirectX::SimpleMath::Matrix _view;			// Identity matrix for 2D
	DirectX::SimpleMath::Matrix _projection;	// Orthographic projection matrix
	D3D12_VIEWPORT _viewport = {};
};


/*------------------------------------------------------- UICameraUI2D -------------------------------------------------------*/
// 2D orthographic camera for UI rendering
// Provides identity view matrix and orthographic projection matrix
class UICameraUI2D : public UICameraBase, public SingletonPattern<UICameraUI2D> {
	friend class SingletonPattern<UICameraUI2D>;

public:
	// Setup 2D camera with orthographic projection
	bool SetUpCamera(const UIRECT& viewRC) override;

	// Z-depth conversion functions for orthographic projection
	float CalculateNdcZByOrtho(float z_view) const;
	float CalculateViewZByOrtho(float z_ndc) const;

private:
	UICameraUI2D() = default;
	~UICameraUI2D() = default;
};


/*------------------------------------------------------- UICameraBase3D -------------------------------------------------------*/
// Base class for 3D perspective cameras
// Provides view and projection matrices for 3D world rendering
class UICameraBase3D : public UICameraBase {
public:
	// Camera vectors
	UIVector3F _position = {0.0f, 0.0f, -5.0f};
	UIVector3F _target = {0.0f, 0.0f, 0.0f};
	UIVector3F _up = {0.0f, 1.0f, 0.0f};
	UIVector3F _right = {1.0f, 0.0f, 0.0f};
	UIVector3F _forward = {0.0f, 0.0f, 1.0f};

	// Camera properties
	float _fov = DirectX::XM_PIDIV2;
	float _aspectRatio = 1.0f;	// should be set according to view width/height
	float _nearPlane = 0.01f;
	float _farPlane = 10000.0f;

	// World space dimensions (calculated from frustum)
	float _worldWidth = 0.0f;
	float _worldHeight = 0.0f;

	// Setup methods
	bool SetUpCamera(const UIRECT& viewRC) override;

	// Update camera matrices
	void SetViewMatrix();
	void SetProjectionMatrix();

	// World space access
	float GetWorldWidth() const { return _worldWidth; }
	float GetWorldHeight() const { return _worldHeight; }
	float GetViewDistance() const { return UIVectorMath::Distance3D(_position, _target); }

	// Convert screen 2D to 3D
	UIVector3F ConvertScreen2DTo3D(const UIVector3F& screenPos);
	UIVector3F Convert3DToScreen2D(const UIVector3F& viewPos);

	// World space calculation
	void CalculateWorldDimensions();
};

/*------------------------------------------------------- UICameraUI3D -------------------------------------------------------*/
// 3D camera for UI overlay rendering
// Singleton camera used for 3D UI elements
class UICameraUI3D : public UICameraBase3D, public SingletonPattern<UICameraUI3D> {
	friend class SingletonPattern<UICameraUI3D>;

private:
	UICameraUI3D() = default;
	~UICameraUI3D() = default;
};

/*------------------------------------------------------- UICameraGame -------------------------------------------------------*/
// 3D camera for game world rendering
// Singleton camera used for main game scene
class UICameraGame : public UICameraBase3D, public SingletonPattern<UICameraGame> {
	friend class SingletonPattern<UICameraGame>;

	UICameraGame() = default;
	~UICameraGame() = default;
};

/*------------------------------------------------------- UICameraCtrl -------------------------------------------------------*/
// Independent 3D camera system for Chart3D control, similar to Unity3D Scene View axes
// Instanced camera with mouse rotation support
class UICameraCtrl : public UICameraBase3D {
	// Mouse rotation support
	float _yaw = 0.0f;    // Left-right rotation angle
	float _pitch = 0.0f;  // Up-down rotation angle

public:
	UICameraCtrl() = default;
	~UICameraCtrl() = default;

	// Mouse rotation
	void RotateCamera(float deltaYaw, float deltaPitch);
};


/*------------------------------------------------------- UIZPlaneTransform -------------------------------------------------------*/
// Utility for transforming 2D points to 3D world coordinates
// Used by Draw3D series functions to convert screen space to world space
struct UIZPlaneTransform {
    static DirectX::XMMATRIX GetTransformMatrix(bool isRotationZ, float xByZ, float yByZ, float zAngle,
												bool isRotationX, float yByX, float xAngle,
												bool isRotationY, float xByY, float yAngle,
												float z);

	static void TransformPoint(const DirectX::XMMATRIX& transform, const UIVector2F& point, float z, UIVector3F& wp);
	static void TransformPoints(const DirectX::XMMATRIX& transform, const std::vector<UIVector2F>& points, float z, std::vector<UIVector3F>& wps);
	static void TransformLinePoints(const DirectX::XMMATRIX& transform, const UIVector2F& ps, const UIVector2F& pe, float z, std::vector<UIVector3F>& wps);
	static void TransformRectPoints(const DirectX::XMMATRIX& transform, const UIVector2F& ps, const UIVector2F& pe, float z, std::vector<UIVector3F>& wps);
};


/*------------------------------------------------------- UI3DRotation -------------------------------------------------------*/
// 3D rect rotation helper
// The rect is on the same z plane
// The rotation axis of x/y is in the same z plane
struct UI3DRotation {
	void SetRotationZ(bool isRotationZ, LONG xbyZ = 0, LONG ybyZ = 0, float zAngle = 0.f);
	void SetRotationX(bool isRotationX, LONG yByx = 0, float xAngle = 0.f);
	void SetRotationY(bool isRotationY, LONG xByY = 0, float yAngle = 0.f);

	bool _isRotationZ = false;
	UIVector2F _XYByZ;
	float _zAngle;

	bool _isRotationX = false;
	bool _isRotationY = false;
	UIVector2F _XY;
	float _xAngle;
	float _yAngle;
};
