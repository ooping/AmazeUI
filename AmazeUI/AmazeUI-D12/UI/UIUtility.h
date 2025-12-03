#pragma once

#include "..\CORE\Common.h"

/*-------------------------------------------------- DirectX 12 --------------------------------------------------*/
#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <DirectXMath.h>
#include <DirectXColors.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <pix.h>

/*-------------------------------------------------- DirectX Tool Kit 12 --------------------------------------------------*/
#include "Audio.h"
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PrimitiveBatch.h"
#include "RenderTargetState.h"
#include "ResourceUploadBatch.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "WICTextureLoader.h"






/*-------------------------------------------------- Types --------------------------------------------------*/
// using UIVEC2 = DirectX::XMFLOAT2;
// using UIVEC3 = DirectX::XMFLOAT3;
// using UIVEC4 = DirectX::XMFLOAT4;
// using UIMX4x4 = DirectX::XMFLOAT4X4;
// using UIMX3x3 = DirectX::XMFLOAT3X3;





/*-------------------------------------------------- macros --------------------------------------------------*/
#define IDR_MAIN_BAR					80001
#define IDB_HOT_EFFECT					80002
#define IDB_HOME_MENU					80003
#define IDB_HOT_EFFECT2					80004

#define IDB_BUTTON1_NORMAL              80011
#define IDB_BUTTON1_HOT                 80012
#define IDB_BUTTON1_DOWN                80013

#define IDB_CHECKBOX1_NOR     			80020
#define IDB_CHECKBOX1_HOT				80021
#define IDB_CHECKBOX1_TICK_NOR			80022
#define IDB_CHECKBOX1_TICK_HOT			80023

#define IDB_ARROW1_DOWN					80032

#define IDB_ARROW2_UP					80035
#define IDB_ARROW2_DOWN					80036
#define IDB_ARROW2_LEFT					80037
#define IDB_ARROW2_RIGHT				80038

#define IDB_OK							80041
#define IDB_CLOSE						80042


/*-------------------------------------------------- vector math --------------------------------------------------*/
namespace UIVectorMath {
    struct VectorFloat2 {
        float _x;
        float _y;

        VectorFloat2(DirectX::XMFLOAT2 vec) : _x(vec.x), _y(vec.y) {}
        VectorFloat2(float x = 0.0f, float y = 0.0f) : _x(x), _y(y) {}
        
        VectorFloat2 operator+(const VectorFloat2& other) const {
            return VectorFloat2(_x + other._x, _y + other._y);
        }
        VectorFloat2 operator-(const VectorFloat2& other) const {
            return VectorFloat2(_x - other._x, _y - other._y);
        }
        VectorFloat2 operator*(float scale) const {
            return VectorFloat2(_x * scale, _y * scale);
        }
        
        DirectX::XMFLOAT2 ToXMFLOAT2() const {
            return DirectX::XMFLOAT2(_x, _y);
        }
    };

    struct VectorFloat3 {
        float _x;
        float _y;
        float _z;

        VectorFloat3(DirectX::XMFLOAT3 vec) : _x(vec.x), _y(vec.y), _z(vec.z) {}
        VectorFloat3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : _x(x), _y(y), _z(z) {}
        
        VectorFloat3 operator+(const VectorFloat3& other) const {
            return VectorFloat3(_x + other._x, _y + other._y, _z + other._z);
        }
        VectorFloat3 operator-(const VectorFloat3& other) const {
            return VectorFloat3(_x - other._x, _y - other._y, _z - other._z);
        }
        VectorFloat3 operator*(float scale) const {
            return VectorFloat3(_x * scale, _y * scale, _z * scale);
        }

        DirectX::XMFLOAT3 ToXMFLOAT3() const {
            return DirectX::XMFLOAT3(_x, _y, _z);
        }
    };

    inline float Distance2D(const VectorFloat2& _p1, const VectorFloat2& _p2) {
        float _dx = _p1._x - _p2._x;
        float _dy = _p1._y - _p2._y;
        return sqrt(_dx * _dx + _dy * _dy);
    }

    inline float Distance3D(const VectorFloat3& _p1, const VectorFloat3& _p2) {
        float _dx = _p1._x - _p2._x;
        float _dy = _p1._y - _p2._y;
        float _dz = _p1._z - _p2._z;
        return sqrt(_dx * _dx + _dy * _dy + _dz * _dz);
    }

    // interpolation
    inline VectorFloat2 Lerp2D(const VectorFloat2& _a, const VectorFloat2& _b, float _t) {
        return _a + (_b - _a) * _t;
    }

    inline VectorFloat3 Lerp3D(const VectorFloat3& _a, const VectorFloat3& _b, float _t) {
        return _a + (_b - _a) * _t;
    }

    // dot product
    inline float Dot2D(const VectorFloat2& _a, const VectorFloat2& _b) {
        return _a._x * _b._x + _a._y * _b._y;
    }

    inline float Dot3D(const VectorFloat3& _a, const VectorFloat3& _b) {
        return _a._x * _b._x + _a._y * _b._y + _a._z * _b._z;
    }

