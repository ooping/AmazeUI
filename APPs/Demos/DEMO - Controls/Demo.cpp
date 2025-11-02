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

// ========== UI Widget IDs ==========
namespace UIWidgetIDs {
	// #define WidgetAuto(Type, IDNum, IDName, FuncName) \
	// constexpr int IDNum = IDNum; \
	// inline auto FuncName() { return UIGetWidgetByID<Type>(IDNum); }

	// ===== Main / BottomBar (0xx) =====
	WidgetAuto(UILabel, 10, label_MSG, Label_MSG)
	WidgetAuto(UILabel, 11, label_101, Label_101)
	WidgetAuto(UITab, 90, tab_Main, Tab_Main)

	// ===== Tab1 Controls (1xx) =====
	WidgetAuto(UICheckButton, 101, cb_101, Cb_101)
	WidgetAuto(UICheckButton, 102, cb_102, Cb_102)
	WidgetAuto(UICheckButton, 103, cb_103, Cb_103)
	WidgetAuto(UIEdit, 110, edit_101, Edit_101)
	WidgetAuto(UIImageView, 111, image_101, Image_101)
	WidgetAuto(UIComboBox, 112, combo_101, Combo_101)
	WidgetAuto(UIGrid, 113, grid_101, Grid_101)
	WidgetAuto(UIChart, 114, chart_3d_101, Chart_3D_101)
	WidgetAuto(UIChart, 115, chart_3d_102, Chart_3D_102)

	// ===== Tab2 Controls (2xx) =====
	WidgetAuto(UIChart, 200, chart_201, Chart_201)
}
using namespace UIWidgetIDs;

void UIWinTop::ShowMsg(const string& msg) {
	UIPostEvent([msg = move(msg)]() {
		Label_MSG()->SetText(msg);
	});
}

