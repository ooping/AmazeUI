#pragma once

#include "pch.h"

struct MsgData {
	std::string _str;
	int _param;
};

struct CurveData {
	std::string _name;
	VECTOR_DOUBLE _xList;
	VECTOR_DOUBLE _yList;
	bool _clear;
	bool _lShow;

	UIColor _color;
};

struct GridData {
	VECTOR_DOUBLE _list1;
	VECTOR_DOUBLE _list2;

	int _flag;
};

void ShowMsg(const std::string& msg);
void ShowTXMsg(int ms, const std::string& msg);
void ShowRXMsg(int ms, const std::string& msg);
void ShowCurve(std::string name, VECTOR_DOUBLE& xList, VECTOR_DOUBLE& yList, UIColor color= UIColor::Black, bool clear=false, bool lShow=true);
void ShowGird(VECTOR_DOUBLE& list1, VECTOR_DOUBLE& list2, int flag=0);

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