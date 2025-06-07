
#pragma once

#include "UIUtility.h"

/*-------------------------------------------------------UIWindowBase-------------------------------------------------------*/
/*
base window class
basic information:
1 position information
2 hierarchy information
3 layout information
4 other

basic functions:
1 basic drawing
2 window creation, destruction
3 default message processing
note: operations are divided into protected and unprotected, protected means sending operation requests to the message loop, 
unprotected means direct operation, for unprotected direct setting function names will be added with Direct, such as SetXXDirect();
*/
class UIContainer;
template<class T> class UIContainerHelp;
class UITab;
class UIWindowBase
{
	friend UIContainer;
	template<class T> friend class UIContainerHelp;
	friend UITab;

public:
	UIWindowBase();
	virtual ~UIWindowBase();

	// create window
	bool CreateWin(UIContainer* pUIContainer, const RECT& relativeRect=Shape2D::NULL_RECT, int layoutFlag=UILayoutCalc::NO_ZOOM, bool isShow=true, bool isOnHeap=false);
	bool CreateWin(UIWindowBase* pParent, const RECT& relativeRect=Shape2D::NULL_RECT, int layoutFlag=UILayoutCalc::NO_ZOOM, bool isShow=true, bool isOnHeap=false);
	bool CreateWinPopup(UIWindowBase* pParent, const RECT& relativeRect=Shape2D::NULL_RECT, int layoutFlag=UILayoutCalc::NO_ZOOM, bool isShow=true, bool isOnHeap=false);
	// destroy window
	bool DestroyWin();

	// message processing
	virtual bool HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

	// set direct, unprotected setting
	void MoveWindow(const RECT& relativeRect);
	void ShowWindow(bool flag);

	// when call this function, should make sure the parent window is focus, otherwise there are some logic problems
	void SetWindowFocus();

	// read, unprotected
	RECT GetClientRect();
	RECT GetRelativeRect();
	RECT GetAbusoluteRect();
	bool IsWindowFocus();
	bool IsWindowShow();

	//
	virtual void Draw();						// drawing

	UIContainer* GetUIContainer();

	void SetTransformMatrix(const DirectX::XMMATRIX& transformMatrix);
	DirectX::XMMATRIX GetInheritedTransformMatrix();

protected:
	// default message processing
	bool DefHandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

	DirectX::XMMATRIX GetTransformMatrix();

	// position information
	RECT _clientRC;								// starting from 0 size rectangle
	POINT _relativePoint;						// starting position relative to parent window
	POINT _abusolutePoint;						// starting position of top-level window

	// hierarchy information
	int _layoutLever;							// hierarchy, determines depth value 1-1.0 2-0.95 3-0.9
	float _z;									// depth value

	// other
	bool _isShow;								// display window
	//bool _isRecvTabKey;						// receive tab key
	bool _isFocus;								// window focus activation information
	int _layoutMode;							// layout mode
	bool _isTransmissionMsg;					// transmit sub-window messages
		 
	UIContainer* p_UIContainer;					// pointer to self container
	UIContainer* p_parentUIContainer;			// pointer to parent container

	DirectX::XMMATRIX _transformMatrix;
	// 3D map to screen position, maybe is not rect
};





/*-------------------------------------------------------UIContainer-------------------------------------------------------*/
/*
name: window container
description: container class, provides management of sub-windows, dispatching and processing messages.
usage: recommend using the inclusion method, adding the container's message processing in the parent window's message processing as needed.
function:
1 sub-window management, participates in the creation, destruction, layout calculation, drawing management, etc. of windows;
2 message passing and processing, the parent window receives a message first and passes it to the sub-window through the bound window container for pre-processing
(the sub-window bound to the window container continues to pass it down, forming a recursive process), if the sub-window cannot handle it, then the parent window itself will handle it;

note: all settings of the container class are unprotected, only safe when called in the message loop, but because the container class is only used in the AmazeUI framework,
it is a framework internal concept, so the function name is not added with Direct;
*/
class UIContainer
{
	// container element
	struct UIElement
	{
		UIWindowBase* p_win;					// pointer to sub-window
		UILayoutCalc _layoutInfo;				// layout mode
		bool _isOnHeap;							// whether the window memory is on the heap

		UIElement(UIWindowBase* p, bool isOnHeap) { p_win=p; _isOnHeap=isOnHeap; }
		bool operator==(UIWindowBase* p) { return p_win==p; }
	};
	typedef std::vector<UIElement> UIElementListType;

public:
	UIContainer();
	~UIContainer();

	// bind with parent window
	void BindWindow(UIWindowBase *pBindWindow);
	void BindWindow();