void UIWinTop::OnCreate()
{
	const RECT clientRC = GetClientRect();
	string str;

	// ===== Create BottomBar (Message Display) =====
	{	
		RECT rc = clientRC;
		rc.top = rc.bottom - 40;
		auto* pCanvas000 = UICreateWidget<UICanvas>(this, 0, rc, UILayoutCalc::MOVE_Y);

		UICreateWidget<UILabel>(pCanvas000, 10)->SetText("Show Message..");

		UILayoutGrid layoutGrid1;
		layoutGrid1.InitPoint(CreatePoint()(20, 0));
		layoutGrid1.SetRowColumn(1, 10, 70, 5);
		layoutGrid1.SetCell(0, 0, 0, 8, Label_MSG());
		layoutGrid1.CalcGridPos();
	}

	// ===== Create Main Tab =====
	auto* pTab = UICreateWidget<UITab>(this, 90, clientRC, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
	
	auto* pCanvas100 = UICreateWidget<UICanvas>(pTab, 0);
	auto* pCanvas200 = UICreateWidget<UICanvas>(pTab, 0);
	pTab->SetCellNum(2);
	pTab->SetCell(0, "tab1", pCanvas100);
	pTab->SetCell(1, "tab2", pCanvas200);

	// ===== Tab1: Module Information =====
	{	
		UICreateWidget<UILabel>(pCanvas100, 11)->SetText("Label 1");

		// Button 1
		auto* pBut1 = UICreateWidget<UIButton>(pCanvas100, 0);
		pBut1->SetText("Button 1");
		pBut1->SetClickEvent([this]() {
			ShowMsg("Button 1");
			Image_101()->SetDLLID(IDB_OK);
			Image_101()->PlayHitDrumAnimate();
		});

		// Button 2
		auto* pBut2 = UICreateWidget<UIButton>(pCanvas100, 0);
		pBut2->SetText("Button 2");
		pBut2->SetClickEvent([this]() {
			ShowMsg("Button 2");
			Image_101()->SetDLLID(IDB_CLOSE);
			Image_101()->PlayHitDrumAnimate();

			// Enhanced flame effect with improved particle system
			_flame1.SetEmissionRate(5.0f);        // 更高发射率
			_flame1.SetFlameIntensity(2.0f);      // 更强火焰强度
			_flame1.SetFlameHeight(1.5f);         // 更高火焰
			_flame1.SetParticleSize(10.0f);        // 更大粒子
			_flame1.SetTurbulence(0.8f);          // 更强湍流
			_flame1.SetConeAngle(60.0f);          // 更大发射角度
			_flame1.SetMaxParticles(1000);         // 更多粒子
			_flame1.SetWindForce({0.2f, 0.0f, 0.0f}); // 添加风力效果
			_flame1.PlayFlameAnimate({0.0f, 0.0f, 0.0f}, UICameraGame::GetSingletonInstance(), 300);
		});

		// CheckButtons
		Cb_101()->CreateControl(101, pCanvas100);
		Cb_101()->SetText("CheckButton 1");
		Cb_101()->SetCheck(true);
		Cb_102()->CreateControl(102, pCanvas100);
		Cb_102()->SetText("CheckButton 2");
		Cb_103()->CreateControl(103, pCanvas100);
		Cb_103()->SetText("CheckButton 3");
		vector<UICheckButton*> checkButtons = { Cb_101(), Cb_102(), Cb_103() };
		UISetCheckButtonMutex(checkButtons);

		// Image
		Image_101()->CreateControl(111, pCanvas100);
		Image_101()->SetDLLPath();

		// Edit
		Edit_101()->CreateControl(110, pCanvas100);
		DateTimeHelper dateTime;
		Edit_101()->SetText(std::format("{:04d}-{:02d}-{:02d}", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetMonthDay()));

		// ComboBox
		Combo_101()->CreateControl(112, pCanvas100);
		Combo_101()->AddText("Select 1");
		Combo_101()->AddText("Select 2");
		Combo_101()->AddText("Select 3");
		Combo_101()->AddText("Select 4");
		Combo_101()->AddText("Select 5");
		Combo_101()->AddText("Select 6");

		// Grid
		Grid_101()->CreateControl(113, pCanvas100);
		Grid_101()->SetRowColumn(33, 3);
		Grid_101()->SetRowFix();
		Grid_101()->SetColumnFix();
		Grid_101()->SetCellFontHeight(22);
		for (int i = 0; i < 33; ++i) { Grid_101()->SetRowHeight(i, 30); }
		Grid_101()->SetColumnWidth(0, 60);
		Grid_101()->SetColumnWidth(1, 110);
		Grid_101()->SetColumnWidth(2, 110);

		for (int i=0; i <= 32; ++i){
			for (int j=0; j < 3; ++j) {
				Grid_101()->SetCellText(i, j, std::format("{},{}", i, j));
			}
		}

		// 3D Chart 1
		auto* pChart3D101 = UICreateWidget<UIChart3D>(pCanvas100, 114);
		vector<UIPointFloat3> points;
		for (int i = 0; i < 100; ++i) {
			points.push_back({0, cos(i * XM_PI / 50.0f), (float)i});
		}
		pChart3D101->AddCurve(L"Curve1", points, UIColor::Gold);
		
		points.clear();
		for (int i = 0; i < 100; ++i) {
			points.push_back({(float)i, sin(i * XM_PI / 50.0f), 0});
		}
		pChart3D101->AddCurve(L"Curve2", points, UIColor::Black);
		pChart3D101->CalcXYCoordRange();

		// 3D Chart 2
		auto* pChart3D102 = UICreateWidget<UIChart3D>(pCanvas100, 115);
		points.clear();
		for (int i = 0; i < 100; ++i) {
			points.push_back({(float)i, sin(i * XM_PI / 50.0f), 0});
		}
		pChart3D102->AddCurve(L"Curve1", points, UIColor::Black);
		pChart3D102->CalcXYCoordRange();

		// Layout
		UILayoutGrid layoutGrid1;
		layoutGrid1.InitPoint(CreatePoint()(20, 20));
		layoutGrid1.SetRowColumn(16, 12, 140, 20, 35, 20);
		
		layoutGrid1.SetCell(0, 0, Label_101());
		layoutGrid1.SetCell(0, 1, pBut1);
		layoutGrid1.SetCell(0, 2, pBut2);
		layoutGrid1.SetCell(0, 3, Image_101());
		
		layoutGrid1.SetCell(1, 1, Cb_101());
		layoutGrid1.SetCell(1, 2, Cb_102());
		layoutGrid1.SetCell(1, 3, Cb_103());

		layoutGrid1.SetCell(2, 1, Edit_101());
		layoutGrid1.SetCell(2, 2, Combo_101());

		layoutGrid1.SetCell(3, 0, 15, 3, pChart3D101);
		layoutGrid1.SetCell(3, 4, 15, 7, pChart3D102);

		layoutGrid1.SetCell(0, 8, 15, 9, Grid_101());
		layoutGrid1.CalcGridPos();
	}

	// ===== Tab2: Calibration Config =====
	{	
		RECT rc = clientRC;
		rc.left += 20;
		rc.right -= 20;
		rc.top += 20;
		rc.bottom -= 50;

		Chart_201()->CreateControl(200, pCanvas200, rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
		vector<float> xList, yList;
		for (int i = 0; i < 100; ++i) {
			xList.push_back((float)i);
			yList.push_back(sin(i * XM_PI / 50.0f));
		}
		Chart_201()->AddCurve1("Chart 1", xList, yList);
		Chart_201()->CalcXYCoordRange();
	}
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

	UIPostEvent([]() {
		RECT rc;
		::GetClientRect(UIFrame::GetSingletonInstance()->GetWindowHandle(), &rc);

		UIWinTop::GetSingletonInstance()->CreateWindowBase(UIFrame::GetSingletonInstance()->GetTopUIContainer(), rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
	});

    // initialize UI
    UIWin32APP::GetSingletonInstance()->Instance(hInstance, L"AmazeUI Controls", 1800, 1000);
    return 0;
}













