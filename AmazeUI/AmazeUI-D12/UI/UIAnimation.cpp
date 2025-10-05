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



bool UIAnimateFrameHelp::IsAnimationRun() {
	if (!_isAnimationStarted) {
		return false;
	}

	_isAnimationStarted = _frameIndex <= _maxFrame;
	return _isAnimationStarted;
}

bool UIAnimateFrameHelp::UpdateAnimation() {
	return (++_frameIndex) <= _maxFrame;
}

void UIAnimateFrameHelp::PlayAnimate(int maxFrame) {
	_frameIndex = 0;
	_maxFrame = maxFrame;
	_isAnimationStarted = true;

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

/*--------------------------------- UIAnimateSecondHelp ---------------------------------*/
void UIAnimateSecondHelp::PlayAnimate(float duration) {
	_duration = duration;
	_elapsedTime = 0.0f;
	_deltaTime = 0.0f;
	_lastTickTime = GetTickCount();
	_isAnimationStarted = true;
	UIRegisterAnimate(this);
}

bool UIAnimateSecondHelp::IsAnimationRun() {
	return _isAnimationStarted && (_elapsedTime < _duration);
}

bool UIAnimateSecondHelp::UpdateAnimation() {
	if (!_isAnimationStarted) {
		return false;
	}

	DWORD currentTime = GetTickCount();
	_deltaTime = (currentTime - _lastTickTime) / 1000.0f; // Convert to seconds
	_lastTickTime = currentTime;
	_elapsedTime += _deltaTime;

	return IsAnimationRun();
}





/*--------------------------------- UIAnimateParticle ---------------------------------*/
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

void UIAnimateParticle::SetEmitterDirection(const UIPointFloat3& direction) {
	_emitterDirection = direction;
	NormalizeVector(_emitterDirection);
}

void UIAnimateParticle::NormalizeVector(UIPointFloat3& vector) {
	float length = sqrt(vector._x * vector._x + vector._y * vector._y + vector._z * vector._z);
	if (length > 0.0f) {
		vector._x /= length;
		vector._y /= length;
		vector._z /= length;
	}
}

float UIAnimateParticle::DotProduct(const UIPointFloat3& a, const UIPointFloat3& b) {
	return a._x * b._x + a._y * b._y + a._z * b._z;
}

UIPointFloat3 UIAnimateParticle::CrossProduct(const UIPointFloat3& a, const UIPointFloat3& b) {
	return UIPointFloat3(
		a._y * b._z - a._z * b._y,
		a._z * b._x - a._x * b._z,
		a._x * b._y - a._y * b._x
	);
}

// Core emission algorithm — compute cone-shaped particle emission directly in world-space coordinates.
void UIAnimateParticle::GenerateEmissionPoint(UIPointFloat3& outPosition, UIPointFloat3& outVelocity) {
	// 1. Randomly pick a position inside a circular emission area
	float theta = RandomFloat(0, 2 * 3.14159265f);
	float radiusRatio = sqrt(RandomFloat(0, 1.0f)); // square-root distribution yields uniform sampling over the disk
	float radius = _emissionRadius * radiusRatio;

	// 2. Compute emission height along the cone axis
	float height;
	if (_surfaceEmissionOnly) {
		height = _coneHeight; // emit only from the cone apex (surface)
	} else {
		height = RandomFloat(0, _coneHeight);
	}

	// 3. Compute the cone radius at the chosen height
	float heightRatio = height / _coneHeight;
	float coneRadiusAtHeight = radius * (1.0f - heightRatio * tan(_coneAngle * 3.14159265f / 360.0f));

	// 4. Construct the local-space position (cone aligned with +Y)
	UIPointFloat3 basePos(
		coneRadiusAtHeight * cos(theta),
		height,
		coneRadiusAtHeight * sin(theta)
	);

	// 5. If emitter direction != default up (0,1,0), rotate the point to align with emitter direction
	UIPointFloat3 defaultUp(0.0f, 1.0f, 0.0f);
	if (abs(DotProduct(_emitterDirection, defaultUp) - 1.0f) > 0.001f) {
		// compute rotation axis and angle
		UIPointFloat3 rotAxis = CrossProduct(defaultUp, _emitterDirection);
		float rotAngle = acos(DotProduct(defaultUp, _emitterDirection));

		// simplified Rodrigues rotation (sufficient for common cases)
		if (abs(rotAngle) > 0.001f) {
			NormalizeVector(rotAxis);
			float cosA = cos(rotAngle);
			float sinA = sin(rotAngle);

			// Rodrigues: v' = v*cos(θ) + (k×v)*sin(θ) + k*(k·v)*(1-cos(θ))
			UIPointFloat3 kCrossV = CrossProduct(rotAxis, basePos);
			float kDotV = DotProduct(rotAxis, basePos);

			basePos._x = basePos._x * cosA + kCrossV._x * sinA + rotAxis._x * kDotV * (1 - cosA);
			basePos._y = basePos._y * cosA + kCrossV._y * sinA + rotAxis._y * kDotV * (1 - cosA);
			basePos._z = basePos._z * cosA + kCrossV._z * sinA + rotAxis._z * kDotV * (1 - cosA);
		}
	}

	// 6. Compute final world-space position
	outPosition = UIPointFloat3(
		_emitterPosition._x + basePos._x,
		_emitterPosition._y + basePos._y,
		_emitterPosition._z + basePos._z
	);

	// 7. Generate emission direction inside cone (cone-shaped angular distribution)
	float coneAngleRad = _coneAngle * 3.14159265f / 180.0f;
	float phi = RandomFloat(0, 2 * 3.14159265f);
	float cosTheta = RandomFloat(cos(coneAngleRad), 1.0f);
	float sinTheta = sqrt(1 - cosTheta * cosTheta);

	// local base direction (pointing up along +Y)
	UIPointFloat3 baseDir(
		sinTheta * cos(phi),
		cosTheta,
		sinTheta * sin(phi)
	);

	// rotate direction into emitter orientation if needed
	if (abs(DotProduct(_emitterDirection, defaultUp) - 1.0f) > 0.001f) {
		UIPointFloat3 rotAxis = CrossProduct(defaultUp, _emitterDirection);
		float rotAngle = acos(DotProduct(defaultUp, _emitterDirection));

		if (abs(rotAngle) > 0.001f) {
			NormalizeVector(rotAxis);
			float cosA = cos(rotAngle);
			float sinA = sin(rotAngle);

			UIPointFloat3 kCrossV = CrossProduct(rotAxis, baseDir);
			float kDotV = DotProduct(rotAxis, baseDir);

			baseDir._x = baseDir._x * cosA + kCrossV._x * sinA + rotAxis._x * kDotV * (1 - cosA);
			baseDir._y = baseDir._y * cosA + kCrossV._y * sinA + rotAxis._y * kDotV * (1 - cosA);
			baseDir._z = baseDir._z * cosA + kCrossV._z * sinA + rotAxis._z * kDotV * (1 - cosA);
		}
	}

	// 8. Compute final velocity vector
	float speed = RandomFloat(_speedMin, _speedMax);
	outVelocity = UIPointFloat3(
		baseDir._x * speed,
		baseDir._y * speed,
		baseDir._z * speed
	);
}

/*--------------------------------- UIAnimateParticleFlame ---------------------------------*/
UIAnimateParticleFlame::UIAnimateParticleFlame() {
	_flameIntensity = 1.0f;
	_flameHeight = 2.0f;
	_flameSpreadAngle = 30.0f;
	_flameNarrowness = 1.0f;
	_windDirection = UIPointFloat3(0.0f, 0.0f, 0.0f);
}

void UIAnimateParticleFlame::PlayFlameAnimate(const UIPointFloat3& position, UICameraBase3D* pCamera, int maxFrame) {
	_emitterPosition = position;
	_pCamera = pCamera;
	_particles.clear();
	
	// Set flame-specific emitter parameters
	_coneAngle = _flameSpreadAngle;
	_coneHeight = _flameHeight;
	_speedMin = 1.0f * _flameHeight;
	_speedMax = 2.5f * _flameHeight;

	// Convert to time-based animation (assume 60 FPS)
	float duration = maxFrame / 60.0f;
	PlayAnimate(duration);
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
	// Time-based particle emission
	float deltaTime = GetDeltaTime();
	if (deltaTime <= 0.0f) return;

	// Accumulate emission count based on rate and intensity
	_emissionAccumulator += _emissionRate * _flameIntensity * deltaTime;
	int particlesToEmit = static_cast<int>(_emissionAccumulator);
	_emissionAccumulator -= particlesToEmit;

	for (int i = 0; i < particlesToEmit; ++i) {
		// Check particle count limit
		if (_particles.size() >= _maxParticles) {
			break;
		}

		FlameParticle particle;

		// Generate position and velocity using the emission algorithm
		UIPointFloat3 emitPos, emitVel;
		GenerateEmissionPoint(emitPos, emitVel);

		particle._position = emitPos;
		particle._velocity = emitVel;
		particle._initialSpeed = sqrt(emitVel._x * emitVel._x + emitVel._y * emitVel._y + emitVel._z * emitVel._z);

		// Add flame-specific turbulence
		particle._velocity._x += RandomFloat(-0.3f, 0.3f) * _turbulence;
		particle._velocity._z += RandomFloat(-0.3f, 0.3f) * _turbulence;

		// Physical properties
		particle._acceleration = UIPointFloat3(_windForce._x, _gravity, _windForce._z);

		// Lifecycle
		particle._life = 1.0f;
		particle._maxLife = RandomFloat(1.5f, 3.0f);

		// Size properties
		particle._initialSize = RandomFloat(0.05f, 0.15f) * _particleSize * _particleScale * _flameIntensity;
		particle._currentSize = particle._initialSize;
		particle._turbulence = RandomFloat(0.3f, 0.8f) * _turbulence;

		// Visual properties
		particle._color = GetFlameColor(0.0f); // initial color
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