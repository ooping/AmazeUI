
#pragma once

#include "UIUtility.h"
//#include "UIWindow.h"

/*
	windows application class: provide a shell
*/
class UIWin32APP : public SingletonPattern<UIWin32APP> {
	friend class SingletonPattern<UIWin32APP>;				// Allow Singleton to access private constructor
	
public:
	bool Instance(HINSTANCE hInstance, std::wstring titleStr, int width=960, int height=640);

	HWND _hWnd;												// window handle

private:
	UIWin32APP();
	~UIWin32APP() = default;

	int GetClientWidth();
	int GetClientHeight();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	std::wstring _windowClass;								//	main window class name
};
	