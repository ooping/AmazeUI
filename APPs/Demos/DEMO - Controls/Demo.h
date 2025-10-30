#pragma once

#include "pch.h"

class UIWinTop : public UIWindow<UIWinTop>,
                 public SingletonPattern<UIWinTop>,
                 public SingleThreadHelper<UIWinTop> {
	friend class SingletonPattern<UIWinTop>;
    
public:
	void OnCreate();
	void OnDestroy();

private:
	void ShowMsg(const std::string& msg);
	
	// Flame particle effect
	UIAnimateParticleFlame _flame1;
};