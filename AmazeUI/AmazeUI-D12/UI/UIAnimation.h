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
	void SetCaret(ULONG width, ULONG height, const UIColor& color);
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
void UIShowCaret(ULONG width, ULONG height, const UIColor& color);
void UIHideCaret();
void UISetCaretPos(ULONG x, ULONG y, bool IsShowImmd=true, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());






// Animation effect helper class
class UIAnimateFrameHelp : public UIAnimationBase {
public:
	UIAnimateFrameHelp() = default;

	void PlayAnimate(int maxFrame = 5);

protected:
	bool IsAnimationRun();
	bool UpdateAnimation();

	int _maxFrame = 1;
	int _frameIndex = 0;
	bool _isAnimationStarted = false; // Animation start flag
};

// Time-based animation helper class
class UIAnimateSecondHelp : public UIAnimationBase {
public:
	UIAnimateSecondHelp() = default;

	void PlayAnimate(float duration = 2.0f);

protected:
	bool IsAnimationRun();
	bool UpdateAnimation();
	float GetDeltaTime() const { return _deltaTime; }

	float _duration = 2.0f;           // Animation duration in seconds
	float _elapsedTime = 0.0f;        // Elapsed time in seconds  
	float _deltaTime = 0.0f;          // Time since last update
	DWORD _lastTickTime = 0;          // Last update timestamp
	bool _isAnimationStarted = false; // Animation start flag
};



// Animation common effects
class UIAnimateEffectHitDrum : public UIAnimateFrameHelp {
public:
	UIAnimateEffectHitDrum();
	~UIAnimateEffectHitDrum() = default;

	void PlayHitDrumAnimate(int maxFrame=10);

	/*--------------------------------- Hit drum effect ---------------------------------*/
public:
	void SetHitPower(float v);

	void DrawHitDrumAnimate(UIImage& image, int centerX, int centerY, float scale, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void DrawHitDrumAnimate(UIImage& image, RECT rc, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void DrawSlicedHitDrumAnimate(UISlicedImage& slicedImage, RECT rc, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

protected:
	float _hitPower;			// Image magnification coefficient
};





// Base particle system class  
class UIAnimateParticle : public UIAnimateSecondHelp {
public:
	UIAnimateParticle() = default;
	virtual ~UIAnimateParticle() = default;
	
	// Emitter shape and position controls
	void SetEmitterPosition(const UIPointFloat3& position) { _emitterPosition = position; }
	void SetEmitterDirection(const UIPointFloat3& direction);
	void SetEmissionRadius(float radius) { _emissionRadius = radius; }
	void SetConeAngle(float angleDegrees) { _coneAngle = angleDegrees; }
	void SetConeHeight(float height) { _coneHeight = height; }
	void SetSurfaceEmissionOnly(bool surfaceOnly) { _surfaceEmissionOnly = surfaceOnly; }
	
	// Emission and particle controls
	void SetEmissionRate(float rate) { _emissionRate = rate; }
	void SetMaxParticles(int maxCount) { _maxParticles = maxCount; }
	void SetSpeedRange(float minSpeed, float maxSpeed) { _speedMin = minSpeed; _speedMax = maxSpeed; }
	void SetParticleSize(float size) { _particleSize = size; }
	void SetParticleScale(float scale) { _particleScale = scale; }
	
	// Physics controls
	void SetTurbulence(float turbulence) { _turbulence = turbulence; }
	void SetWindForce(const UIPointFloat3& wind) { _windForce = wind; }
	void SetGravity(float gravity) { _gravity = gravity; }
	void SetAirResistance(float resistance) { _airResistance = resistance; }

protected:
	// Common particle structure
	struct BaseParticle {
		UIPointFloat3 _position;		// world space position
		UIPointFloat3 _velocity;		// world space velocity
		UIPointFloat3 _acceleration;	// world space acceleration
		UIColor _color;
		float _life;            		// Life remaining [0, 1]
		float _maxLife;         		// Maximum life time
		float _initialSize;     		// Initial size for proper scaling
		float _currentSize;     		// Current size
		UCHAR _alpha;
	};

	// Common particle operations
	virtual void EmitParticles() = 0;
	virtual void UpdateParticles() = 0;
	virtual void DrawParticles() = 0;

	// Utility functions
	float RandomFloat(float min, float max);
	float Lerp(float a, float b, float t);
	void NormalizeVector(UIPointFloat3& vector);
	float DotProduct(const UIPointFloat3& a, const UIPointFloat3& b);
	UIPointFloat3 CrossProduct(const UIPointFloat3& a, const UIPointFloat3& b);
	
	// Core emission algorithm - generates position, direction and speed in world space
	void GenerateEmissionPoint(UIPointFloat3& outPosition, UIPointFloat3& outVelocity);

	// Emitter shape properties (all in world space)
	UIPointFloat3 _emitterPosition = { 0.0f, 0.0f, 0.0f };		// Emitter center position in world space
	UIPointFloat3 _emitterDirection = { 1.0f, 0.0f, 0.0f };		// Cone axis direction (normalized)
	float _emissionRadius = 0.1f;								// Base emission radius
	float _coneAngle = 30.0f;									// Cone angle in degrees
	float _coneHeight = 2.0f;									// Cone height
	bool _surfaceEmissionOnly = false;							// Emit only from cone surface

	// Emission properties
	float _emissionRate = 100.0f;								// Particles emitted per second
	int _maxParticles = 1000;									// Maximum particle count
	float _emissionAccumulator = 0.0f;							// Fractional particle accumulator

	// Particle properties
	float _speedMin = 1.0f;										// Minimum emission speed
	float _speedMax = 3.0f;										// Maximum emission speed
	float _particleSize = 1.0f;									// Base particle size
	float _particleScale = 1.0f;								// Particle size multiplier

	// Physics properties
	float _turbulence = 0.5f;									// Turbulence strength
	UIPointFloat3 _windForce = { 0.0f, 0.0f, 0.0f };			// Wind force vector
	float _gravity = 0.0f;										// Gravity strength
	float _airResistance = 0.02f;								// Air resistance coefficient

	// Rendering
	UICameraBase* _pCamera = nullptr;							// Camera reference for rendering

	void DrawAnimation() override;
};

// Flame particle effect
class UIAnimateParticleFlame : public UIAnimateParticle {
public:
	UIAnimateParticleFlame();
	~UIAnimateParticleFlame() = default;

	void PlayFlameAnimate(const UIPointFloat3& position, UICameraBase* pCamera, int maxFrame = 60);

	/*--------------------------------- Flame particle effect ---------------------------------*/
public:
	void SetFlameIntensity(float intensity);    // Set flame intensity (affects size and count)
	void SetFlameHeight(float height);          // Set flame height
	void SetFlameSpread(float angle);           // Set flame spread angle (degrees)
	void SetFlameShape(float narrowness);       // Set flame shape narrowness
	void SetFlameWind(const UIPointFloat3& wind); // Set wind direction

protected:
	// Override base particle methods
	void EmitParticles() override;
	void UpdateParticles() override;
	void DrawParticles() override;

private:
	struct FlameParticle : public BaseParticle {
		float _initialSpeed;    // Initial upward speed
		float _turbulence;      // Random movement factor
	};

	std::vector<FlameParticle> _particles;

	// Flame-specific parameters
	float _flameIntensity;  // Flame intensity multiplier
	float _flameHeight;     // Maximum flame height
	float _flameSpreadAngle; // Flame spread angle in radians
	float _flameNarrowness; // Shape narrowness factor
	UIPointFloat3 _windDirection; // Wind effect direction

	// Helper functions
	UIColor GetFlameColor(float lifeRatio);
};