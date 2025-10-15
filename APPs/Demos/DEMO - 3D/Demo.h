#pragma once

#include "pch.h"
#include "StaticMesh.h"
#include <memory>


class UIGame3D : public UIControlBase<UIGame3D> {
	friend UIControlBase;

public:
	UIGame3D();
	~UIGame3D();
	
	void CalcArea();
	void Draw();
	
	// Mouse event handlers
	bool OnRButtonDown(POINT pt);
	bool OnRButtonUp(POINT pt);
	bool OnMouseMove(POINT pt);
	void OnMouseLeave(POINT pt);
	
	// Game-specific methods
	void LoadModel(const std::wstring& filePath);

private:
	// Dragon mesh
	std::unique_ptr<StaticMesh> _dragonMesh;
	
	// Dragon transform
	DirectX::XMFLOAT3 _dragonPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 _dragonRotation = { 0.0f, 0.0f, 0.0f };  // Euler angles (degrees)
	float _dragonScale = 0.006f;
	
	// Independent camera system (like UIChart3D)
	UICameraCtrl _cameraCtrl;
	
	// Mouse interaction state
	POINT _lastMousePos = { 0, 0 };
	bool _moveFlag = false;  // Right button down flag
	
	// Inherited transform matrix
	DirectX::XMMATRIX _inheritedTransformMatrix;
};



class UIWinTop : public UIWindow<UIWinTop>, public SingleThreadHelper<UIWinTop> {
    
public:
	void OnCreate();
	void OnDestroy();
	void OnNotify(int id, LPARAM param);

private:
	void ShowMsg(const std::string& msg);

	UIGame3D _game3D;
};