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

void UIWinTop::ShowMsg(const string& msg) {
	_lable001.SetText(msg);
}

void UIWinTop::OnCreate()
{
	const RECT clientRC = GetClientRect();
	string str;


	{	// connection config
		RECT rc = clientRC;
		rc.top = rc.bottom - 40;
		_canvas000.CreateWin(this, rc, UILayoutCalc::MOVE_Y);

		_lable001.CreateControl(0, &_canvas000);
		_lable001.SetText("Show Message..");

		// 
		_layoutGrid000.InitPoint(CreatePoint()(20, 0));
		_layoutGrid000.SetRowColumn(1, 10, 70, 5);
		//
		_layoutGrid000.SetCell(0, 0, 0, 8, &_lable001);;
		//
		_layoutGrid000.CalcGridPos();
	}

	_tab1.CreateControl(0, this, clientRC, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);

	_canvas100.CreateWin(&_tab1);
	_canvas200.CreateWin(&_tab1);
	_tab1.SetCellNum(2);
	_tab1.SetCell(0, "tab1", &_canvas100);
	_tab1.SetCell(1, "tab2", &_canvas200);

	{	// module infor
		_lable101.CreateControl(0, &_canvas100);
		_lable101.SetText("Lable 1");

		_but101.CreateControl(101, &_canvas100);
		_but101.SetText("Button 1");
		_but102.CreateControl(102, &_canvas100);
		_but102.SetText("Button 2");

		_checkbut101.CreateControl(199, &_canvas100);
		_checkbut101.SetText("CheckButton 1");
		_checkbut101.SetCheck(true);
		_checkbut102.CreateControl(199, &_canvas100);
		_checkbut102.SetText("CheckButton 2");
		_checkbut103.CreateControl(199, &_canvas100);
		_checkbut103.SetText("CheckButton 3");
		vector<UICheckButton*> checkButtons = { &_checkbut101, &_checkbut102, &_checkbut103 };
		UISetCheckButtonMutex(checkButtons);

		_image101.CreateControl(0, &_canvas100);
		_image101.SetDLLPath();

		_edit101.CreateControl(0, &_canvas100);
		DateTimeHelper dateTime;
		_edit101.SetText(std::format("{:04d}-{:02d}-{:02d}", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetMonthDay()));

		_combo101.CreateControl(0, &_canvas100);
		_combo101.AddText("Select 1");
		_combo101.AddText("Select 2");
		_combo101.AddText("Select 3");
		_combo101.AddText("Select 4");
		_combo101.AddText("Select 5");
		_combo101.AddText("Select 6");

		//
		_grid101.CreateControl(199, &_canvas100);
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

		_image3D101.CreateControl(0, &_canvas100);

		// ²¼¾Ö¼ÆËã
		_layoutGrid100.InitPoint(CreatePoint()(20, 20));
		_layoutGrid100.SetRowColumn(16, 12, 140, 20, 35, 20);
		//
		_layoutGrid100.SetCell(0, 0, &_lable101);
		_layoutGrid100.SetCell(0, 1, &_but101);
		_layoutGrid100.SetCell(0, 2, &_but102);
		_layoutGrid100.SetCell(0, 3, &_image101);
		
		_layoutGrid100.SetCell(1, 1, &_checkbut101);
		_layoutGrid100.SetCell(1, 2, &_checkbut102);
		_layoutGrid100.SetCell(1, 3, &_checkbut103);

		_layoutGrid100.SetCell(2, 1, &_edit101);
		_layoutGrid100.SetCell(2, 2, &_combo101);

		//
		_layoutGrid100.SetCell(0, 6, 15, 7, &_grid101);
		//
		_layoutGrid100.SetCell(8, 1, 11, 3, &_image3D101);
		//
		_layoutGrid100.CalcGridPos();
	}

	{	// calibration config
		RECT rc = clientRC;
		rc.left += 20;
		rc.right -= 20;
		rc.top += 20;
		rc.bottom -= 50;
		_chart201.CreateControl(0, &_canvas200, rc, UILayoutCalc::SIZE_X | UILayoutCalc::SIZE_Y);
		vector<double> xList, yList;
		for (int i = 0; i < 100; ++i) {
			xList.push_back(i);
			yList.push_back(sin(i * XM_PI / 50.0f));
		}
		_chart201.AddCurve1("Chart 1", xList, yList);
		_chart201.CalcXYCoordRange();
	}

	{	
	}
}

void UIWinTop::OnDestroy() {
}

void UIWinTop::OnNotify(int id, LPARAM param) {
	string str;

	switch(id) {
		case 101: {
			ShowMsg("Button 1");
			_image101.SetDLLID(IDB_OK);
			_image101.PlayHitDrumAnimate();

		} break;
		case 102: {
			ShowMsg("Button 2");
			_image101.SetDLLID(IDB_CLOSE);
			_image101.PlayHitDrumAnimate();
		} break;
	}
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
    UIWin32APP::GetSingletonInstance()->Instance(hInstance, L"AmazeUI Controls", 1800, 1000);
    return 0;
}













