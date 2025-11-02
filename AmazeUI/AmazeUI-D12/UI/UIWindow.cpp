#include "UIWindow.h"
#include "UIDXFoundation.h"
#include "UIAnimation.h"
#include "UIApplication.h"
#include "UIElement.h"

using namespace std;
using namespace DirectX;
using namespace UIShape2D;

const float gLevelZ = 0.01f;
const float gPopupZ = 0.5f;


UIWindowBase::UIWindowBase() {
	_isShow = false;
	//_isRecvTabKey = false;
	_isFocus = false;
	_isTransmissionMsg = false;
	_isPopup = false;

	_layoutLevel = 1;
	_z = 0;

	_layoutOrder = 5;

	p_parentUIContainer = nullptr;
	p_UIContainer = nullptr;

	_transformMatrix = DirectX::XMMatrixIdentity();
}

UIWindowBase::~UIWindowBase() {
}

bool UIWindowBase::CreateWindowBase(UIContainer* pUIContainer, const RECT& relativeRect, int layoutFlag, bool isShow) {
	// check if the parent container exists
	if (pUIContainer == nullptr) {
		return false;
	}

	// get parent container
	p_parentUIContainer = pUIContainer;

	// whether to display and draw
	_isShow = isShow;

	// size and position information
	// absolute size
	_clientRC = relativeRect;
	OffsetRect(&_clientRC, -_clientRC.left, -_clientRC.top);
	// relative to parent window starting point
	_relativePoint = UIShape2D::CreatePoint()(relativeRect.left, relativeRect.top);
	// relative to handle window starting point
	POINT p = p_parentUIContainer->GetBindWindowAbusolutePoint();
	_abusolutePoint = UIShape2D::CreatePoint()(p.x+relativeRect.left, p.y+relativeRect.top);

	// layout information
	_layoutMode = layoutFlag;

	// hierarchy information and z value
	_isPopup = p_parentUIContainer->IsBindWindowPopup();
	_layoutLevel = p_parentUIContainer->GetBindWindowLayoutLevel() + 1;
	_z = (!_isPopup ? 1.0f : gPopupZ) - (_layoutLevel - 1) * gLevelZ;

	return HandleMessage(WM_CREATE, 0, 0);
}

bool UIWindowBase::CreateWindowBase(UIWindowBase* pParent, const RECT& relativeRect, int layoutFlag, bool isShow) {
	return CreateWindowBase(pParent->GetUIContainer(), relativeRect, layoutFlag, isShow);
}

bool UIWindowBase::DestroyWindowBase() {
	return HandleMessage(WM_DESTROY, 0, 0);
}

bool UIWindowBase::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	return DefHandleMessage(message, wParam, lParam);
}

bool UIWindowBase::DefHandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	// transmit message
	if (_isTransmissionMsg && UIMessageFilter2(message)) {
		p_parentUIContainer->SendMessageToBindWindow(message, wParam, lParam);
		return true;
	}

	if (message == WM_CREATE) {	
		// add the newly created window to the parent container
		p_parentUIContainer->AddChild(this);
	} else if (message == WM_DESTROY) {
		// delete the window from the parent container
		p_parentUIContainer->DelChild(this);
	}

	// message pre-processing
	bool rt = false;
	if (p_UIContainer != nullptr) {	
		// sub-window processing the corresponding message
		rt = p_UIContainer->HandleMessagePre(message, wParam, lParam);
	}

	return rt;
}

void UIWindowBase::Draw() {
	if (!_isShow) {
		return;
	}

	// self drawing
	//UIRect(_abusolutePoint.x, _abusolutePoint.y, GetRectWidth()(_clientRC), GetRectHeight()(_clientRC), _z)(_RED_, 100);

	if (p_UIContainer != nullptr) {
		//RECT rc = _clientRC;
		//OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
		//UIScreenClipRectGuard uiClip(rc);

		p_UIContainer->Draw();
	}
}

RECT UIWindowBase::GetClientRect() {
	return _clientRC;
}

