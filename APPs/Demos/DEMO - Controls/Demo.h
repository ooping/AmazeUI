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

	UILabel _label000;

	UICheckButton _checkbut101, _checkbut102, _checkbut103;
	UIEdit _edit101;
	UILabel _label101;
	UIImageView _image101;
	UIComboBox _combo101;
	UIGrid _grid101;

	UIChart _chart201;
	
	// Flame particle effect
	UIAnimateParticleFlame _flame1;
};