#pragma once

#include "pch.h"


class UI2DDemo : public UIWindowBase {
public:
	void Draw();
};



class UIWinTop : public UIWindow<UIWinTop>, 
                 public SingletonPattern<UIWinTop>,
                 public SingleThreadHelper<UIWinTop> {
	friend class SingletonPattern<UIWinTop>;
    
public:
	void OnCreate();
	void OnDestroy();

private:
	void ShowMsg(const std::string& msg);

	UI2DDemo _2D;
};