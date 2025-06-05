#pragma once

#include "UIDXFoundation.h"






/*-------------------------------------------------------Animation Engine-------------------------------------------------------*/

// Base class of animation
/*
Derived from the base class, the animation object has two drawing situations:
1. Complete the DrawAuto() function, and the object itself draws
2. Do not complete the DrawAuto() function, only inherit the animation drawing judgment, and handle the specific drawing separately
*/
struct UIAnimationBase {
	friend class UIAnimationManage;

	virtual bool IsAnimationRun() = 0;					// Whether the animation is running
	virtual bool UpdateAnimation() = 0;					// Calculate whether to switch to the next frame
	virtual void DrawAnimation(){}						// This frame animation drawing may be empty
};



// Animation management
class UIAnimationManage : public SingletonPattern<UIAnimationManage> {
    friend class SingletonPattern<UIAnimationManage>;

public:
	void AddAnimation(UIAnimationBase* pAnimateObj);
	void DelAnimation(UIAnimationBase* pAnimateObj);
	bool UpdateAnimations();
	void DrawAnimations();

protected:
	UIAnimationManage() = default;
	~UIAnimationManage();

	std::list<UIAnimationBase*> _animationList;				// Animation list, animations will be deleted automatically when finished
};
void UIRegisterAnimate(UIAnimationBase* pAnimateObj);



// Cursor support
class UICaret : public UIAnimationBase, public SingletonPattern<UICaret> {
    friend class SingletonPattern<UICaret>;

public:
	void SetCaret(float z, ULONG width, ULONG height, const UIColor& color);
	void HideCaret();
	void SetPos(ULONG x, ULONG y, bool IsShowImmd, const DirectX::XMMATRIX& transformMatrix);

private:
	UICaret();

	bool IsAnimationRun();
	bool UpdateAnimation();
	void DrawAnimation();

	// Drawing related
	ULONG _x, _y, _width, _height; // _x, _y are the coordinates of the first point of the caret center
	float _z;			
	UIColor _color;

	// Animation related
	bool _isCaretOn; // Display flag
	DWORD _oldTick;

	bool _1stFrame;

	DirectX::XMMATRIX _transformMatrix;
};
void UIShowCaret(float z, ULONG width, ULONG height, const UIColor& color);
void UIHideCaret();
void UISetCaretPos(ULONG x, ULONG y, bool IsShowImmd=true, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());






// Animation effect helper class
class UIAnimateHelp : public UIAnimationBase {
public:
	UIAnimateHelp();

	void PlayAnimate(int maxFrame=5);

protected:
	bool IsAnimationRun();
	bool UpdateAnimation();

	int _maxFrame;
	int _frameIndex;			// Start from 1
	bool _1stFrame;
};



// Animation common effects
class UIAnimateEffectHitDrum : public UIAnimateHelp {
public:
	UIAnimateEffectHitDrum();
	~UIAnimateEffectHitDrum() = default;

	void PlayHitDrumAnimate(int maxFrame=10);

	/*--------------------------------- Hit drum effect ---------------------------------*/
public:
	void SetHitPower(float v);
	void SetDLLPath(std::wstring path=L"GUIResource.dll", const UIColor& colorKey=UIColor::Invalid);
	void SetDLLID(int id);
	void SetImagePath(std::wstring path, const UIColor& colorKey=UIColor::Invalid);

	void DrawHitDrumAnimate(int centerX, int centerY, float _z, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void DrawHitDrumAnimate(RECT rc, float _z, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void DrawFrameHitDrumAnimate(RECT rc, int corner, float _z, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

protected:
	void InitializeCommon();

	int _loadImageWay;							// 1: _imageID    2: _imagePath
	int _imageID;
	std::wstring _dllPath;
	std::wstring _imagePath;
	UIColor _colorKey;
	float _hitPower;							// Image magnification coefficient
};