#include "UIAnimation.h"
#include "UIElement.h"
#include <cstdlib>
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

UIAnimateParticle::UIAnimateParticle() {
	_emitterPosition = UIPointFloat3(0.0f, 0.0f, 0.0f);
	_pCamera = nullptr;
	_emissionRate = 2.0f;
	_deltaTime = 1.0f / 60.0f; // Assume 60fps
	_particleScale = 1.0f;
	_maxParticles = 150; // 增加到150个粒子
	_particleSize = 1.0f;
	_turbulence = 0.5f;
	_emissionAngle = 45.0f; // degrees
	_windForce = UIPointFloat3(0.0f, 0.0f, 0.0f);
	_gravity = -0.8f;
	_airResistance = 0.02f;
}

void UIAnimateParticle::DrawAnimation() {
	if (_pCamera == nullptr) {
		return;
	}

	// Emit new particles
	EmitParticles();
	
	// Update existing particles
	UpdateParticles();
	
	// Draw particles
	DrawParticles();
}

float UIAnimateParticle::RandomFloat(float min, float max) {
	return min + (static_cast<float>(rand()) / RAND_MAX) * (max - min);
}

float UIAnimateParticle::Lerp(float a, float b, float t) {
	return a + t * (b - a);
}

UIAnimateParticleFlame::UIAnimateParticleFlame() {
	_flameIntensity = 1.0f;
	_flameHeight = 2.0f;
}

void UIAnimateParticleFlame::PlayFlameAnimate(const UIPointFloat3& position, UICameraBase* pCamera, int maxFrame) {
	_emitterPosition = position;
	_pCamera = pCamera;
	_particles.clear();
	PlayAnimate(maxFrame);
}

void UIAnimateParticleFlame::SetFlameIntensity(float intensity) {
	_flameIntensity = intensity;
}

void UIAnimateParticleFlame::SetFlameHeight(float height) {
	_flameHeight = height;
}

void UIAnimateParticleFlame::DrawParticles() {
	// Draw each particle as a 3D circle
	for (const auto& particle : _particles) {
		UIPointFloat3 pos(particle._position._x, particle._position._y, particle._position._z);
		UICircle3D circle(pos, particle._currentSize);
		circle(particle._color, _pCamera);
	}
}

void UIAnimateParticleFlame::EmitParticles() {
	// Limit particle count to maxParticles
	if (_particles.size() >= _maxParticles) {
		return;
	}
	
	// Enhanced emission rate for more particles - 增加粒子数量
	int particlesToEmit = static_cast<int>(_emissionRate * _flameIntensity * 2.5f);
	
	// Apply emission angle range for flame shape control
	float angleRadians = _emissionAngle * 3.14159f / 180.0f;
	
	for (int i = 0; i < particlesToEmit; ++i) {
		// Respect max particle limit
		if (_particles.size() >= _maxParticles) {
			break;
		}
		
		FlameParticle particle;
		
		// Enhanced emission area for wider flame base - 增大发射区域
		float baseRadius = 0.15f * _particleScale;
		float offsetX = RandomFloat(-baseRadius, baseRadius);
		float offsetZ = RandomFloat(-baseRadius, baseRadius);
		particle._position = UIPointFloat3(
			_emitterPosition._x + offsetX,
			_emitterPosition._y,
			_emitterPosition._z + offsetZ
		);
		
		// Enhanced velocity with emission angle control
		particle._initialSpeed = RandomFloat(1.5f, 3.0f) * _flameHeight;
		
		// Apply emission angle for flame shape - 火焰形状控制
		float randomAngle = RandomFloat(-angleRadians / 2, angleRadians / 2);
		float velocityX = sin(randomAngle) * particle._initialSpeed * 0.3f;
		float velocityZ = cos(randomAngle) * RandomFloat(-0.2f, 0.2f);
		
		particle._velocity = UIPointFloat3(
			velocityX + RandomFloat(-0.2f, 0.2f) * _turbulence, // Enhanced turbulence
			particle._initialSpeed,   // Upward movement
			velocityZ + RandomFloat(-0.2f, 0.2f) * _turbulence  // Enhanced turbulence
		);
		
		// Gravity and air resistance
		particle._acceleration = UIPointFloat3(0.0f, -0.8f, 0.0f);
		
		// Enhanced lifetime variation
		particle._life = 1.0f;
		particle._maxLife = RandomFloat(0.8f, 1.2f);
		
		// Enhanced particle size - 增大粒子尺寸
		float baseSizeMultiplier = _particleSize * _particleScale * 2.5f; // 2.5倍基础大小
		particle._initialSize = RandomFloat(0.08f, 0.20f) * _flameIntensity * baseSizeMultiplier;
		particle._currentSize = particle._initialSize;
		particle._turbulence = RandomFloat(0.5f, 1.5f) * _turbulence;
		
		// Initial color and alpha
		particle._color = GetFlameColor(1.0f);
		particle._alpha = 255;
		
		_particles.push_back(particle);
	}
}

void UIAnimateParticleFlame::UpdateParticles() {
	// Update all particles
	for (auto it = _particles.begin(); it != _particles.end();) {
		FlameParticle& particle = *it;
		
		// Update physics
		particle._velocity._x += particle._acceleration._x * _deltaTime;
		particle._velocity._y += particle._acceleration._y * _deltaTime;
		particle._velocity._z += particle._acceleration._z * _deltaTime;
		
		// Add turbulence to horizontal movement
		particle._velocity._x += RandomFloat(-0.1f, 0.1f) * particle._turbulence * _deltaTime;
		particle._velocity._z += RandomFloat(-0.1f, 0.1f) * particle._turbulence * _deltaTime;
		
		particle._position._x += particle._velocity._x * _deltaTime;
		particle._position._y += particle._velocity._y * _deltaTime;
		particle._position._z += particle._velocity._z * _deltaTime;
		
		// Update life
		particle._life -= _deltaTime;
		
		// Update color and alpha based on life
		float lifeRatio = particle._life / particle._maxLife;
		particle._color = GetFlameColor(lifeRatio);
		particle._alpha = static_cast<UCHAR>(255 * lifeRatio);
		
		// Fixed size calculation - linear interpolation instead of exponential growth
		float sizeMultiplier = Lerp(1.0f, 1.8f, 1.0f - lifeRatio); // Grow from 1x to 1.8x over lifetime
		particle._currentSize = particle._initialSize * sizeMultiplier;
		
		// Remove dead particles
		if (particle._life <= 0.0f) {
			it = _particles.erase(it);
		} else {
			++it;
		}
	}
}

UIColor UIAnimateParticleFlame::GetFlameColor(float lifeRatio) {
	if (lifeRatio > 0.7f) {
		// White/Yellow hot core
		return UIColor(255, 255, 200);
	} else if (lifeRatio > 0.4f) {
		// Orange flames
		return UIColor(255, 150, 50);
	} else {
		// Red flames fading
		return UIColor(255, 50, 0);
	}
}