    // Cross product (2D returns scalar, 3D returns vector)
    inline float Cross2D(const VectorFloat2& _a, const VectorFloat2& _b) {
        return _a._x * _b._y - _a._y * _b._x;
    }

    inline VectorFloat3 Cross3D(const VectorFloat3& _a, const VectorFloat3& _b) {
        return VectorFloat3(
            _a._y * _b._z - _a._z * _b._y,
            _a._z * _b._x - _a._x * _b._z,
            _a._x * _b._y - _a._y * _b._x
        );
    }

    // Vector length
    inline float Length2D(const VectorFloat2& _p) {
        return sqrt(_p._x * _p._x + _p._y * _p._y);
    }

    inline float Length3D(const VectorFloat3& _p) {
        return sqrt(_p._x * _p._x + _p._y * _p._y + _p._z * _p._z);
    }

    // Vector normalization
    inline VectorFloat2 Normalize2D(const VectorFloat2& _p) {
        float _length = Length2D(_p);
        if (_length > 0.0f) {
            return _p * (1.0f / _length);
        }
        return VectorFloat2(0.0f, 0.0f);
    }

    inline VectorFloat3 Normalize3D(const VectorFloat3& _p) {
        float _length = Length3D(_p);
        if (_length > 0.0f) {
            return _p * (1.0f / _length);
        }
        return VectorFloat3(0.0f, 0.0f, 0.0f);
    }
}

// Type aliases provided for backward compatibility
using UIVector2F = UIVectorMath::VectorFloat2;
using UIVector3F = UIVectorMath::VectorFloat3;


/*-------------------------------------------------- UI utility functions--------------------------------------------------*/



// Control Dynamic Scaling Auxiliary Class
/*	father window
-------------------------
|			|			|
|		   dy1			|
|			|			|
|		--------		|
|-dx1-	|child	|-dx2-	|
|		|window	|		|
|		--------		|
|			|			|
|		   dy2			|
|			|			|
-------------------------
*/
struct UILayoutCalc {
	// Setting x and y can only be done once.
	enum zoomMode {
		NO_ZOOM = 0,
		MOVE_X = 1,			// bit 1	dx2 unchanged dx1 changed, Constant size in control x direction
		MOVE_Y = 2,			// bit 2	dy2 unchanged dy1 changed, Controls constant in size in the y-direction
		SIZE_X = 4,			// bit 3	Both dx1 and dx2 are unchanged, controls change in size in the x-direction
		SIZE_Y = 8,			// bit 4	Both dy1 and dy2 are unchanged, control changes in size in the y-direction
		SCALE_X = 16,		// bit 5	dx1, dx2 are proportionally changed
		SCALE_Y = 32,		// bit 6	dy1, dy2 are proportionally changed
	};

	UILayoutCalc(int flag = SIZE_X | SIZE_Y);

	void SetLayoutMode(int flag);										// Setting the zoom mode
	void InitLayout(const UIRECT& parentRect, const UIRECT& rect);			// Initial calculation: Normal mode/directUI mode
	UIRECT CalcLayout(LONG cx, LONG cy);									// Calc new state

	int _zoomModeflag;		// Layout Mode

	UIRECT _parentRect;		// The area of the parent form
	UIRECT _rect;				// Control relative to parent form area
};


/*-------------------------------------------------- Color Helper --------------------------------------------------*/
class UIColor {
public:
    // invalid color
    UIColor() : _r(0), _g(0), _b(0), _a(0), valid(false) {}
    
    // constructor using integer values (0-255)
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    UIColor(T red, T green, T blue, T alpha = T(255)) 
        : _r(static_cast<uint8_t>(red)), 
          _g(static_cast<uint8_t>(green)), 
          _b(static_cast<uint8_t>(blue)), 
          _a(static_cast<uint8_t>(alpha)), 
          valid(true) {}

    // constructor using float values (0.0-1.0)
    UIColor(float red, float green, float blue, float alpha = 1.0f) 
        : _r(static_cast<uint8_t>(red * 255.0f)), 
          _g(static_cast<uint8_t>(green * 255.0f)), 
          _b(static_cast<uint8_t>(blue * 255.0f)), 
          _a(static_cast<uint8_t>(alpha * 255.0f)), 
          valid(true) {}

    // constructor using DirectX::XMVECTORF32
    constexpr UIColor(const DirectX::XMVECTORF32& color) 
        : _r(static_cast<uint8_t>(color.f[0] * 255.0f)), 
          _g(static_cast<uint8_t>(color.f[1] * 255.0f)), 
          _b(static_cast<uint8_t>(color.f[2] * 255.0f)), 
          _a(static_cast<uint8_t>(color.f[3] * 255.0f)), 
          valid(true) {}