RECT UIWindowBase::GetRelativeRect() {
	RECT relativeRect = _clientRC;
	OffsetRect(&relativeRect, _relativePoint.x, _relativePoint.y);
	return relativeRect;
}

RECT UIWindowBase::GetAbsoluteRect() {
	RECT abusoluteRect = _clientRC;
	OffsetRect(&abusoluteRect, _abusolutePoint.x, _abusolutePoint.y);
	return abusoluteRect;
}

bool UIWindowBase::IsWindowFocus() {
	return _isFocus;
}

bool UIWindowBase::IsWindowShow() {
	return _isShow;
}

void UIWindowBase::ShowWindow(bool flag) {
	_isShow = flag;
}

void UIWindowBase::MoveWindow(const RECT& relativeRect) {
	if (p_parentUIContainer == nullptr) {
		return;
	}

	POINT p = p_parentUIContainer->GetBindWindowAbusolutePoint();
	POINT tempPoint = UIShape2D::CreatePoint()(p.x+relativeRect.left, p.y+relativeRect.top);

	// if the position does not change, do not update the calculation
	if (CompareRects()(relativeRect, GetRelativeRect()) && ComparePoints()(_abusolutePoint, tempPoint)) {
		return;
	}

	// position calculation
	_relativePoint = UIShape2D::CreatePoint()(relativeRect.left, relativeRect.top);
	//
	_abusolutePoint = tempPoint;
	//
	_clientRC = relativeRect;
	OffsetRect(&_clientRC, -relativeRect.left, -relativeRect.top);

	//
	this->HandleMessage(WM_SIZE, 0, (GetRectHeight()(relativeRect) << 16)+GetRectWidth()(relativeRect));
}

UIContainer* UIWindowBase::GetUIContainer() {
	return p_UIContainer;
}

void UIWindowBase::SetWindowFocus() {
	p_parentUIContainer->SetFocusOnChild(this);
}


void UIWindowBase::SetAsPopup(bool isPopup) {
	if (_isPopup == isPopup) {
		return; // No change needed
	}

	_isPopup = isPopup;
	_z = (!isPopup ? 1.0f : gPopupZ) - (_layoutLevel - 1) * gLevelZ;

    if (p_UIContainer != nullptr) {
        p_UIContainer->ForEachWindow([isPopup](UIWindowBase* pWin) {
            pWin->SetAsPopup(isPopup);
        });
    }
}

void UIWindowBase::SetTransformMatrix(const XMMATRIX& transformMatrix) {
	_transformMatrix = transformMatrix;
}

XMMATRIX UIWindowBase::GetTransformMatrix() {
	return _transformMatrix;
}

XMMATRIX UIWindowBase::GetInheritedTransformMatrix() {
	XMMATRIX transformMatrix = p_parentUIContainer->GetBindWindowInheritedTransformMatrix();
	return transformMatrix * _transformMatrix;
}

int UIWindowBase::GetRenderLayout() {
	return (!_isPopup ? _layoutLevel * 100 : 5000) + _layoutOrder * 10;
}

UIContainer::UIContainer() {
	_isBindDUI = true;
	_pBindDUIWin = nullptr;

	_preMousePt = UIShape2D::CreatePoint()(0, 0);
}

UIContainer::~UIContainer() {
}

void UIContainer::BindWindow(UIWindowBase *pBindWindow) {
	_isBindDUI = true;
	_pBindDUIWin = pBindWindow;
}

void UIContainer::BindWindow() {
	_isBindDUI = false;
}

void UIContainer::AddChild(UIWindowBase* pWin) {
	// add to sub-window list
	_winList.push_back(UIElement(pWin));

	// initialize the layout information of the sub-window
	RECT parentRect;
	if (_isBindDUI == true) {
		parentRect = _pBindDUIWin->GetClientRect();
	}
	else {
		::GetClientRect(UIFrame::GetSingletonInstance()->GetWindowHandle(), &parentRect);
	}
	//
	_winList.back()._layoutInfo.SetLayoutMode(pWin->_layoutMode);
	_winList.back()._layoutInfo.InitLayout(parentRect, pWin->GetRelativeRect());
}

