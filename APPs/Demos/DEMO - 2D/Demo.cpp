//--------------------------------------------------------------------------------------
// Main.cpp
//
// Entry point for Windows desktop application.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Demo.h"
#include <Dbt.h>

using namespace DirectX;
using namespace std;
using namespace UIShape2D;


void UI2DDemo::Draw()
{
     UIPoint(5, 5, 0.3)(UIColor::Red);

     vector<POINT> points = {
        POINT{10, 10},
        POINT{20, 20},
        POINT{30, 30},
        POINT{40, 40},
        POINT{50, 50},
     };
     UIPoints(points, 0.5)(UIColor::Blue);

    UILine(10, 10, 200, 10, 0.2)(UIColor::Blue);
    UILine(10, 10, 200, 200, 0.5)(UIColor::Red);
    UILine(10, 10, 10, 200, 0.2)(UIColor::Blue);

    UIRect(RECT{ 700, 10, 800, 200 }, 0.3)(Colors::Aquamarine, 255);
    UILine(700, 10, 700, 200, 0.3)(UIColor::Blue);
    UILine(800, 10, 800, 200, 0.3)(UIColor::Blue);
    UILine(700, 10, 800, 10, 0.3)(UIColor::Blue);
    UILine(700, 200, 800, 200, 0.3)(UIColor::Blue);

    // load image
	// get current working directory
    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"GUIResource.dll");
    UIImage(strFilePath, 80003, UIColor(255, 0, 255, 0), 0.3)(NULL_RECT, RECT{100, 75, 0, 0});

    UIImage(strFilePath, IDB_BUTTON1_NORMAL, UIColor(255, 0, 255, 0), 0.5)(RECT{850, 130, 1050, 180});
    UISlicedImage(strFilePath, IDB_BUTTON1_NORMAL, UIColor(255, 0, 255, 0), 3, 3, 3, 3, 0.5)(RECT{850, 50, 1050, 100});
    
    {
        UIScreenClipRectGuard clipRect(RECT{ 500, 10, 600, 200 });
        UIRect(RECT{ 500, 10, 600, 200 }, 0.3)(UIColor::Pink, 100);
    }

    DX::FindMediaFile(strFilePath, MAX_PATH, L"cat.png");
    UIImage(strFilePath, UIColor::Invalid, 0.5)(NULL_RECT, RECT{ 100, 100, 0, 0 }, 255);
 
    UIFont(0.3, 24)(L"DirectXTK Simple UIDXFoundation", POINT{100, 10}, UIColor::Black);
    UIFont(0.1, 18)(L"VAI12345678901234567890", RECT{700, 10, 800, 200}, UIColor::Red);

    {
        UIScreenClipRectGuard clipRect(RECT{ 350, 10, 450, 200 });
        UIRect(RECT{ 350, 10, 450, 200 }, 0.3)(UIColor::Pink, 100);
    }

    {
        XMMATRIX localTransform = UIZPlaneTransform::GetTransformMatrix(false, 0, 0, 0, false, 0, 0,
                                                                        true, 1200.f, -0.05f * XM_PI, 0.f);
        UIDXFoundation* pDXF = UIDXFoundation::GetSingletonInstance();
        
        //pDXF->Draw3DImage(strFilePath, UIColor::Invalid, {0, 0, 400, 100}, XMFLOAT2(1000.f, 300.f), XMFLOAT2(1400.f, 400.f), 0.1f, 255, localTransform);

        UIRect(RECT{ 1000, 300, 1400, 400 }, 0.1)(UIColor::Yellow, 100);
        pDXF->Draw3DRectSolid({ 1000, 300 }, { 1400, 400 }, 0.1f, UIColor::Pink, 255);

        //pDXF->Draw3DTextFT(L"Hello AmazeUI", { 1000, 300, 1400, 400 }, 0x01 | 0x04, 0.1f, UIColor::Black, 24);
    }

}

void UIWinTop::OnCreate()
{
	const RECT clientRC = GetClientRect();
	string str;

    _2D.CreateWindowBase(this, clientRC, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
}

void UIWinTop::OnDestroy() {
}













// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    // 
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // check CPU support
    if (!XMVerifyCPUSupport()) {
        return 1;
    }

    // initialize COM
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize)) {
        return 1;
    }

    UIPostMessage(NULL, WM_MQ, (WPARAM)(pMQFuncType)[](LPARAM) {
        RECT rc;
        ::GetClientRect(UIFrame::GetSingletonInstance()->GetWindowHandle(), &rc);

        UIWinTop::GetSingletonInstance()->CreateWindowBase(UIFrame::GetSingletonInstance()->GetTopUIContainer(), rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
    }, 0);

    // initialize UI
    UIWin32APP::GetSingletonInstance()->Instance(hInstance, L"AmazeUI 2D", 1800, 1000);
    return 0;
}













