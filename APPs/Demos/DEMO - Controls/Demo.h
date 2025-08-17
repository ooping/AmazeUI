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
	UILabel _label000;

    UICanvas _canvas100, _canvas101, _canvas102, _canvas103;
	UICheckButton _checkbut101, _checkbut102, _checkbut103;
	UIButton _but101, _but102;
	UIEdit _edit101;
	UILabel _label101;
	UIImageView _image101;
	UIComboBox _combo101;
	UIGrid _grid101;


	UICanvas _canvas200;
	UIChart _chart201;
};