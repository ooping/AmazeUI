#pragma once

#include "pch.h"
#include <memory>

// ========== UI Widget IDs ==========
namespace UIWidgetIDs {
	// #define WidgetAuto(Type, IDNum, IDName, FuncName) \
	// constexpr int IDNum = IDNum; \
	// inline auto FuncName() { return UIGetWidgetByID<Type>(IDNum); }

	// ===== Main (0xx) =====
	WidgetAuto(UILabel, 10, label_MSG, Label_MSG)
}
using namespace UIWidgetIDs;


class PhoenixModel : public UIModelAnimation {
public:
	void Initialize() {
		LoadFromFile(L"Phoenix\\source\\fly.fbx");
		PlayAnimation(0, true, 10.0f);
	}

	// transform
	DirectX::XMFLOAT3 _position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 _rotation = { 180.0f, 0.0f, 0.0f };  // Flip upside down (X-axis 180 degrees)
	float _scale = 0.004f;
};

class DragonModel : public UIModelAnimation {
public:
	void Initialize() {
		LoadFromFile(L"Dragon\\source\\Dwarf Idle.fbx");
		PlayAnimation(0, true, 5.0f);
	}

	// transform
	DirectX::XMFLOAT3 _position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 _rotation = { 180.0f, 0.0f, 0.0f };  // Flip upside down (X-axis 180 degrees)
	float _scale = 0.2f;
};


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
	void LoadModel();

private:
	// 3D models
	PhoenixModel _phoenixModel;
	DragonModel _dragonModel;

	// Independent camera system (like UIChart3D)
	UICameraCtrl _cameraCtrl;
	
	// Mouse interaction state
	POINT _lastMousePos = { 0, 0 };
	bool _moveFlag = false;  // Right button down flag
	
	// Inherited transform matrix
	DirectX::XMMATRIX _inheritedTransformMatrix;
};



class UIWinTop : public UIWindow<UIWinTop>,
                 public SingletonPattern<UIWinTop>,
                 public SingleThreadHelper<UIWinTop> {
	friend class SingletonPattern<UIWinTop>;
    
public:
	void OnCreate();
	void OnDestroy();
};