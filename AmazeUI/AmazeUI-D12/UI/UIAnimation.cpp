#include "UIAnimation.h"
#include "UIElement.h"
//#include "UI3D.h"

using namespace std;
using namespace Shape2D;

UIAnimationManage::~UIAnimationManage() {
	UICaret::GetSingletonInstance()->DestroySingletonInstance();
}

void UIAnimationManage::AddAnimation(UIAnimationBase* pAnimateObj) {
	if (pAnimateObj!=NULL) {
		if (find(_animationList.begin(), _animationList.end(), pAnimateObj)==_animationList.end()) {
			_animationList.push_back(pAnimateObj);
		}
	}
}

void UIAnimationManage::DelAnimation(UIAnimationBase* pAnimateObj) {
	if (pAnimateObj!=NULL) {
		auto it = find(_animationList.begin(), _animationList.end(), pAnimateObj);
		if (it!=_animationList.end()) {
			_animationList.erase(it);
		}
	}
}

bool UIAnimationManage::UpdateAnimations() {
	if (_animationList.size() == 0)	{
		return false;
	}

	// Calculate whether each animation switches to the next frame
	bool isUpdateAnimation = false;
    for (auto& animation : _animationList) {
        isUpdateAnimation |= animation->UpdateAnimation();
    }

    // Delete the animation that has finished playing
    _animationList.remove_if([](UIAnimationBase* animation) {
        return !animation->IsAnimationRun();
    });

	return isUpdateAnimation;
}

void UIAnimationManage::DrawAnimations() {
	for (auto i=_animationList.begin(); i!=_animationList.end(); ++i) {
		(*i)->DrawAnimation();
	}
}

void UIRegisterAnimate(UIAnimationBase* pAnimateObj) {
	UIPostMessage(NULL, WM_REGANIMATE, (WPARAM)pAnimateObj, 0);
}



UICaret::UICaret() {
	_x = 0;
	_y = 0;
	_width = 1;
	_height = 16;
	_z = -0.5;

	_oldTick = 0;

	_isCaretOn = false;
}

bool UICaret::IsAnimationRun() {
	return true;
}

bool UICaret::UpdateAnimation() {
	DWORD nowTick = ::GetTickCount();
	DWORD dxTick  = nowTick-_oldTick;						// ?????
	
	if (dxTick>600) {
		_oldTick = nowTick;
		_isCaretOn = !_isCaretOn;
		return true;
	}

	if (_1stFrame) {
		_1stFrame = false;
		return true;
	}

	return false;
}

void UICaret::HideCaret() {
	UIAnimationManage::GetSingletonInstance()->DelAnimation(UICaret::GetSingletonInstance());
}

void UICaret::SetCaret(float z, ULONG width, ULONG height, const UIColor& color) {
	_z = z;
	_width = width;
	_height = height;
	_color = color;
	_isCaretOn = true;
	_oldTick = ::GetTickCount();
	_1stFrame = false;
	
	UIAnimationManage::GetSingletonInstance()->AddAnimation(UICaret::GetSingletonInstance());
}

void UICaret::SetPos(ULONG x, ULONG y, bool IsShowImmd, const DirectX::XMMATRIX& transformMatrix) {
	if (_x!=x || _y!=y) {
		_x = x;
		_y = y;
	}

	if (IsShowImmd) {
		_isCaretOn = true;
		_oldTick = ::GetTickCount();
		_1stFrame = true;
	}

	_transformMatrix = transformMatrix;
}

void UICaret::DrawAnimation() {
	if (!_isCaretOn) {
		return;
	}

	UIRect(_x, _y-_height/2, _x+_width-1, _y+_height/2, _z)(_color, 255, _transformMatrix);
}

void UIShowCaret(float z, ULONG width, ULONG height, const UIColor& color) {
	UICaret::GetSingletonInstance()->SetCaret(z, width, height, color);
}

void UIHideCaret() {
	UICaret::GetSingletonInstance()->HideCaret();
}

void UISetCaretPos(ULONG x, ULONG y, bool IsShowImmd, const DirectX::XMMATRIX& transformMatrix) {
	UICaret::GetSingletonInstance()->SetPos(x, y, IsShowImmd, transformMatrix);
}



UIAnimateHelp::UIAnimateHelp() {
	_frameIndex = 1;
	_maxFrame = 0;
	_1stFrame = false;
}

