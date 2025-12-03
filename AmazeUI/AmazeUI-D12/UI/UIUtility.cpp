#include "UIUtility.h"
using namespace std;
using namespace UIShape2D;


UIRECT CreateRect::operator()(UIPOINT point, UISIZE size) {
	return {point.x, point.y, point.x + size.cx, point.y + size.cy};
}

UIRECT CreateRect::operator()(LONG left, LONG top, LONG right, LONG bottom) {
    return {left, top, right, bottom};
}

UIPOINT UIShape2D::CreatePoint::operator()(LONG x, LONG y) {
    return {x, y};
}

UISIZE CreateSize::operator()(LONG x, LONG y) {
    return {x, y};
}

LONG GetRectWidth::operator()(const UIRECT& rc) {
    return rc.right - rc.left;
}

LONG GetRectHeight::operator()(const UIRECT& rc) {
    return rc.bottom - rc.top;
}

UIPOINT GetRectCenter::operator()(const UIRECT& rc) {
    return {rc.left + (rc.right - rc.left) / 2, rc.top + (rc.bottom - rc.top) / 2};
}

bool CompareRects::operator()(const UIRECT& r1, const UIRECT& r2) {
    return r1.left == r2.left && r1.top == r2.top && r1.right == r2.right && r1.bottom == r2.bottom;
}

UIRECT IntersectRects::operator()(const UIRECT& rc1, const UIRECT& rc2) {
	if ((rc1.right<=rc2.left)||(rc2.right<=rc1.left)||(rc1.bottom<=rc2.top)||(rc2.bottom<=rc1.top)) {
		return NULL_RECT;
	}
    
	return CreateRect()(max(rc1.left, rc2.left), max(rc1.top, rc2.top), min(rc1.right, rc2.right), min(rc1.bottom, rc2.bottom));
}

bool ComparePoints::operator()(const UIPOINT& p1, const UIPOINT& p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

bool ContainsPoint::operator()(const UIPOINT& point, const UIRECT& rect) {
    return point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom;
}

UIRECT ScaleRect::operator()(const UIRECT& rc, float scale) {
    int dx = static_cast<int>((scale - 1) * GetRectWidth()(rc) / 2);
    int dy = static_cast<int>((scale - 1) * GetRectHeight()(rc) / 2);
    
    return CreateRect()(rc.left-dx/2, rc.top-dy/2, rc.right+dx/2, rc.bottom+dy/2);
}

UILayoutCalc::UILayoutCalc(int flag) {
	_zoomModeflag=flag;
}

void UILayoutCalc::SetLayoutMode(int flag) {
	_zoomModeflag = flag;
}

void UILayoutCalc::InitLayout(const UIRECT& parentRect, const UIRECT& rect) {
	_parentRect = parentRect;
	_rect = rect;
}

// Calculate the new state
UIRECT UILayoutCalc::CalcLayout(LONG cx, LONG cy) {
	UIRECT newRect = _rect;

	if (_zoomModeflag == 0) {
		return newRect;
	}

	if (_zoomModeflag & 1) {
		newRect.left = cx - (_parentRect.right - _rect.left);
		newRect.right = newRect.left + GetRectWidth()(_rect);
	}

	if (_zoomModeflag & 2) {
		newRect.top = cy - (_parentRect.bottom - _rect.top);
		newRect.bottom = newRect.top + GetRectHeight()(_rect);
	}

	if (_zoomModeflag & 4) {
		newRect.right = cx - (_parentRect.right - _rect.right);
	}

	if (_zoomModeflag & 8) {
		newRect.bottom = cy - (_parentRect.bottom - _rect.bottom);
	}

	if (_zoomModeflag & 16) {
		newRect.left = (LONG)(((float)_rect.left / GetRectWidth()(_parentRect)) * cx);
		newRect.right = newRect.left + (LONG)(((float)GetRectWidth()(_rect) / GetRectWidth()(_parentRect)) * cx);
	}

	if (_zoomModeflag & 32) {
		newRect.top = (LONG)(((float)_rect.top / GetRectHeight()(_parentRect)) * cy);
		newRect.bottom = newRect.top + (LONG)(((float)GetRectHeight()(_rect) / GetRectHeight()(_parentRect)) * cy);
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