void UIContainer::DelChild(UIWindowBase* pWin)
{
	// delete from sub-window list
	UIElementListType::iterator it = find(_winList.begin(), _winList.end(), pWin);
	if (it == _winList.end()) {
		return;
	}

	// All heap objects are managed by UIWidgetManage, no manual deletion
	_winList.erase(it);
}

void UIContainer::SetFocusOnChild(UIWindowBase* pNew) {
	UIWindowBase* pOld = GetFocusOnChild();

	if (pOld != nullptr && pOld != pNew) {
		pOld->_isFocus = false;
		pOld->HandleMessage(WM_KILLFOCUS, NULL, NULL);
	}

	pNew->_isFocus = true;
	pNew->HandleMessage(WM_SETFOCUS, NULL, NULL);
}

void UIContainer::SetUnFocusAll() {
	for (UINT i=0; i<_winList.size(); ++i) {	
		if (_winList[i].p_win->_isShow==false) {
			continue;
		}

		if (_winList[i].p_win->_isFocus==true) {
			_winList[i].p_win->_isFocus = false;
			_winList[i].p_win->HandleMessage(WM_KILLFOCUS, NULL, NULL);
		}
	}
}

UIWindowBase* UIContainer::GetFocusOnChild() {
	for (UINT i=0; i<_winList.size(); ++i) {	
		if (_winList[i].p_win->_isShow==false) {
			continue;
		}

		if (_winList[i].p_win->_isFocus==true) {
			return _winList[i].p_win;
		}
	}
	return nullptr;
}

UIWindowBase* UIContainer::GetMinZChild(const POINT& pt) {
	vector<int> hoverIndexList;

	// find all hover sub-windows
	for (UINT i=0;i<_winList.size();++i) {
		if (_winList[i].p_win->_isShow==false) {
			continue;
		}

		if (ContainsPoint()(pt, _winList[i].p_win->GetAbsoluteRect())==true) {
			hoverIndexList.push_back(i);
		}
	}
	// if there are no hover sub-windows, return nullptr
	if (hoverIndexList.size()==0) {
		return nullptr;
	}
	
	// find the sub-window with the highest layout level, if there are the same, return the first one found
	UIWindowBase* pMinZWin = _winList[hoverIndexList[0]].p_win;
	for (UINT i=1; i<hoverIndexList.size(); ++i) {
		if (_winList[hoverIndexList[i]].p_win->_z < pMinZWin->_z) {
			pMinZWin = _winList[hoverIndexList[i]].p_win;
		}
	}

	return pMinZWin;
}

POINT UIContainer::GetBindWindowAbusolutePoint() {
	if (_isBindDUI==true) {
		return _pBindDUIWin->_abusolutePoint;
	}

	return UIShape2D::CreatePoint()(0, 0);
}


RECT UIContainer::GetBindWindowAbusoluteRect() {
	if (_isBindDUI==true) {
		return _pBindDUIWin->GetAbsoluteRect();
	}

	return NULL_RECT;
}

int UIContainer::GetBindWindowLayoutLevel() {
	if (_isBindDUI==true) {
		return _pBindDUIWin->_layoutLevel;
	}

	return 0;
}

bool UIContainer::IsBindWindowPopup() {
	if (_isBindDUI==true) {
		return _pBindDUIWin->_isPopup;
	}

	return false;
}

void UIContainer::SendMessageToBindWindow(UINT message, WPARAM wParam, LPARAM lParam) {
	if (_isBindDUI==true) {
		UIPostMessage(_pBindDUIWin, message, wParam, lParam);
	}
	else {
		::PostMessage(UIFrame::GetSingletonInstance()->GetWindowHandle(), message, wParam, lParam);
	}
}

