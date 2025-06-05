
#include "..\CORE\Common.h"

#pragma once

// win32
#ifdef _WIN32
//
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <WS2tcpip.h>
//
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#endif

/*-------------------------------------------------- Header files / DirectX12 --------------------------------------------------*/
#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <DirectXMath.h>
#include <DirectXColors.h>


#include "..\\Kits\\ATGTK\\d3dx12.h"
#include "..\\Kits\\ATGTK\\FindMedia.h"



/*-------------------------------------------------- Header files / DirectX Tool Kit --------------------------------------------------*/
// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h> 
// then add the NuGet package WinPixEventRuntime to the project. 
#include <pix.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "Audio.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "DescriptorHeap.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PrimitiveBatch.h"
#include "ResourceUploadBatch.h"
#include "RenderTargetState.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h" 
#include "VertexTypes.h"


// FreeType 相关头文件
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H



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



#define WM_SIZERESET					(WM_USER+1003)
#define WM_REGANIMATE					(WM_USER+1005)
#define WM_MQ							(WM_USER+1006)

typedef void (*pMQFuncType)(LPARAM lParam);


/*-------------------------------------------------- UI utility functions--------------------------------------------------*/
namespace Shape2D {
	constexpr RECT NULL_RECT = {0, 0, 0, 0};
	constexpr POINT NULL_POINT = {0, 0};
	constexpr SIZE NULL_SIZE = {0, 0};

	struct CreateRect {
		RECT operator()(POINT point, SIZE size);
        RECT operator()(LONG left, LONG top, LONG right, LONG bottom);
	};

	struct CreatePoint {
		POINT operator()(LONG x, LONG y);
	};

	struct CreateSize {
		SIZE operator()(LONG x, LONG y);
	};

	struct GetRectWidth {
		LONG operator()(const RECT& rc);
	};

	struct GetRectHeight {
		LONG operator()(const RECT& rc);
	};

	struct GetRectCenter {
		POINT operator()(const RECT& rc);
	};

	struct CompareRects {
		bool operator()(const RECT& r1, const RECT& r2);
	};

	struct IntersectRects {
		RECT operator()(const RECT& rc1, const RECT& rc2);
	};

	struct ComparePoints {
		bool operator()(const POINT& p1, const POINT& p2);
	};

	struct ContainsPoint {
		bool operator()(const POINT& point, const RECT& rect);
	};

	struct ScaleRect {
		RECT operator()(const RECT& rc, float scale);
	};
};

struct IsKeyDown {
	bool operator()(int key);
};

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

	UILayoutCalc(int flag=SIZE_X|SIZE_Y);

	void SetLayoutMode(int flag);										// Setting the zoom mode
	void InitLayout(const RECT& parentRect, const RECT& rect);			// Initial calculation: Normal mode/directUI mode
	RECT CalcLayout(LONG cx, LONG cy);									// Calc new state

	int _zoomModeflag;		// Layout Mode

	RECT _parentRect;		// The area of the parent form
	RECT _rect;				// Control relative to parent form area
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

	DirectX::XMVECTORF32 ToXMVECTORF32(uint8_t alpha) const { return DirectX::XMVECTORF32{_r/255.0f, _g/255.0f, _b/255.0f, alpha/255.0f}; }
    DirectX::XMVECTORF32 ToXMVECTORF32() const { return DirectX::XMVECTORF32{_r/255.0f, _g/255.0f, _b/255.0f, _a/255.0f}; }
    uint32_t ToRGBA() const { return static_cast<uint32_t>(_r) << 24 | static_cast<uint32_t>(_g) << 16 | 
									 static_cast<uint32_t>(_b) << 8 |  static_cast<uint32_t>(_a); }

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





#if _WIN32
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