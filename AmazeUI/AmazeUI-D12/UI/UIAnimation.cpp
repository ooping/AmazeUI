#include "UIAnimation.h"
#include "UIElement.h"
//#include "UI3D.h"

using namespace std;
using namespace UIShape2D;

UIAnimationManage::~UIAnimationManage() {
	UICaret::GetSingletonInstance()->DestroySingletonInstance();
}

void UIAnimationManage::AddAnimation(UIAnimationBase* pAnimateObj) {
	if (pAnimateObj != NULL) {
		if (find(_animationList.begin(), _animationList.end(), pAnimateObj) == _animationList.end()) {
			_animationList.push_back(pAnimateObj);
		}
	}
}

void UIAnimationManage::DelAnimation(UIAnimationBase* pAnimateObj) {
	if (pAnimateObj != NULL) {
		auto it = find(_animationList.begin(), _animationList.end(), pAnimateObj);
		if (it != _animationList.end()) {
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
	for (auto i = _animationList.begin(); i != _animationList.end(); ++i) {
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
	_z = 0.0f;

	_oldTick = 0;

	_isCaretOn = false;
}

bool UICaret::IsAnimationRun() {
	return true;
}

bool UICaret::UpdateAnimation() {
	DWORD nowTick = ::GetTickCount();
	DWORD dxTick = nowTick - _oldTick;						// ?????
	
	if (dxTick > 600) {
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

void UICaret::SetCaret(ULONG width, ULONG height, const UIColor& color) {
	_width = width;
	_height = height;
	_color = color;
	_isCaretOn = true;
	_oldTick = ::GetTickCount();
	_1stFrame = false;
	
	UIAnimationManage::GetSingletonInstance()->AddAnimation(UICaret::GetSingletonInstance());
}

void UICaret::SetPos(ULONG x, ULONG y, bool IsShowImmd, const DirectX::XMMATRIX& transformMatrix) {
	if (_x != x || _y != y) {
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

	UIRect(_x, _y - _height / 2, _x + _width - 1, _y + _height / 2, _z)(_color, 255, _transformMatrix);
}

void UIShowCaret(ULONG width, ULONG height, const UIColor& color) {
	UICaret::GetSingletonInstance()->SetCaret(width, height, color);
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
	return _frameIndex <= _maxFrame;
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

UIAnimateEffectHitDrum::UIAnimateEffectHitDrum() {
	_hitPower = 0.8f;
}

void UIAnimateEffectHitDrum::PlayHitDrumAnimate(int maxFrame) {
	PlayAnimate(maxFrame);
}

void UIAnimateEffectHitDrum::SetHitPower(float v) {
	_hitPower = v;
}

void UIAnimateEffectHitDrum::DrawHitDrumAnimate(UIImage& image, int centerX, int centerY, float scale, const DirectX::XMMATRIX& transformMatrix) {
	image.operator()(centerX, centerY, scale, 255, transformMatrix);
	image.operator()(centerX, centerY, scale + (_hitPower * _frameIndex / _maxFrame), static_cast<UCHAR>(255 - (250 * _frameIndex / _maxFrame)), transformMatrix);
}

void UIAnimateEffectHitDrum::DrawHitDrumAnimate(UIImage& image, RECT rc, const DirectX::XMMATRIX& transformMatrix) {
	image.operator()(rc, 1.0f, 255, transformMatrix);
	image.operator()(rc, 1.0f + (_hitPower * _frameIndex / _maxFrame), static_cast<UCHAR>(255 - (250 * _frameIndex / _maxFrame)), transformMatrix);
}

void UIAnimateEffectHitDrum::DrawSlicedHitDrumAnimate(UISlicedImage& slicedImage, RECT rc, const DirectX::XMMATRIX& transformMatrix) {
	slicedImage.operator()(rc, 255, transformMatrix);
	slicedImage.operator()(ScaleRect()(rc, 1.0f + (_hitPower * _frameIndex / _maxFrame)), static_cast<UCHAR>(255 - (250 * _frameIndex / _maxFrame)), transformMatrix);
}
