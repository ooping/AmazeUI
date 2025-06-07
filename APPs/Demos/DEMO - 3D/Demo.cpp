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
using namespace Shape2D;


UIWinTop gWinTop;


void UI2DDemo::Draw()
{
    UIDXFoundation::GetSingletonInstance()->Render3D();
}

void UIWinTop::OnCreate()
{
	const RECT clientRC = GetClientRect();
	string str;

    _2D.CreateWin(&gWinTop, clientRC, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
}

void UIWinTop::OnDestroy() {
}

void UIWinTop::OnNotify(int id, LPARAM param) {
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

        gWinTop.CreateWin(UIFrame::GetSingletonInstance()->GetTopUIContainer(), rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
    }, 0);

    // initialize UI
    UIWin32APP::GetSingletonInstance()->Instance(hInstance, L"AmazeUI 3D", 1800, 1000);
    return 0;
}













