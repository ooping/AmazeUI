#pragma once

#include "pch.h"

void ShowMsg(const std::string& msg);

class UIWinTop : public UIWindow<UIWinTop>, public SingleThreadHelper<UIWinTop> {
    
public:
	void OnCreate();
	void OnDestroy();
	void OnNotify(int id, LPARAM param);

private:
	void ShowMsg(const std::string& msg);

    UITab _tab1;

	UICanvas _canvas000;
	UILable _lable001;
	UILayoutGrid _layoutGrid000;

    UICanvas _canvas100, _canvas101, _canvas102, _canvas103;
	UICheckButton _checkbut101, _checkbut102, _checkbut103;
	UIButton _but101, _but102;
	UIEdit _edit101;
	UILable _lable101;
	UIImageView _image101;
	UIComboBox _combo101;
	UIGrid _grid101;

	UIImage3D _image3D101;
	UILayoutGrid _layoutGrid100;

	UICanvas _canvas200;
	UIChart _chart201;
};