bool UIContainer::HandleMessagePre(UINT message, WPARAM wParam, LPARAM lParam) {
	UIWindowBase* pMinZWin = nullptr;
	bool isMsgHandled = false;

	switch (message) {
		case WM_DESTROY: {
			for (int i = (int)_winList.size() - 1; i >= 0; --i) {
				_winList[i].p_win->HandleMessage(WM_DESTROY, 0, 0);
			}
		} break;
		case WM_SIZE: {
			//SetUnFocusAll();
			SIZE size;
			size.cx = LOWORD(lParam);
			size.cy = HIWORD(lParam);

			// calculate the zoom of the sub-window
			for (UINT i=0; i<_winList.size(); ++i) {
				RECT newRect;
				if (_winList[i]._layoutInfo._zoomModeflag==UILayoutCalc::NO_ZOOM) {
					newRect = _winList[i].p_win->GetRelativeRect();
				}
				else {
					newRect = _winList[i]._layoutInfo.CalcLayout(size.cx, size.cy);
				}

				_winList[i].p_win->MoveWindow(newRect);
			}
		} break;
		//case WM_MOUSEHOVER:
		case WM_MOUSEMOVE: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			// record the current point as the previous point
			POINT prePoint = _preMousePt;
			_preMousePt = pt;

			// find the sub-window that the mouse has left
			vector<int> leaveIndexList;
			for (UINT i=0; i<_winList.size(); ++i) {
				if (_winList[i].p_win->_isShow==false) {
					continue;
				}

				if (ContainsPoint()(pt, _winList[i].p_win->GetAbsoluteRect())==false) {
					if (ContainsPoint()(prePoint, _winList[i].p_win->GetAbsoluteRect())==true) {
						leaveIndexList.push_back(i);
					}
				}
			}
			// process all sub-windows that the mouse has left
			if (leaveIndexList.size()!=0) {
				// find the sub-window with the highest layout level
				pMinZWin = _winList[leaveIndexList[0]].p_win;
				for (UINT i=1; i<leaveIndexList.size(); ++i) {
					if (_winList[leaveIndexList[i]].p_win->_z<pMinZWin->_z) {
						pMinZWin = _winList[leaveIndexList[i]].p_win;
					}
				}
				pMinZWin->HandleMessage(WM_MOUSELEAVE, wParam, lParam);
			}

			// find the sub-window with the highest layout level that the mouse is currently on, and pass WM_MOUSEMOVE&WM_MOUSEHOVER messages
			pMinZWin = GetMinZChild(pt);
			if (pMinZWin != nullptr) {
				pMinZWin->HandleMessage(WM_MOUSEHOVER, wParam, lParam);           //?
				if (!ComparePoints()(pt, prePoint)) {
					isMsgHandled = pMinZWin->HandleMessage(WM_MOUSEMOVE, wParam, lParam);
				}
			}
		} break;
		case WM_MOUSEWHEEL: {
			UIWindowBase* pChild = GetFocusOnChild();
			if (pChild == nullptr) {
				break;
			}

			isMsgHandled = pChild->HandleMessage(WM_MOUSEWHEEL, wParam, lParam);
		} break;
		case WM_LBUTTONDOWN: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			pMinZWin = GetMinZChild(pt);
			if (pMinZWin != nullptr) {
				SetFocusOnChild(pMinZWin);
				isMsgHandled = pMinZWin->HandleMessage(WM_LBUTTONDOWN, wParam, lParam);
			}
			else {
				SetUnFocusAll();
			}
		} break;
		case WM_LBUTTONUP: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			pMinZWin = GetMinZChild(pt);
			if (pMinZWin != nullptr) {
				isMsgHandled = pMinZWin->HandleMessage(WM_LBUTTONUP, wParam, lParam);
			}
		} break;
		case WM_RBUTTONDOWN: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			pMinZWin = GetMinZChild(pt);
			if (pMinZWin != nullptr) {
				SetFocusOnChild(pMinZWin);
				isMsgHandled = pMinZWin->HandleMessage(WM_RBUTTONDOWN, wParam, lParam);
			}
		} break;
		case WM_RBUTTONUP: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			pMinZWin = GetMinZChild(pt);
			if (pMinZWin != nullptr) {
				isMsgHandled = pMinZWin->HandleMessage(WM_RBUTTONUP, wParam, lParam);
			}
		} break;
		case WM_LBUTTONDBLCLK: {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			pMinZWin = GetMinZChild(pt);
			if (pMinZWin != nullptr) {
				isMsgHandled = pMinZWin->HandleMessage(WM_LBUTTONDBLCLK, wParam, lParam);
			}
		} break;
		case WM_KEYDOWN: {
			UIWindowBase* pChild = GetFocusOnChild();
			if (pChild == nullptr) {
				break;
			}

			if (!(wParam==0x03 || wParam==0x16 || wParam==0x18)) {
				pChild->HandleMessage(WM_KEYDOWN, wParam, 0);
			}

			//isMsgHandled = true;
		} break;
		case WM_CHAR: {
			UIWindowBase* pChild = GetFocusOnChild();
			if (pChild == nullptr) {
				break;
			}

			if (wParam==0x03) {
				pChild->HandleMessage(WM_COPY, 0, 0);
			}
			else if (wParam==0x16) {
				pChild->HandleMessage(WM_PASTE, 0, 0);
			}
			else if (wParam==0x18) {
				pChild->HandleMessage(WM_CUT, 0, 0);
			}
			else {
				pChild->HandleMessage(WM_CHAR, wParam, 0);
			}

			isMsgHandled = true;
		} break;
		case WM_COPY: {
			UIWindowBase* pChild = GetFocusOnChild();
			if (pChild == nullptr) {
				break;
			}

			isMsgHandled = pChild->HandleMessage(WM_COPY, 0, 0);
		} break;
		case WM_PASTE: {
			UIWindowBase* pChild = GetFocusOnChild();
			if (pChild == nullptr) {
				break;
			}

			isMsgHandled = pChild->HandleMessage(WM_PASTE, 0, 0);
		} break;
		case WM_CUT: {
			UIWindowBase* pChild = GetFocusOnChild();
			if (pChild == nullptr) {
				break;
			}

			isMsgHandled = pChild->HandleMessage(WM_CUT, 0, 0);
		} break;
		case WM_KILLFOCUS: {
			for (UINT i=0;i<_winList.size();++i) {
				if (_winList[i].p_win->_isFocus==true) {
					_winList[i].p_win->_isFocus = false;
					_winList[i].p_win->HandleMessage(message, wParam, lParam);
				}
			}
		} break;
	}

	return isMsgHandled;
}

