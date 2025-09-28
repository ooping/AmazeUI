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


UIWinTop gWinTop;

void UIWinTop::ShowMsg(const string& msg) {
	_label000.SetText(msg);
}

void UIWinTop::OnCreate()
{
	const RECT clientRC = GetClientRect();
	string str;


	{	// connection config
		RECT rc = clientRC;
		rc.top = rc.bottom - 40;
		auto* pCanvas000 = new UICanvas();
		pCanvas000->CreateWindowBaseOnHeap(this, rc, UILayoutCalc::MOVE_Y);

		_label000.CreateControl(0, pCanvas000);
		_label000.SetText("Show Message..");

		// Test the efficiency of multiple calls of std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>>::DrawIndexed()
		//for (int i = 0; i < 1; ++i) {
		//	auto *pLabel = new UILabel();
		//	pLabel->CreateControlOnHeap(0, pCanvas000);
		//	pLabel->SetText("B");
		//}

		// 
		UILayoutGrid layoutGrid1;
		layoutGrid1.InitPoint(CreatePoint()(20, 0));
		layoutGrid1.SetRowColumn(1, 10, 70, 5);
		//
		layoutGrid1.SetCell(0, 0, 0, 8, &_label000);
		//
		layoutGrid1.CalcGridPos();
	}

	auto* pTab1 = new UITab();
	pTab1->CreateControlOnHeap(0, this, clientRC, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
	//
	auto* pCanvas100 = new UICanvas();
	auto* pCanvas200 = new UICanvas();
	pCanvas100->CreateWindowBaseOnHeap(pTab1);
	pCanvas200->CreateWindowBaseOnHeap(pTab1);
	pCanvas100->CreateWindowBaseOnHeap(pTab1);
	pCanvas200->CreateWindowBaseOnHeap(pTab1);
	//
	pTab1->SetCellNum(2);
	pTab1->SetCell(0, "tab1", pCanvas100);
	pTab1->SetCell(1, "tab2", pCanvas200);

	{	// module infor
		_label101.CreateControl(0, pCanvas100);
		_label101.SetText("Lable 1");

		auto* pBut1 = new UIButton();
		pBut1->CreateControlOnHeap(0, pCanvas100);
		pBut1->SetText("Button 1");
		pBut1->SetClickEvent([this]() {
			ShowMsg("Button 1");
			_image101.SetDLLID(IDB_OK);
			_image101.PlayHitDrumAnimate();
		});

		auto* pBut2 = new UIButton();
		pBut2->CreateControlOnHeap(0, pCanvas100);
		pBut2->SetText("Button 2");
		pBut2->SetClickEvent([this]() {
			ShowMsg("Button 2");
			_image101.SetDLLID(IDB_CLOSE);
			_image101.PlayHitDrumAnimate();

			// Enhanced flame effect with improved particle system
			_flame1.SetEmissionRate(5.0f);        // 更高发射率
			_flame1.SetFlameIntensity(2.0f);      // 更强火焰强度
			_flame1.SetFlameHeight(1.5f);         // 更高火焰
			_flame1.SetParticleSize(1.5f);        // 更大粒子
			_flame1.SetTurbulence(0.8f);          // 更强湍流
			_flame1.SetEmissionAngle(60.0f);      // 更大发射角度
			_flame1.SetMaxParticles(200);         // 更多粒子
			_flame1.SetWindForce({0.2f, 0.0f, 0.0f}); // 添加风力效果
			_flame1.PlayFlameAnimate({0.0f, 0.0f, 0.0f}, UICameraGame::GetSingletonInstance(), 300);
		});

		auto* pBut3 = new UIButton();
		pBut3->CreateControlOnHeap(0, pCanvas100);
		pBut3->SetText("Torch Flame");
		pBut3->SetClickEvent([this]() {
			ShowMsg("Torch Flame Effect");
			
			// Torch-like flame effect - narrow and tall
			_flame1.SetEmissionRate(4.0f);
			_flame1.SetFlameIntensity(1.8f);
			_flame1.SetFlameHeight(4.0f);         // 很高的火焰
			_flame1.SetParticleSize(1.2f);
			_flame1.SetTurbulence(0.3f);          // 较低湍流，更稳定
			_flame1.SetEmissionAngle(25.0f);      // 较窄的发射角度
			_flame1.SetMaxParticles(120);
			_flame1.SetWindForce({0.0f, 0.1f, 0.0f}); // 轻微向上的风力
			_flame1.PlayFlameAnimate({0.0f, 0.0f, 0.0f}, UICameraGame::GetSingletonInstance(), 300);
		});


		_checkbut101.CreateControl(199, pCanvas100);
		_checkbut101.SetText("CheckButton 1");
		_checkbut101.SetCheck(true);
		_checkbut102.CreateControl(199, pCanvas100);
		_checkbut102.SetText("CheckButton 2");
		_checkbut103.CreateControl(199, pCanvas100);
		_checkbut103.SetText("CheckButton 3");
		vector<UICheckButton*> checkButtons = { &_checkbut101, &_checkbut102, &_checkbut103 };
		UISetCheckButtonMutex(checkButtons);

		_image101.CreateControl(0, pCanvas100);
		_image101.SetDLLPath();

		_edit101.CreateControl(0, pCanvas100);
		DateTimeHelper dateTime;
		_edit101.SetText(std::format("{:04d}-{:02d}-{:02d}", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetMonthDay()));

		_combo101.CreateControl(0, pCanvas100);
		_combo101.AddText("Select 1");
		_combo101.AddText("Select 2");
		_combo101.AddText("Select 3");
		_combo101.AddText("Select 4");
		_combo101.AddText("Select 5");
		_combo101.AddText("Select 6");

		//
		_grid101.CreateControl(199, pCanvas100);
		_grid101.SetRowColumn(33, 3);
		_grid101.SetRowFix();
		_grid101.SetColumnFix();
		_grid101.SetCellFontHeight(22);
		for (int i = 0; i < 33; ++i) { _grid101.SetRowHeight(i, 30); }
		_grid101.SetColumnWidth(0, 60);
		_grid101.SetColumnWidth(1, 110);
		_grid101.SetColumnWidth(2, 110);

		for (int i=0; i <= 32; ++i){
			for (int j=0; j < 3; ++j) {
				_grid101.SetCellText(i, j, std::format("{},{}", i, j));
			}
		}

		auto* pChart3D101 = new UIChart3D();
		pChart3D101->CreateControlOnHeap(0, pCanvas100);
		//
		vector<UIPointFloat3> points;
		for (int i = 0; i < 100; ++i) {
			points.push_back({0, cos(i * XM_PI / 50.0f), (float)i});
		}
		pChart3D101->AddCurve(L"Curve1", points, UIColor::Gold);
		//
		points.clear();
		for (int i = 0; i < 100; ++i) {
			points.push_back({(float)i, sin(i * XM_PI / 50.0f), 0});
		}
		pChart3D101->AddCurve(L"Curve2", points, UIColor::Black);
		//
		pChart3D101->CalcXYCoordRange();

		auto* pChart3D102 = new UIChart3D();
		pChart3D102->CreateControlOnHeap(0, pCanvas100);
		//
		points.clear();
		for (int i = 0; i < 100; ++i) {
			points.push_back({(float)i, sin(i * XM_PI / 50.0f), 0});
		}
		pChart3D102->AddCurve(L"Curve1", points, UIColor::Black);
		//
		pChart3D102->CalcXYCoordRange();

		UILayoutGrid layoutGrid1;
		layoutGrid1.InitPoint(CreatePoint()(20, 20));
		layoutGrid1.SetRowColumn(16, 12, 140, 20, 35, 20);
		//
		layoutGrid1.SetCell(0, 0, &_label101);
		layoutGrid1.SetCell(0, 1, pBut1);
		layoutGrid1.SetCell(0, 2, pBut2);
		layoutGrid1.SetCell(0, 3, &_image101);
		
		layoutGrid1.SetCell(1, 1, &_checkbut101);
		layoutGrid1.SetCell(1, 2, &_checkbut102);
		layoutGrid1.SetCell(1, 3, &_checkbut103);

		layoutGrid1.SetCell(2, 1, &_edit101);
		layoutGrid1.SetCell(2, 2, &_combo101);

		layoutGrid1.SetCell(3, 0, 15, 3, pChart3D101);
		layoutGrid1.SetCell(3, 4, 15, 7, pChart3D102);

		//
		layoutGrid1.SetCell(0, 8, 15, 9, &_grid101);
		//
		layoutGrid1.CalcGridPos();
	}

	{	// calibration config
		RECT rc = clientRC;
		rc.left += 20;
		rc.right -= 20;
		rc.top += 20;
		rc.bottom -= 50;

		_chart201.CreateControl(0, pCanvas200, rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
		vector<float> xList, yList;
		for (int i = 0; i < 100; ++i) {
			xList.push_back((float)i);
			yList.push_back(sin(i * XM_PI / 50.0f));
		}
		_chart201.AddCurve1("Chart 1", xList, yList);
		_chart201.CalcXYCoordRange();
	}
}

void UIWinTop::OnDestroy() {
}

void UIWinTop::OnNotify(int id, LPARAM param) {
	string str;
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

        gWinTop.CreateWindowBase(UIFrame::GetSingletonInstance()->GetTopUIContainer(), rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
    }, 0);

    // initialize UI
    UIWin32APP::GetSingletonInstance()->Instance(hInstance, L"AmazeUI Controls", 1800, 1000);
    return 0;
}













