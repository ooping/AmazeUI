#include "UIUtility.h"
using namespace std;
using namespace Shape2D;


RECT CreateRect::operator()(POINT point, SIZE size) {
	return {point.x, point.y, point.x + size.cx, point.y + size.cy};
}

RECT CreateRect::operator()(LONG left, LONG top, LONG right, LONG bottom) {
    return {left, top, right, bottom};
}

POINT Shape2D::CreatePoint::operator()(LONG x, LONG y) {
    return {x, y};
}

SIZE CreateSize::operator()(LONG x, LONG y) {
    return {x, y};
}

LONG GetRectWidth::operator()(const RECT& rc) {
    return rc.right - rc.left;
}

LONG GetRectHeight::operator()(const RECT& rc) {
    return rc.bottom - rc.top;
}

POINT GetRectCenter::operator()(const RECT& rc) {
    return {rc.left + (rc.right - rc.left) / 2, rc.top + (rc.bottom - rc.top) / 2};
}

bool CompareRects::operator()(const RECT& r1, const RECT& r2) {
    return r1.left == r2.left && r1.top == r2.top && r1.right == r2.right && r1.bottom == r2.bottom;
}

RECT IntersectRects::operator()(const RECT& rc1, const RECT& rc2) {
	if ((rc1.right<=rc2.left)||(rc2.right<=rc1.left)||(rc1.bottom<=rc2.top)||(rc2.bottom<=rc1.top)) {
		return NULL_RECT;
	}
    
	return CreateRect()(max(rc1.left, rc2.left), max(rc1.top, rc2.top), min(rc1.right, rc2.right), min(rc1.bottom, rc2.bottom));
}

bool ComparePoints::operator()(const POINT& p1, const POINT& p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

bool ContainsPoint::operator()(const POINT& point, const RECT& rect) {
    return point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom;
}

RECT ScaleRect::operator()(const RECT& rc, float scale) {
    int dx = static_cast<int>((scale - 1) * GetRectWidth()(rc) / 2);
    int dy = static_cast<int>((scale - 1) * GetRectHeight()(rc) / 2);
    
    return CreateRect()(rc.left-dx/2, rc.top-dy/2, rc.right+dx/2, rc.bottom+dy/2);
}

bool IsKeyDown::operator()(int key) {
	return GetKeyState(key)&0x80 ? true:false;
}


UILayoutCalc::UILayoutCalc(int flag) {
	_zoomModeflag=flag;
}

void UILayoutCalc::SetLayoutMode(int flag) {
	_zoomModeflag = flag;
}

void UILayoutCalc::InitLayout(const RECT& parentRect, const RECT& rect) {
	_parentRect = parentRect;
	_rect = rect;
}

// Calculate the new state
RECT UILayoutCalc::CalcLayout(LONG cx, LONG cy) {
	RECT newRect = _rect;

	if (_zoomModeflag==0) {
		return newRect;
	}

	if (_zoomModeflag&1) {
		newRect.left = cx-(_parentRect.right-_rect.left);
		newRect.right = newRect.left+GetRectWidth()(_rect);
	}
	
	if (_zoomModeflag&2) {
		newRect.top = cy-(_parentRect.bottom-_rect.top);
		newRect.bottom = newRect.top+GetRectHeight()(_rect);
	}

	if (_zoomModeflag&4) {
		newRect.right = cx-(_parentRect.right-_rect.right);
	}

	if (_zoomModeflag&8) {
		newRect.bottom = cy-(_parentRect.bottom-_rect.bottom);
	}

	if (_zoomModeflag&16) {
		newRect.left = (LONG)(((float)_rect.left/GetRectWidth()(_parentRect))*cx);
		newRect.right = newRect.left + (LONG)(((float)GetRectWidth()(_rect)/GetRectWidth()(_parentRect))*cx);
	}

	if (_zoomModeflag&32) {
		newRect.top = (LONG)(((float)_rect.top/GetRectHeight()(_parentRect))*cy);
		newRect.bottom = newRect.top + (LONG)(((float)GetRectHeight()(_rect)/GetRectHeight()(_parentRect))*cy);
	}

	// Prevent negative values caused by minimization
	if (newRect.left < 0) {
		newRect.left = 0;
	}
	if (newRect.right < 0) {
		newRect.right = 0;
	}
	if (newRect.top < 0) {
		newRect.top = 0;
	}
	if (newRect.bottom < 0) {
		newRect.bottom = 0;
	}

	// Prevent exceptions caused by minimization
	if (newRect.left > newRect.right) {
		newRect.left = newRect.right;
	}

	if (newRect.top > newRect.bottom) {
		newRect.top = newRect.bottom;
	}

	return newRect;
}