void UIContainer::Draw() {
	for (UINT i=0; i<_winList.size(); ++i) {
		if (_winList[i].p_win->_isShow) {
			_winList[i].p_win->Draw();
		}
	}
}

XMMATRIX UIContainer::GetBindWindowInheritedTransformMatrix() {
	return _isBindDUI ? _pBindDUIWin->GetInheritedTransformMatrix() : XMMatrixIdentity();
}

void UIContainer::ForEachWindow(function<void(UIWindowBase*)> func) {
	for (auto& element : _winList) {
		if (element.p_win != nullptr) {
			func(element.p_win);
		}
	}
}


bool UIMessageFilter1(UINT message) {
	bool rt = false;
	switch (message) {
        case WM_CREATE:
        case WM_DESTROY:
        case WM_PAINT:
        case WM_SIZE:
        case WM_MOUSEHOVER:
        case WM_MOUSELEAVE:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_KEYDOWN:
        case WM_CHAR:
        //case WM_SETFOCUS:
        //case WM_KILLFOCUS:
        case WM_MOUSEWHEEL:
        case WM_COPY:
        case WM_PASTE:
        case WM_CUT:
        case WM_MOVE:
        case WM_COMMAND:
        case WM_NOTIFY:
        case WM_HSCROLL:
        case WM_VSCROLL:
			rt = true;
            break;
	}
	return rt;
}


