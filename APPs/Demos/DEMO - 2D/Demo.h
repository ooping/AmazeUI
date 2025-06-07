#pragma once

#include "pch.h"


class UI2DDemo : public UIWindowBase {
public:
	void Draw();
};



class UIWinTop : public UIWindow<UIWinTop>, public SingleThreadHelper<UIWinTop> {
    
public:
	void OnCreate();
	void OnDestroy();
	void OnNotify(int id, LPARAM param);

private:
	void ShowMsg(const std::string& msg);

	UI2DDemo _2D;
};