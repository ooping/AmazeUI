#include "UIApplication.h"
#include "UIWindow.h"

using namespace std;

UIWin32APP::UIWin32APP() {
	_hWnd = NULL;
	_windowClass = L"AmazeUI";
}

bool UIWin32APP::Instance(HINSTANCE hInstance, std::wstring titleStr, int width, int height) {
	// Register class
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = UIWin32APP::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = _windowClass.c_str();
	wcex.hIconSm = LoadIconW(hInstance, L"IDI_ICON");
	if (!RegisterClassExW(&wcex)) {
		return false;
	}

	// Create window
	_hWnd = CreateWindowExW(0, _windowClass.c_str(), titleStr.c_str(), WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);
	
	if (!_hWnd) {
		return false;
	}

	// move window to center
	{
		HWND parentHwnd;
		RECT rc, rc2;
		int  x,y;
		//
		((parentHwnd = GetParent(_hWnd)) == NULL) ? SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0) : GetClientRect(parentHwnd, &rc);

		GetWindowRect(_hWnd, &rc2);
		x = ((rc.right-rc.left)-(rc2.right-rc2.left))/2 +rc.left;
		y = ((rc.bottom-rc.top)-(rc2.bottom-rc2.top))/2 +rc.top;
		//
		SetWindowPos(_hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
	}

	// show window
	ShowWindow(_hWnd, SW_SHOW);
	UpdateWindow(_hWnd);

	UIFrame::GetSingletonInstance()->Initialize(_hWnd, GetClientWidth(), GetClientHeight());
	UIMessageLoop::GetSingletonInstance()->RunMessageLoop();

	// Main message loop
    MSG msg = {};
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // 这里可以处理其他任务
            // 比如更新游戏逻辑、渲染等
            Sleep(1); // 可选：避免CPU占用过高
        }
    }
	// while (GetMessage(&msg, NULL, 0, 0))
	// {
	// 	TranslateMessage(&msg);
	// 	DispatchMessage(&msg);
	// }

	return true;
}



LRESULT CALLBACK UIWin32APP::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// send message to directUI message loop
	UIPostMessage(NULL, message, wParam, lParam);

	// handle win32 message
	static bool s_in_sizemove = false;

	switch (message) {
		case WM_SIZE: {
			if (wParam!=SIZE_MINIMIZED && !s_in_sizemove) {
				UIPostMessage(NULL, WM_SIZERESET, wParam, lParam);
			}
		} break;
		// case WM_ENTERSIZEMOVE:
		// 	s_in_sizemove = true;
		// 	break;
		case WM_EXITSIZEMOVE: {
     		s_in_sizemove = false;
			UIPostMessage(NULL, WM_SIZERESET, wParam, lParam);
		} break;
		case WM_DESTROY: {
			PostQuitMessage(0);
		} break;
		default: {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}

int UIWin32APP::GetClientWidth() { 
	RECT rc;
	if (::GetClientRect(_hWnd, &rc) == TRUE) {
		return rc.right-rc.left; 
	}
	else {
		return -1;
	}
}

int UIWin32APP::GetClientHeight() { 
	RECT rc;
	if (::GetClientRect(_hWnd, &rc) == TRUE) {
		return rc.bottom-rc.top; 
	}
	else {
		return -1;
	}
}