bool UIMessageFilter2(UINT message) {
	bool rt = false;
	switch (message) {
        case WM_NOTIFY:
        case WM_HSCROLL:
        case WM_VSCROLL:
            rt = true;
            break;
	}
	return rt;
}


void UIPostMessage(UIWindowBase* winPoint, UINT message, WPARAM wParam, LPARAM lParam) {
	// Message queue reception message filter
	if (UIMessageFilter1(message)) {
		UIMessageLoop::GetSingletonInstance()->PushMessage(winPoint, message, wParam, lParam);
        // char debugStr[256];
        // sprintf_s(debugStr, "count:%d, PushMessage: %d wParam=%p, lParam=%p\n",
        //     UIMessageLoop::GetSingletonInstance()->_uiMsgList.size(),
        //     message,
        //     (void*)wParam,
        //     (void*)lParam);
        // OutputDebugStringA(debugStr);
	}
}

void UIRefresh(bool immediately) {
	UIPostMessage(0, WM_PAINT, immediately?1:0, 0);
}

void UIMessageLoop::PushMessage(UIWindowBase* winPoint, UINT message, WPARAM wParam, LPARAM lParam) {
	//lock_guard<mutex> lock(_uiMsgListMutex);
    CSGuard lock2(_uiMsgListCS);
    
    UIMessage newMsg(winPoint, message, wParam, lParam);

	if (message==WM_PAINT) {
        wParam == 1 ? _uiMsgList.push_front(newMsg) : _uiMsgList.push_back(newMsg);

		_uiMsgList.erase(unique(_uiMsgList.begin(), 
								_uiMsgList.end(), 
								[](UIMessage& msg1, UIMessage& msg2) { return (msg1._msg==WM_PAINT) && (msg2._msg==WM_PAINT); } 
							   ), 
						 _uiMsgList.end());
	}
    else {
        _uiMsgList.push_back(newMsg);
    }
}

bool UIMessageLoop::PopMessage(UIMessage& msg) {
	//lock_guard<mutex> lock(_uiMsgListMutex);
    CSGuard lock2(_uiMsgListCS);

    if (_uiMsgList.empty()) {
        return false; 
    }

    msg = _uiMsgList.front();
    _uiMsgList.pop_front();
    return true;
}

void UIMessageLoop::RunMessageLoopThread() {
    TimerHelper timer(33);

    bool shouldExit = false;
    while (!shouldExit) {
        timer.BeginWait();

		bool needsRepaint = false;  // repaint flag

		// Step 1: Process all UI update closures first
		if (UIEventQueue::GetSingletonInstance()->ProcessAll()) {
			needsRepaint = true;
		}		
		
		// Step 2: Process all messages in the message queue until the queue is empty or the time limit is reached
        UIMessage msg;
		while (PopMessage(msg)) {
			if (msg.p_win == nullptr && msg._msg == WM_DESTROY) {
				shouldExit = true;
			}

			if (msg._msg == WM_PAINT) {
				needsRepaint = true;
			}
			else {
                HandleMessage(msg.p_win, msg._msg, msg._wParam, msg._lParam);
			}

			// if the time limit is reached, exit the message loop
			if (timer.BreakWait()) {
                break;
            }
		}

		// Step 3: Process animation switch
        bool isUpdateAnimation = UIAnimationManage::GetSingletonInstance()->UpdateAnimations();

		// if the repaint flag is true, then refresh
		if (needsRepaint || isUpdateAnimation) {
            UIDXFoundation::GetSingletonInstance()->Render();
		}

        timer.EndWait();
	}
}

void UIMessageLoop::RunMessageLoop() { 
	StartThread(&UIMessageLoop::RunMessageLoopThread);
}