bool UIAnimateHelp::IsAnimationRun() {
	return _frameIndex<=_maxFrame;
}

bool UIAnimateHelp::UpdateAnimation() {
	if (_1stFrame) {
		_1stFrame = false;
		return true;
	}

	++_frameIndex;
	return true;
}

void UIAnimateHelp::PlayAnimate(int maxFrame) {
	_frameIndex = 1;
	_maxFrame = maxFrame;
	_1stFrame = true;

	UIRegisterAnimate(this);
}

void UIAnimateEffectHitDrum::InitializeCommon() {
	_hitPower = 0.8f;
}

UIAnimateEffectHitDrum::UIAnimateEffectHitDrum() {
	_loadImageWay = 0;

	InitializeCommon();
}

void UIAnimateEffectHitDrum::PlayHitDrumAnimate(int maxFrame) {
	PlayAnimate(maxFrame);
}

void UIAnimateEffectHitDrum::SetHitPower(float v) {
	_hitPower=v;
}

void UIAnimateEffectHitDrum::SetDLLPath(std::wstring path, const UIColor& colorKey) {
	_dllPath = path;
	_colorKey = colorKey;
	_loadImageWay = 1;
}

void UIAnimateEffectHitDrum::SetDLLID(int id) {
	_imageID = id;
}

void UIAnimateEffectHitDrum::SetImagePath(std::wstring path, const UIColor& colorKey) {
	_imagePath = path;
	_colorKey = colorKey;
	_loadImageWay = 2;
}

void UIAnimateEffectHitDrum::DrawHitDrumAnimate(int centerX, int centerY, float _z, const DirectX::XMMATRIX& transformMatrix) {
	if ((_loadImageWay==1 && _imageID==-1) || 
	    (_loadImageWay==2 && _imagePath==L"") || 
		(_loadImageWay!=1 && _loadImageWay!=2)) {
		return;
	}

	UIImage image;
	if (_loadImageWay==1) {
		image = UIImage(_dllPath.c_str(), _imageID, _colorKey, _z);
	} else {
		image = UIImage(_imagePath, _colorKey, _z);
	}
	
	image.operator()(centerX, centerY, 1.f, 255, transformMatrix);
	image.operator()(centerX, centerY, 1+_hitPower*_frameIndex/_maxFrame, static_cast<UCHAR>(255-250*_frameIndex/_maxFrame), transformMatrix);
}

void UIAnimateEffectHitDrum::DrawHitDrumAnimate(RECT rc, float _z, const DirectX::XMMATRIX& transformMatrix) {
	if ((_loadImageWay==1 && _imageID==-1) || 
	    (_loadImageWay==2 && _imagePath==L"") || 
		(_loadImageWay!=1 && _loadImageWay!=2)) {
		return;
	}

	UIImage image;
	if (_loadImageWay==1) {
		image = UIImage(_dllPath.c_str(), _imageID, _colorKey, _z);
	} else {
		image = UIImage(_imagePath, _colorKey, _z);
	}
	
	image.operator()(rc, 1.f, 255, transformMatrix);
	image.operator()(rc, 1+_hitPower*_frameIndex/_maxFrame, static_cast<UCHAR>(255-250*_frameIndex/_maxFrame), transformMatrix);
}

void UIAnimateEffectHitDrum::DrawFrameHitDrumAnimate(RECT rc, int corner, float _z, const DirectX::XMMATRIX& transformMatrix) {
	if ((_loadImageWay==1 && _imageID==-1) || 
	    (_loadImageWay==2 && _imagePath==L"") || 
		(_loadImageWay!=1 && _loadImageWay!=2)) {
		return;
	}

	//
	UISlicedImage frameImage;
	if (_loadImageWay==1) {
		frameImage = UISlicedImage(_dllPath.c_str(), _imageID, _colorKey, corner, corner, corner, corner, _z);
	} else {
		frameImage = UISlicedImage(_imagePath, _colorKey, corner, corner, corner, corner, _z);
	}
	
	frameImage.operator()(rc, 255, transformMatrix);
	frameImage.operator()(ScaleRect()(rc, 1+_hitPower*_frameIndex/_maxFrame), static_cast<UCHAR>(255-250*_frameIndex/_maxFrame), transformMatrix);
}
