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
class UIAnimateHelp : public UIAnimationBase {
public:
	UIAnimateHelp();

	void PlayAnimate(int maxFrame = 5);

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

	void DrawHitDrumAnimate(UIImage& image, int centerX, int centerY, float scale, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void DrawHitDrumAnimate(UIImage& image, RECT rc, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());
	void DrawSlicedHitDrumAnimate(UISlicedImage& slicedImage, RECT rc, const DirectX::XMMATRIX& transformMatrix=DirectX::XMMatrixIdentity());

protected:
	float _hitPower;			// Image magnification coefficient
};




// Base particle system class
class UIAnimateParticle : public UIAnimateHelp {
public:
	UIAnimateParticle();
	virtual ~UIAnimateParticle() = default;
	
	// Public setters for common properties
	void SetEmissionRate(float rate) { _emissionRate = rate; }
	void SetParticleScale(float scale) { _particleScale = scale; }
	void SetMaxParticles(int maxCount) { _maxParticles = maxCount; }
	void SetParticleSize(float size) { _particleSize = size; }
	void SetTurbulence(float turbulence) { _turbulence = turbulence; }
	void SetEmissionAngle(float angle) { _emissionAngle = angle; }
	
	// Advanced particle controls
	void SetWindForce(const UIPointFloat3& wind) { _windForce = wind; }
	void SetGravity(float gravity) { _gravity = gravity; }
	void SetAirResistance(float resistance) { _airResistance = resistance; }

protected:
	// Common particle structure
	struct BaseParticle {
		UIPointFloat3 _position;
		UIPointFloat3 _velocity;
		UIPointFloat3 _acceleration;
		UIColor _color;
		float _life;            // Life remaining [0, 1]
		float _maxLife;         // Maximum life time
		float _initialSize;     // Initial size for proper scaling
		float _currentSize;     // Current size
		UCHAR _alpha;
	};

	// Common particle operations
	virtual void EmitParticles() = 0;
	virtual void UpdateParticles() = 0;
	virtual void DrawParticles() = 0;

	// Utility functions
	float RandomFloat(float min, float max);
	float Lerp(float a, float b, float t);

	// Common properties
	UIPointFloat3 _emitterPosition;
	UICameraBase* _pCamera;
	float _emissionRate;
	float _deltaTime;
	float _particleScale;     // 粒子大小倍数
	int _maxParticles;        // 最大粒子数量
	float _particleSize;      // 粒子基础大小
	float _turbulence;        // 湍流强度
	float _emissionAngle;     // 发射角度范围
	
	// Advanced physics properties
	UIPointFloat3 _windForce; // 风力向量
	float _gravity;           // 重力强度
	float _airResistance;     // 空气阻力

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