bool UIMessageLoop::HandleMessage(UIWindowBase* winPoint, UINT message, WPARAM wParam, LPARAM lParam) {
	// Game message processing is generally used to process pure animation objects
	//if (winPoint == nullptr)
	//	UIWinShell::_pWinShell->HandleMessageGame(message, wParam, lParam);

	bool rt = false;
	if (winPoint == nullptr) {
		if (UIFrame::GetSingletonInstance()->HandleMessageFromHookWindow(message, wParam, lParam)) {
			return true;
		}

        // handle top-level messages
		rt = UIFrame::GetSingletonInstance()->GetTopUIContainer()->HandleMessagePre(message, wParam, lParam);
	}
	else{	
        // handle UIWindowBase window messages
		rt = winPoint->HandleMessage(message, wParam, lParam);
	}

	return rt;
}

void UIEventQueue::Post(UIEventFunc func) {
	std::lock_guard<std::mutex> lock(_queueMutex);
	_eventQueue.push_back(func);
}

bool UIEventQueue::ProcessAll() {
	std::deque<UIEventFunc> localQueue;
	
	// Step 1: Swap the global queue with local queue under lock, then immediately release the lock
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		
		if (_eventQueue.empty()) {
			return false;
		}
		
		// Swap is O(1) operation: only pointers are exchanged
		// After swap: _eventQueue becomes empty, localQueue contains all events
		localQueue.swap(_eventQueue);
	}
	// Lock is released here
	
	// Step 2: Execute all events without holding any lock
	// This allows nested UIPostEvent() calls to work safely
	while (!localQueue.empty()) {
		auto func = localQueue.front();
		localQueue.pop_front();
		
		func();
	}
	
	return true;
}


UIFrame::UIFrame() {
	_windowHWnd = nullptr;
	_pHookWindowForPopup = nullptr;
	_pHookWindowForScrollBar = nullptr;
}

void UIFrame::Initialize(HWND windowHWnd, int width, int height) {
	_windowHWnd = windowHWnd;
	_topUIContainer.BindWindow();

	UIDXFoundation::GetSingletonInstance()->Initialize(width, height);
}

HWND UIFrame::GetWindowHandle() const {
	return _windowHWnd;
}

UIContainer* UIFrame::GetTopUIContainer() {
	return &_topUIContainer;
}

void UIFrame::SetHookWindowForPopup(UIWindowBase* pHookWindow) {
	if (pHookWindow == _pHookWindowForPopup) {
		return;
	}

	if (_pHookWindowForPopup != nullptr) {
		_pHookWindowForPopup->ShowWindow(false);
	}

	_pHookWindowForPopup = pHookWindow;
	if (_pHookWindowForPopup != nullptr) {
		_pHookWindowForPopup->SetAsPopup();
	}
}

void UIFrame::SetHookWindowForScrollBar(UIWindowBase* pHookWindow) {
	_pHookWindowForScrollBar = pHookWindow;
}

bool UIFrame::HandleMessageFromHookWindow(UINT message, WPARAM wParam, LPARAM lParam) {
	if (_pHookWindowForScrollBar != nullptr && _pHookWindowForScrollBar->IsWindowShow()) {
		// Judge if the lbutton is down status
		if( IsKeyDown()(VK_LBUTTON) ) {
			if (message == WM_MOUSEMOVE) {
				_pHookWindowForScrollBar->HandleMessage(message, wParam, lParam);
				return true;
			}
		} else {
			_pHookWindowForScrollBar = nullptr;
		}
	}
	
	if (_pHookWindowForPopup != nullptr && _pHookWindowForPopup->IsWindowShow()) {
		// judge if the mouse is in the popup window
		if (message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN || message == WM_MOUSEMOVE) {
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			if (ContainsPoint()(pt, _pHookWindowForPopup->GetAbsoluteRect())==false) {
				if (message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN) {
					_pHookWindowForPopup->ShowWindow(false);
					_pHookWindowForPopup = nullptr;
					UIHideCaret();
					UIRefresh(true); // force refresh
				}

				return false;
			}
		}

		return _pHookWindowForPopup->HandleMessage(message, wParam, lParam);
	}

	return false;
}

