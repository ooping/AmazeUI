//--------------------------------------------------------------------------------------
// Main.cpp
//
// Entry point for Windows desktop application.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Demo.h"
#include <Dbt.h>

using namespace DirectX;
using namespace std;
using namespace UIShape2D;


UIWinTop gWinTop;


UIGame3D::UIGame3D() {
}

UIGame3D::~UIGame3D() {
}

void UIGame3D::CalcArea() {
	RECT ctrlRC = GetAbsoluteRect();
	
	// Set up camera system (like UIChart3D)
	if (!_cameraCtrl.SetUpCamera(ctrlRC)) {
		return;
	}
}

void UIGame3D::LoadModel() {
	_phoenixModel.Initialize();
	//_dragonModel.Initialize();
}

void UIGame3D::Draw() {
	// UIModelAnimation animation updates automatically via UIAnimationManage system
	// No manual update needed!
	
	// Get inherited transform matrix (like UIChart3D)
	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	
	// Draw border
	RECT rc = _clientRC;
	OffsetRect(&rc, x_, y_);
	//UIRect(rc, _z, GetRenderLayout() + 1)(UIColor::PrimaryBlue, transformMatrix);
	
	{	// Calculate world matrix for phoenix
		XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(_phoenixModel._rotation.x),
			XMConvertToRadians(_phoenixModel._rotation.y),
			XMConvertToRadians(_phoenixModel._rotation.z)
		);
		
		XMMATRIX translationMatrix = XMMatrixTranslation(
			_phoenixModel._position.x,
			_phoenixModel._position.y,
			_phoenixModel._position.z
		);
		
		XMMATRIX scaleMatrix = XMMatrixScaling(_phoenixModel._scale, _phoenixModel._scale, _phoenixModel._scale);
		
		// World = Scale * Rotation * Translation
		XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		
		// Render phoenix with camera system
		_phoenixModel.Render(&_cameraCtrl, worldMatrix);
	}

	// {	// Calculate world matrix for dragon
	// 	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
	// 		XMConvertToRadians(_dragonModel._rotation.x),
	// 		XMConvertToRadians(_dragonModel._rotation.y),
	// 		XMConvertToRadians(_dragonModel._rotation.z)
	// 	);
		
	// 	XMMATRIX translationMatrix = XMMatrixTranslation(
	// 		_dragonModel._position.x,
	// 		_dragonModel._position.y,
	// 		_dragonModel._position.z
	// 	);
		
	// 	XMMATRIX scaleMatrix = XMMatrixScaling(_dragonModel._scale, _dragonModel._scale, _dragonModel._scale);
		
	// 	// World = Scale * Rotation * Translation
	// 	XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		
	// 	// Render dragon with camera system
	// 	_dragonModel.Render(&_cameraCtrl, worldMatrix);
	// }
}

bool UIGame3D::OnRButtonDown(POINT pt) {
	// Convert to control-relative coordinates
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;
	
	_lastMousePos = point;
	_moveFlag = true;  // Set right button down flag
	return true;
}

bool UIGame3D::OnRButtonUp(POINT) {
	_moveFlag = false;  // Clear right button down flag
	return true;
}

void UIGame3D::OnMouseLeave(POINT) {
	_moveFlag = false;  // Clear right button down flag when mouse leaves
}

bool UIGame3D::OnMouseMove(POINT pt) {
	// Only allow movement when right button is down
	if (_moveFlag) {
		POINT point = pt;
		point.x -= _abusolutePoint.x;
		point.y -= _abusolutePoint.y;

		// Calculate mouse movement distance
		int deltaX = point.x - _lastMousePos.x;
		int deltaY = point.y - _lastMousePos.y;

		// Only rotate when there is actual movement
		if (deltaX != 0 || deltaY != 0 || _lastMousePos.x != 0 || _lastMousePos.y != 0) {
			// Convert to rotation angles (further reduced sensitivity)
			float sensitivity = 0.1f;
			float deltaYaw = deltaX * sensitivity;
			float deltaPitch = -deltaY * sensitivity;  // Y-axis inverted

			// Rotate camera
			_cameraCtrl.RotateCamera(deltaYaw, deltaPitch);

			// Refresh display
			UIRefresh();
		}

		// Update last mouse position
		_lastMousePos = point;
	}

	return true;
}

void UIWinTop::OnCreate() {
	const RECT clientRC = GetClientRect();
	string str;

    _game3D.CreateWindowBase(&gWinTop, clientRC, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
	// Model will be loaded on first draw to ensure DirectX resources are ready
	_game3D.LoadModel();
}

void UIWinTop::OnDestroy() {
}

void UIWinTop::OnNotify(int id, LPARAM param) {
}













// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    // 
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // check CPU support
    if (!XMVerifyCPUSupport()) {
        return 1;
    }

    // initialize COM
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize)) {
        return 1;
    }

    UIPostMessage(NULL, WM_MQ, (WPARAM)(pMQFuncType)[](LPARAM) {
        RECT rc;
        ::GetClientRect(UIFrame::GetSingletonInstance()->GetWindowHandle(), &rc);

        gWinTop.CreateWindowBase(UIFrame::GetSingletonInstance()->GetTopUIContainer(), rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
    }, 0);

    // initialize UI
    UIWin32APP::GetSingletonInstance()->Instance(hInstance, L"AmazeUI 3D", 1800, 1000);
    return 0;
}