	DirectX::XMVECTORF32 ToXMVECTORF32(uint8_t alpha) const { 
        return DirectX::XMVECTORF32{_r / 255.0f, _g / 255.0f, _b / 255.0f, alpha / 255.0f}; 
    }
    DirectX::XMVECTORF32 ToXMVECTORF32() const { 
        return DirectX::XMVECTORF32{_r / 255.0f, _g / 255.0f, _b / 255.0f, _a / 255.0f}; 
    }
    uint32_t ToRGBA() const { 
        return static_cast<uint32_t>(_r) << 24 | static_cast<uint32_t>(_g) << 16 | 
               static_cast<uint32_t>(_b) << 8 | static_cast<uint32_t>(_a); 
    }
    std::wstring ToWStringForU32() const { return L"(" + std::to_wstring(ToRGBA()) + L")"; }

	bool operator<(const UIColor& other) const { return ToRGBA() < other.ToRGBA(); }
    bool operator==(const UIColor& other) const { return _r == other._r && _g == other._g && _b == other._b && _a == other._a; }
    
    // check if the color is valid
    bool IsValid() const { return valid; }
    
    // color components
    uint8_t _r, _g, _b, _a;

    // predefined colors
    static const UIColor Invalid;
    static const UIColor Transparent;
    static const UIColor Black;
    static const UIColor White;
    static const UIColor Red;
    static const UIColor Green;
    static const UIColor Blue;
    static const UIColor Yellow;
    static const UIColor Gray;
    static const UIColor Orange;
    static const UIColor Purple;
    static const UIColor Pink;
    static const UIColor Gold;

    static const UIColor BackgroundPurple;
    static const UIColor CursorBlue;
    static const UIColor ScrollbarNormal;
    static const UIColor ScrollbarHover;

    static const UIColor PrimaryBlue;
    static const UIColor PrimaryGreen;
    static const UIColor PrimaryPurple;
    static const UIColor PrimaryPink;
    static const UIColor PrimaryGray;

    static const UIColor PrimaryBlueLight;
    static const UIColor PrimaryGreenLight;
    static const UIColor PrimaryPurpleLight;
    static const UIColor PrimaryPinkLight;
    static const UIColor PrimaryGrayLight;

    static const UIColor SelectedBlue;

	static const UIColor Gray95;

private:
    bool valid;
};

inline const UIColor UIColor::Invalid = UIColor();
inline const UIColor UIColor::Transparent = UIColor(0, 0, 0, 0);
inline const UIColor UIColor::Black = UIColor(0, 0, 0, 255);
inline const UIColor UIColor::White = UIColor(255, 255, 255, 255);
inline const UIColor UIColor::Red = UIColor(255, 0, 0, 255);
inline const UIColor UIColor::Green = UIColor(0, 255, 0, 255);
inline const UIColor UIColor::Blue = UIColor(0, 0, 255, 255);
inline const UIColor UIColor::Yellow = UIColor(255, 255, 0, 255);
inline const UIColor UIColor::Gray = UIColor(128, 128, 128, 255);
inline const UIColor UIColor::Orange = UIColor(255, 128, 0, 255);
inline const UIColor UIColor::Purple = UIColor(128, 0, 255, 255);
inline const UIColor UIColor::Pink = UIColor(255, 128, 128, 255);
inline const UIColor UIColor::Gold = UIColor(255, 215, 0, 255);

inline const UIColor UIColor::BackgroundPurple = UIColor(242, 242, 247, 255);
inline const UIColor UIColor::CursorBlue = UIColor(0, 122, 255, 255);
inline const UIColor UIColor::ScrollbarNormal = UIColor(0, 0, 0, 31);
inline const UIColor UIColor::ScrollbarHover = UIColor(0, 0, 0, 66);

inline const UIColor UIColor::PrimaryBlue = UIColor(0, 122, 255, 255);
inline const UIColor UIColor::PrimaryGreen = UIColor(52, 199, 89, 255);
inline const UIColor UIColor::PrimaryPurple = UIColor(160, 116, 196, 255);
inline const UIColor UIColor::PrimaryPink = UIColor(255, 45, 85, 255);
inline const UIColor UIColor::PrimaryGray = UIColor(142, 142, 147, 255);

inline const UIColor UIColor::PrimaryBlueLight = UIColor(64, 156, 255, 255);
inline const UIColor UIColor::PrimaryGreenLight = UIColor(52, 199, 89, 255);
inline const UIColor UIColor::PrimaryPurpleLight = UIColor(233, 213, 255, 255);
inline const UIColor UIColor::PrimaryPinkLight = UIColor(255, 200, 210, 255);
inline const UIColor UIColor::PrimaryGrayLight = UIColor(199, 199, 204, 255);

inline const UIColor UIColor::SelectedBlue = UIColor(187, 233, 255, 255);

inline const UIColor UIColor::Gray95 = UIColor(242, 242, 242, 255);


/*-------------------------------------------------- Utilities --------------------------------------------------*/
#if USE_WIN32
// Helper class for COM exceptions
class com_exception : public std::exception {
public:
	com_exception(HRESULT hr) noexcept : result(hr) {}

	const char* what() const override {
		static char s_str[64] = {};
		sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
		return s_str;
	}

private:
	HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
		throw com_exception(hr);
	}
}

#endif