	// add sub-window   delete sub-window   data protection not performed in operation
	void AddChild(UIWindowBase* pWin, bool isOnHeap);
	void DelChild(UIWindowBase* pWin);

	// message pre-processing includes all messages except WM_PAINT
	bool HandleMessagePre(UINT message, WPARAM wParam, LPARAM lParam);

	// read
	POINT GetBindWinAbusolutePoint();
	RECT GetBindWinAbusoluteRect();
	int GetBindWinLayoutLever();

	// set
	void SetFocusOnChild(UIWindowBase* pNew);
	void SetUnFocusAll();
	void SendMessageToBindWin(UINT message, WPARAM wParam, LPARAM lParam);

	// draw
	void Draw();

	DirectX::XMMATRIX GetBindWindowInheritedTransformMatrix();

private:
	UIWindowBase* GetMinZChild(const POINT& pt);
	UIWindowBase* GetFocusOnChild();			// find the sub-window with focus on

	bool _isBindDUI;							// mark the parent window as DUI or hwnd
	UIWindowBase* _pBindDUIWin;					// the parent window bound to DUI

	POINT _preMousePt;							// the previous mouse position

	UIElementListType _winList;					// sub-window queue
};

// provide support for self Container
template<class T>
class UIContainerHelp
{
public:
	UIContainerHelp()
	{
		T* pT = static_cast<T*>(this);
		pT->p_UIContainer = &_uiContainer;
		_uiContainer.BindWindow(pT); 
	}

	// vector container's resize will call the copy constructor
	UIContainerHelp(UIContainerHelp& obj)
	{
		T* pT = static_cast<T*>(this);
		pT->p_UIContainer = &_uiContainer;
		_uiContainer.BindWindow(pT);
	}

protected:
	UIContainer _uiContainer;
};





/*************************************************** UIMessage Loop ***************************************************/
class UIWindowBase;
struct UIMessage {
	// Default constructor
	UIMessage() : p_win(nullptr), _msg(0), _wParam(0), _lParam(0) {}
	
	// Constructor with parameters
	UIMessage(UIWindowBase* pWin, UINT message, WPARAM wParam = 0, LPARAM lParam = 0) 
		: p_win(pWin), _msg(message), _wParam(wParam), _lParam(lParam) {
	}

	UIWindowBase* p_win;     	// Target window pointer
	UINT _msg;            		// Message type
	WPARAM _wParam;       		// Message parameters
	LPARAM _lParam;       		// Message parameters
};

// DirectUI Message Loop Processing Mechanism
class UIMessageLoop : public SingletonPattern<UIMessageLoop>, public SingleThreadHelper<UIMessageLoop> {
	friend class SingletonPattern<UIMessageLoop>;
	friend void UIPostMessage(UIWindowBase* winPoint, UINT message, WPARAM wParam, LPARAM lParam);

public:
	void RunMessageLoop();

	std::deque<UIMessage> _uiMsgList;		// UI message queue

private:
	UIMessageLoop() = default;
	~UIMessageLoop() = default;

	void RunMessageLoopThread();

	void PushMessage(UIWindowBase* winPoint, UINT message, WPARAM wParam, LPARAM lParam);
	bool PopMessage(UIMessage& msg); 

	bool HandleMessage(UIWindowBase* winPoint, UINT message, WPARAM wParam, LPARAM lParam);

	//std::mutex _uiMsgListMutex;
	CriticalSection _uiMsgListCS;
};
void UIPostMessage(UIWindowBase* winPoint, UINT message, WPARAM wParam, LPARAM lParam);
void UIRefresh(bool immediately=true);
bool UIMessageFilter1(UINT message);		// Enter UI message loop filter
bool UIMessageFilter2(UINT message);		// Reverse message transmission filter


// UIFrame manage the top level UI container and hook window
class UIFrame : public SingletonPattern<UIFrame> {
	friend class SingletonPattern<UIFrame>;

public:
	void Initialize(HWND windowHWnd, int width, int height);
	HWND GetWindowHandle() const;
	UIContainer* GetTopUIContainer();

	void SetHookWindowForPopup(UIWindowBase* pHookWindow);
	void SetHookWindowForScrollBar(UIWindowBase* pHookWindow);

	bool HandleMessageFromHookWindow(UINT message, WPARAM wParam, LPARAM lParam);
	
private:
	UIFrame();
	~UIFrame() = default;

	UIContainer _topUIContainer;

	UIWindowBase* _pHookWindowForPopup;
	UIWindowBase* _pHookWindowForScrollBar;

	HWND _windowHWnd;
};
