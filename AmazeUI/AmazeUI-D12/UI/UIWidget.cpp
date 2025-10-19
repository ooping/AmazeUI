#include "UIWidget.h"
#include "UIApplication.h"


using namespace std;
using namespace DirectX;
using namespace UIShape2D;

const float gDefaultFontSize = 18.0f;

const float gDeltaZ = 0.005f;



UILayoutGrid::CellInfo::CellInfo() {
	p_win = NULL;
	_endRow = -1;
	_endColumn = -1;
}

void UILayoutGrid::InitPoint(const POINT& relativePT) {
	_relativePoint = relativePT;
}

void UILayoutGrid::SetRowColumn(UINT row, UINT column, int width, int widthInterval, int height, int heightInterval) {
	if (row == 0) {
		return;
	}

	// set grid size
	_grid.resize(row);
	for (UINT r = 0; r < _grid.size(); ++r) {
		_grid[r].resize(column);
	}

	// initialize cell width and interval
	_columnWidthList.resize(column, width);
	_columnIntervalList.resize(column, widthInterval);
	//
	_rowHeightList.resize(row, height);
	_rowIntervalList.resize(row, heightInterval);
}

void UILayoutGrid::SetCell(UINT bRow, UINT bColumn, UINT eRow, UINT eColumn, UIWindowBase* pWin) {
	if (_grid.size() == 0) {
		return;
	}
	if (eRow >= _grid.size() || eColumn >= _grid[0].size()) {
		return;
	}
	if (bRow > eRow || bColumn > eColumn) {
		return;
	}

	_grid[bRow][bColumn].p_win = pWin;
	_grid[bRow][bColumn]._endRow = eRow;
	_grid[bRow][bColumn]._endColumn = eColumn;
}

void UILayoutGrid::SetCell(UINT row, UINT column, UIWindowBase* pWin) {
	SetCell(row, column, row, column, pWin);
}

// set length and width and interval
void UILayoutGrid::SetColumnWidthInterval(UINT column, int width, int interval) {
	if (column >= _columnIntervalList.size()) {
		return;
	}

	_columnWidthList[column] = width;
	_columnIntervalList[column] = interval;
}

void UILayoutGrid::SetRowHeightInterval(UINT row, int heigth, int interval) {
	if (row >= _rowIntervalList.size()) {
		return;
	}

	_rowHeightList[row] = heigth;
	_rowIntervalList[row] = interval;
}

void UILayoutGrid::CalcGridPos() {
	if (_grid.size() == 0) {
		return;
	}

	LONG& x_ = _relativePoint.x;
	LONG& y_ = _relativePoint.y;

	for (UINT r = 0; r < _grid.size(); ++r) {
		for (UINT c = 0; c < _grid[0].size(); ++c) {
			if (_grid[r][c].p_win == NULL) {
				continue;
			}

			int sum1 = accumulate(_columnWidthList.begin(), _columnWidthList.begin() + c, 0);
			sum1 += accumulate(_columnIntervalList.begin(), _columnIntervalList.begin() + c, 0);

			int sum2 = accumulate(_rowHeightList.begin(), _rowHeightList.begin() + r, 0);
			sum2 += accumulate(_rowIntervalList.begin(), _rowIntervalList.begin() + r, 0);

			int sum3 = accumulate(_columnWidthList.begin() + c, _columnWidthList.begin() + _grid[r][c]._endColumn + 1, 0);
			sum3 += accumulate(_columnIntervalList.begin() + c, _columnIntervalList.begin() + _grid[r][c]._endColumn, 0);

			int sum4 = accumulate(_rowHeightList.begin() + r, _rowHeightList.begin() + _grid[r][c]._endRow + 1, 0);
			sum4 += accumulate(_rowIntervalList.begin() + r, _rowIntervalList.begin() + _grid[r][c]._endRow, 0);

			_grid[r][c].p_win->MoveWindow(CreateRect()(CreatePoint()(x_ + sum1, y_ + sum2), CreateSize()(sum3, sum4)));
		}
	}
}





UILabel::UILabel() {
	_color = Colors::Black;
	_fontHeight = gDefaultFontSize;
	_pos = UIFontPos::MiddleLeft;
	_supportMultiLine = false;
}

void UILabel::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLayout = GetRenderLayout();

	if (_supportMultiLine) {
		UIFont(_z, _fontHeight, renderLayout + 5)(_text, GetAbsoluteRect(), _color, _pos, _lineSpacing, transformMatrix);
	}
	else {
		// single line text
		UIFont(_z, _fontHeight, renderLayout + 5)(_text, GetAbsoluteRect(), _color, _pos, transformMatrix);
	}
}

void UILabel::SetText(UIString text, UIFontPos pos) {
	_text = text;
	_pos = pos;
}

UIString UILabel::GetText() {
	return _text;
}

void UILabel::SetColor(UIColor& color) {
	_color = color;
}

void UILabel::SetFontHeight(float h) {
	_fontHeight = h;
}

void UILabel::SetPos(UIFontPos pos) {
	_pos = pos;
}

void UILabel::SetSupportMultiLine(bool support, float lineSpacing) {
	_supportMultiLine = support;
	_lineSpacing = lineSpacing;
}



void UIImageView::SetDLLPath(wstring path, const UIColor& colorKey) {
	_dllPath = path;
	_colorKey = colorKey;
	_loadImageWay = 1;
}

void UIImageView::SetDLLID(int id) {
	_imageID = id;

	int renderLevel = GetRenderLayout();
	_image = UIImage(_dllPath.c_str(), _imageID, UIColor::Invalid, _z, renderLevel);
}

void UIImageView::SetImagePath(wstring path, const UIColor& colorKey) {
	_imagePath = path;
	_colorKey = colorKey;
	_loadImageWay = 2;

	int renderLevel = GetRenderLayout();
	_image = UIImage(_imagePath, UIColor::Invalid, _z, renderLevel);
}

void UIImageView::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	int centerX = GetRectWidth()(_clientRC) / 2 + x_;
	int centerY = GetRectHeight()(_clientRC) / 2 + y_;


	RECT _imageRect;
	_image.GetSize(_imageRect);
	int imageWidth = GetRectWidth()(_imageRect);
	int imageHeight = GetRectHeight()(_imageRect);

	int clientWidth = GetRectWidth()(_clientRC);
	int clientHeight = GetRectHeight()(_clientRC);

	float scale = 1.0f;
	if (imageWidth > clientWidth) {
		scale = static_cast<float>(clientWidth) / imageWidth;
	}
	if (imageHeight > clientHeight) {
		scale = min(scale, static_cast<float>(clientHeight) / imageHeight);
	}

	if (!IsAnimationRun()) {
		if (_loadImageWay == 1) {
			_image(centerX, centerY, scale, 255, transformMatrix);
		}
		else if (_loadImageWay == 2) {
			_image(centerX, centerY, scale, 255, transformMatrix);
		}
	}
	else {
		DrawHitDrumAnimate(_image, centerX, centerY, scale, transformMatrix);
	}
}


UIButton::UIButton() {
	SetHitPower(0.3f);

	_isHover = false;
	_isLButtonDown = false;
}

void UIButton::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	int frameBMPID = IDB_BUTTON1_NORMAL;
	if (_isLButtonDown == true) {
		frameBMPID = IDB_BUTTON1_DOWN;
	}
	else if (_isHover == true) {
		frameBMPID = IDB_BUTTON1_HOT;
	}
	UISlicedImage slicedImage(L"GUIResource.dll", frameBMPID, UIColor(255, 0, 255), 3, 3, 3, 3, _z, renderLevel);

	if (!IsAnimationRun()) {
		slicedImage(x_, y_, GetRectWidth()(_clientRC), GetRectHeight()(_clientRC), 255, transformMatrix);
	}
	else {
		RECT rc = _clientRC;
		OffsetRect(&rc, x_, y_);
		DrawSlicedHitDrumAnimate(slicedImage, rc, transformMatrix);
	}

	// display wstring (text offset = 5)
	UIFont fontHelp(_z - gDeltaZ, gDefaultFontSize, renderLevel + 5);
	RECT rc;
	if (_isLButtonDown == false) {
		rc = _strRect;
	}
	else {
		rc = _strRect2;
	}
	OffsetRect(&rc, x_, y_);
	fontHelp(_text, rc, UIColor::Black, UIFontPos::MiddleCenter, transformMatrix);
}

void UIButton::CalcArea() {
	// calculate the normal state wstring display area
	_strRect.left = _clientRC.left + 1;
	_strRect.right = _clientRC.right - 1;
	_strRect.top = _clientRC.top + 1;
	_strRect.bottom = _clientRC.bottom - 1;

	// calculate the pressed state wstring display area
	_strRect2 = _strRect;
	_strRect2.left += 1;
	_strRect2.right += 1;
	_strRect2.top += 1;
	_strRect2.bottom += 1;
}

void UIButton::SetText(UIString text) {
	_text = text;
}

UIString UIButton::GetText() {
	return _text;
}

bool UIButton::OnMouseMove(POINT) {
	if (!_isHover) {
		OnMouseHoverEvent();
		_isHover = true;
		UIRefresh();
	}

	return true;
}

void UIButton::OnMouseLeave(POINT) {
	_isHover = false;
	_isLButtonDown = false;
	UIRefresh();
}

bool UIButton::OnLButtonDown(POINT) {
	_isLButtonDown = true;
	UIRefresh();

	return true;
}

bool UIButton::OnLButtonUp(POINT) {
	if (_isLButtonDown) {
		if (!OnClickEvent() && _id != 0) {
			SendMessageToParent(WM_NOTIFY, (WPARAM)_id, 0);
		}

		PlayHitDrumAnimate();
	}

	_isLButtonDown = false;
	return true;
}

void UIButton::OnKeyDown(TCHAR nChar) {
	if (nChar == VK_RETURN) {
		SendMessageToParent(WM_NOTIFY, (WPARAM)_id, 0);

		PlayHitDrumAnimate();
	}
}



UICheckButton::UICheckButton() {
	_isHover = false;
	_isCheck = false;
	_fontHeight = gDefaultFontSize;
}

void UICheckButton::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	int checkBoxBMPID;
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw check box
	if (_isCheck && !_isHover) {
		checkBoxBMPID = IDB_CHECKBOX1_TICK_NOR;
	}
	else if (!_isCheck && !_isHover) {
		checkBoxBMPID = IDB_CHECKBOX1_NOR;
	}
	else if (_isCheck && _isHover) {
		checkBoxBMPID = IDB_CHECKBOX1_TICK_HOT;
	}
	else {
		checkBoxBMPID = IDB_CHECKBOX1_HOT;
	}

	// button image (offset = 1 for checkbox images)
	if (!_isHover) {
		UIImage(L"GUIResource.dll", checkBoxBMPID, UIColor(255, 0, 255), _z, renderLevel)(GetRectWidth()(_checkRect) / 2 + x_, GetRectHeight()(_checkRect) / 2 + y_, 1.0, 255, transformMatrix);
	}
	else {
		UIImage(L"GUIResource.dll", checkBoxBMPID, UIColor(255, 0, 255), _z, renderLevel)(GetRectWidth()(_checkRect) / 2 + x_, GetRectHeight()(_checkRect) / 2 + y_, 1.0, 255, transformMatrix);
	}

	// display wstring (text offset = 5)
	RECT rc = _strRect;
	OffsetRect(&rc, x_, y_);
	UIFont fontHelp(_z, _fontHeight, renderLevel + 5);
	fontHelp(_text, rc, UIColor::Black, UIFontPos::MiddleLeft, transformMatrix);
}

void UICheckButton::CalcArea() {
	//
	_checkRect = _clientRC;
	_checkRect.right = 30;

	// calculate the normal state wstring display area
	_strRect.left = _checkRect.right;
	_strRect.right = _clientRC.right;
	_strRect.top = _clientRC.top;
	_strRect.bottom = _clientRC.bottom;
}

void UICheckButton::SetFontHeight(float h) {
	_fontHeight = h;
}

void UICheckButton::SetText(UIString text) {
	_text = text;
}

UIString UICheckButton::GetText() {
	return _text;
}

void UICheckButton::SetCheck(bool f) {
	if (f != _isCheck) {
		_isCheck = f;

		// mutex related button
		if (_isCheck == true) {
			for (UINT i = 0; i < _mutexList.size(); ++i) {
				_mutexList[i]->_isCheck = false;
			}
		}
	}
}

bool UICheckButton::GetCheck() {
	return _isCheck;
}

void UICheckButton::SetMutexList(vector<UICheckButton*>mutexList) {
	_mutexList = mutexList;
}

void UISetCheckButtonMutex(vector<UICheckButton*>& mutexList) {
	for (UINT i = 0; i < mutexList.size(); ++i) {
		vector<UICheckButton*> listTemp = mutexList;
		listTemp.erase(listTemp.begin() + i);

		mutexList[i]->SetMutexList(listTemp);
	}
}

void UISetCheckButtonMutex(UICheckButton* but1, UICheckButton* but2) {
	vector<UICheckButton*> mutexList;
	mutexList.push_back(but1);
	mutexList.push_back(but2);

	UISetCheckButtonMutex(mutexList);
}

bool UICheckButton::OnMouseMove(POINT) {
	if (!_isHover) {
		_isHover = true;
	}

	UIRefresh();
	return true;
}

void UICheckButton::OnMouseLeave(POINT) {
	_isHover = false;
	UIRefresh();
}

bool UICheckButton::OnLButtonDown(POINT) {
	_isCheck = !_isCheck;

	// mutex related button	
	if (_isCheck == true) {
		for (UINT i = 0; i < _mutexList.size(); ++i) {
			_mutexList[i]->_isCheck = false;
		}
	}

	if (!OnClickEvent() && _id != 0) {
		SendMessageToParent(WM_NOTIFY, (WPARAM)_id, _isCheck ? 1 : 0);
	}

	UIRefresh();
	return true;
}



UIEdit::UIEdit() {
	_beginDrawXPos = false;

	_isDrawCaretImmd = false;
	_caretPassChar = 0;

#ifndef _UNICODE
	_isCNInput = false;
#endif

	_isSelArea = 0;										// whether to select area
	_beginAreaPassChar = 0;								// selected area start pass character

	_fontHeight = gDefaultFontSize;
	_fontColor = UIColor::Black;
}

void UIEdit::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw border (offset = 1)
	RECT rc = _clientRC;
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryBlueLight, transformMatrix);

	// draw content
	// calculate and select clip area
	RECT clipRC = _drawRectAllow;
	OffsetRect(&clipRC, x_, y_);

	// get wstring size information
	UIFont fontHelp(_z, _fontHeight, renderLevel + 5);
	SIZE contentSize = fontHelp.GetDrawAreaSize(_text.c_str());
	SIZE caretPassCharSize = fontHelp.GetDrawAreaSize(_text.substr(0, _caretPassChar).c_str());
	SIZE beginAreaPassSize = fontHelp.GetDrawAreaSize(_text.substr(0, _beginAreaPassChar).c_str());

	// calculate drawing area
	_drawRectReal.left = _drawRectAllow.left + _beginDrawXPos;
	_drawRectReal.right = _drawRectReal.left + contentSize.cx;

	if (_isFocus) {
		// calc the caret position
		int xPos = caretPassCharSize.cx + _beginDrawXPos + 5;
		int yPos = (_drawRectAllow.top + _drawRectAllow.bottom) / 2;

		// draw caret
		UISetCaretPos(xPos + x_, yPos + y_, _isDrawCaretImmd, transformMatrix);
		_isDrawCaretImmd = false;

		// draw selected area background color (offset = 0 for background)
		if (_isSelArea == 2) {
			UIScreenClipRectGuard uiClip(clipRC);

		int xPos2 = beginAreaPassSize.cx + _beginDrawXPos + 5;
		rc = CreateRect()(xPos2 < xPos ? xPos2 : xPos, _drawRectAllow.top + 2, xPos > xPos2 ? xPos : xPos2, _drawRectAllow.bottom - 2);
		OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
		UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPinkLight, 220, transformMatrix);
	}
}
	// draw wstring (text offset = 5, already set in fontHelp)
	{
		UIScreenClipRectGuard uiClip(clipRC);

		RECT drawRect = _drawRectReal;
		OffsetRect(&drawRect, x_, y_);
		fontHelp(_text, drawRect, _fontColor, UIFontPos::MiddleLeft, transformMatrix);
	}
}

void UIEdit::CalcArea() {
	// calculate drawing area
	_drawRectAllow = _clientRC;

	_drawRectAllow.left += 5;
	_drawRectAllow.right -= 5;
	_drawRectAllow.top += 2;
	_drawRectAllow.bottom -= 2;

	_drawRectReal = _drawRectAllow;
}

void UIEdit::SetFontHeight(float h) {
	_fontHeight = h;
}

void UIEdit::SetText(UIString text)
{
	_caretPassChar = 0;
	_isSelArea = 0;
	CalcBeginDrawXPos();

	_text = text;
}

UIString UIEdit::GetText() {
	return _text;
}

int UIEdit::GetTextToInt() {
	return _text.empty() ? 0 : stoi(_text);
}

float UIEdit::GetTextToFloat() {
	return _text.empty() ? 0.0f : stof(_text);
}

double UIEdit::GetTextToDouble() {
	return _text.empty() ? 0.0 : stod(_text);
}

void UIEdit::EraseSelectArea() {
	if (_isSelArea != 2) {
		return;
	}

	if (_caretPassChar > _beginAreaPassChar) {
		size_t dx = _caretPassChar - _beginAreaPassChar;
		_text.erase(_beginAreaPassChar, dx);
		_caretPassChar -= dx;
	}
	else if (_caretPassChar < _beginAreaPassChar) {
		_text.erase(_caretPassChar, _beginAreaPassChar - _caretPassChar);
	}

	_isSelArea = 0;
	CalcBeginDrawXPos();
}

// calculate the x coordinate of the drawing start character, _beginDrawXPos and _caretPassChar together determine the cursor position
void UIEdit::CalcBeginDrawXPos() {
	UIFont fontHelp(_z, _fontHeight);

	if (_caretPassChar == 0) { // move the cursor to the start of the wstring
		_beginDrawXPos = 0;
	}
	else if (_caretPassChar == _text.size()) { // move the cursor to the end of the wstring
		SIZE contentSize = fontHelp.GetDrawAreaSize(_text.c_str());

		_beginDrawXPos = contentSize.cx < GetRectWidth()(_drawRectAllow) ? 0 : -(contentSize.cx - GetRectWidth()(_drawRectAllow));
	}
	else { // move the cursor to the current position
		// calculate the length of the wstring intercepted by the insertion cursor
		SIZE caretPassSize = fontHelp.GetDrawAreaSize(_text.substr(0, _caretPassChar).c_str());

		// calculate the position of the last character of the intercepted wstring
		int lastCharPos = caretPassSize.cx + _beginDrawXPos;

		// correct the current value, generally corresponding to the left and right keys of the keyboard
		if (lastCharPos < 0) { // less than _drawRectAllow   move the cursor to the start of _drawRectAllow
			_beginDrawXPos -= lastCharPos;
		}
		else if (lastCharPos > GetRectWidth()(_drawRectAllow)) { // greater than _drawRectAllow   move the cursor to the end of _drawRectAllow
			_beginDrawXPos -= lastCharPos - GetRectWidth()(_drawRectAllow);
		}
		// else  in the _drawRectAllow, _beginDrawXPos remains unchanged
	}
}

void UIEdit::SelectAllText() {
	if (_text.size() == 0) {
		return;
	}

	_isSelArea = 2;
	_beginAreaPassChar = 0;
	_caretPassChar = _text.size();
	_isDrawCaretImmd = true;

	CalcBeginDrawXPos();
}

void UIEdit::CalcCaretPassChar(POINT& point) {
	UIFont fontHelp(_z, _fontHeight);

	// calc the absolute position point
	POINT pt = point;
	pt.x -= _abusolutePoint.x;
	pt.y -= _abusolutePoint.y;

	// get the size information of the wstring
	int preSizeCX = 0;

	if (pt.x <= _drawRectAllow.left) { // the point is in front of the allowed drawing area
		_caretPassChar = 0;
		_beginDrawXPos = 0;
	}
	else if (pt.x >= _drawRectAllow.right) { // the point is behind the allowed drawing area
		_caretPassChar = _text.size();
		CalcBeginDrawXPos();
	}
	else { // the point is in the allowed drawing area
		preSizeCX = 0;
		UINT i;
		for (i = 0; i <= _text.size(); ++i) {
			// substring [0-i]
			SIZE contentSize = fontHelp.GetDrawAreaSize(_text.substr(0, i).c_str());

			// if the point is beyond the intercepted wstring, continue to calculate
			if (pt.x >= _drawRectAllow.left + _beginDrawXPos + contentSize.cx) {
				preSizeCX = contentSize.cx;
				continue;
			}

			// if the point is not beyond the intercepted wstring, calculate which one i-1 or i is closer
			_caretPassChar = pt.x <= (_drawRectAllow.left + _beginDrawXPos + preSizeCX + (contentSize.cx - preSizeCX) / 2) ? (i - 1) : i;
			break;
		}
		//
		if (i > _text.size()) {
			_caretPassChar = _text.size();
		}
	}
}

void UIEdit::OnDestroy() {
	if (_isFocus) {
		UIHideCaret();
	}
}

void UIEdit::OnSetFocus() {
	LONG heightLimit = GetRectHeight()(_clientRC) > 2 ? GetRectHeight()(_clientRC) - 2 : 2;
	LONG height = ((LONG)_fontHeight + 10) < heightLimit ? ((LONG)_fontHeight + 10) : heightLimit;

	UIShowCaret(2, height, UIColor::Blue);
}

void UIEdit::OnKillFocus() {
	UIHideCaret();
}

bool UIEdit::OnLButtonDown(POINT pt) {
	// calc the postion of caret
	CalcCaretPassChar(pt);
	_isDrawCaretImmd = true;

	_isSelArea = 0;

	UIRefresh();
	return true;
}

bool UIEdit::OnLButtonDbClk(POINT) {
	SelectAllText();

	UIRefresh();
	return true;
}

bool UIEdit::OnMouseMove(POINT pt) {
	//::SetCursor(::LoadCursor(NULL, IDC_IBEAM));

	if (IsKeyDown()(VK_LBUTTON) == true) {
		if (_isSelArea == 0) {
			_isSelArea = 1;
			_beginAreaPassChar = _caretPassChar;
		}
		else {
			CalcCaretPassChar(pt);
			_isSelArea = _beginAreaPassChar == _caretPassChar ? 1 : 2;
		}

		_isDrawCaretImmd = true;
		UIRefresh();
	}

	return true;
}

void UIEdit::OnKeyDown(TCHAR nChar) {
	if (nChar == VK_LEFT || nChar == VK_RIGHT || nChar == VK_HOME || nChar == VK_END) {
		if (IsKeyDown()(VK_SHIFT) == true) {
			if (_isSelArea == 0) {
				_isSelArea = 1;
				_beginAreaPassChar = _caretPassChar;
			}
		}
		else {
			_isSelArea = 0;
		}
	}

	switch (nChar) {
	case VK_LEFT: {
		if (_caretPassChar == 0) {
			return;
		}

		--_caretPassChar;
		CalcBeginDrawXPos();
		_isDrawCaretImmd = true;

		if (_isSelArea) {
			_isSelArea = _beginAreaPassChar == _caretPassChar ? 0 : 2;
		}
	} break;
	case VK_RIGHT: {
		if (_caretPassChar == _text.size()) {
			return;
		}

		++_caretPassChar;
		CalcBeginDrawXPos();
		_isDrawCaretImmd = true;

		if (_isSelArea) {
			_isSelArea = _beginAreaPassChar == _caretPassChar ? 0 : 2;
		}
	} break;
	case VK_HOME: {
		if (_caretPassChar == 0) {
			return;
		}

		_caretPassChar = 0;
		_beginDrawXPos = 0;
		_isDrawCaretImmd = true;

		if (_isSelArea) {
			_isSelArea = _beginAreaPassChar == _caretPassChar ? 0 : 2;
		}
	} break;
	case VK_END: {
		if (_caretPassChar == _text.size()) {
			return;
		}

		_caretPassChar = _text.size();
		CalcBeginDrawXPos();
		_isDrawCaretImmd = true;

		if (_isSelArea) {
			_isSelArea = _beginAreaPassChar == _caretPassChar ? 0 : 2;
		}
	} break;
	case VK_DELETE: {
		if (_isSelArea == 2) {
			EraseSelectArea();
		}
		else if (_caretPassChar < _text.size()) {
			_text.erase(_caretPassChar, 1);
		}
		else {
			return;
		}

		_isDrawCaretImmd = true;
	} break;
	case VK_BACK: {
		if (_isSelArea == 2) {
			EraseSelectArea();
		}
		else if (_caretPassChar >= 1) {
			_text.erase(--_caretPassChar, 1);
			CalcBeginDrawXPos();
		}
		else {
			return;
		}

		_isDrawCaretImmd = true;
	} break;
	case (TCHAR)VK_PROCESSKEY: { // input method input processing
		_isCNInput = true;
	} break;
	default: {
		_isCNInput = false;
	} break;
	};

	UIRefresh();
}

void UIEdit::OnChar(TCHAR nChar)
{
	switch (nChar) {
	case VK_BACK: {
	} break;
	case VK_TAB: {
	} break;
	default: {
		if (_isSelArea == 2) {
			EraseSelectArea();
		}

		// insert character
		_text.insert(_caretPassChar, 1, nChar);
		++_caretPassChar;

		// calculate the new drawing start character x coordinate
		CalcBeginDrawXPos();
		_isDrawCaretImmd = true;

	} break;
	};

	UIRefresh();
}

bool UIEdit::OnPaste() {
	wstring strBuf;
	StringPasteFromClipboard()(strBuf, UIFrame::GetSingletonInstance()->GetWindowHandle());

	// modify content
	if (_isSelArea == 2) {
		EraseSelectArea();
	}
	//
	_text.insert(_caretPassChar, strBuf);
	_caretPassChar += strBuf.size();

	CalcBeginDrawXPos();
	UIRefresh();
	return true;
}

bool UIEdit::OnCopy() {
	if (_isSelArea != 2) {
		return true;
	}

	StringCopyToClipboard()(wstring(_caretPassChar > _beginAreaPassChar ? &_text[_beginAreaPassChar] : &_text[_caretPassChar],
		abs((int)_caretPassChar - (int)_beginAreaPassChar)), UIFrame::GetSingletonInstance()->GetWindowHandle());
	return true;
}

bool UIEdit::OnCut() {
	OnCopy();
	if (_isSelArea == 2) {
		EraseSelectArea();
	}

	UIRefresh();
	return true;
}



UISelectList::UISelectList() {
	_selectedectIndex = -1;
	_hoverIndex = -1;
	_isHover = false;
}

void UISelectList::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	// calc the drawing area
	LONG dh = static_cast<LONG>(20 * _list.size());
	RECT parentRC = p_parentUIContainer->GetBindWindowAbusoluteRect();
	RECT abusoluteRC = parentRC.bottom + dh < UIDXFoundation::GetSingletonInstance()->GetOutputHeight() ?
		CreateRect()(parentRC.left, parentRC.bottom, parentRC.right, parentRC.bottom + dh) :
		CreateRect()(parentRC.left, parentRC.top - dh, parentRC.right, parentRC.top);
	RECT relativeRC = abusoluteRC;
	OffsetRect(&relativeRC, -parentRC.left, -parentRC.top);
	MoveWindow(relativeRC);

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw the border (background offset = 0, border offset = 1)
	{
		RECT rc = _clientRC;
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(UIColor(246, 250, 252), 220, transformMatrix);
		UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryBlueLight, transformMatrix);
	}

	// draw the hover (offset = 1)
	if (_hoverIndex != -1) {
		RECT rc = _clientRC;
		rc.top = _hoverIndex * 20;
		rc.bottom = (_hoverIndex + 1) * 20;
		++rc.left;
		++rc.top;
	--rc.right;
	--rc.bottom;
	OffsetRect(&rc, x_, y_);

	UISlicedImage(L"GUIResource.dll", IDB_HOT_EFFECT2, UIColor(255, 0, 255), 2, 2, 2, 2, _z, renderLevel)(rc, 255, transformMatrix);
}
	// draw the content (text offset = 5)
	for (UINT i = 0; i < _list.size(); ++i) {
		RECT rc = _clientRC;
		rc.left += 5;
		rc.top = i * 20;
		rc.bottom = rc.top + 20;
		OffsetRect(&rc, x_, y_);

		// when the animation is running
		if (IsAnimationRun()) {
			int width = GetRectWidth()(rc) + i * 10;
			int dx = width / _maxFrame;
			rc.left += dx * (_maxFrame - _frameIndex);
		}

		UIFont fontHelp(_z, gDefaultFontSize, renderLevel + 5);
		fontHelp(_list[i], rc, UIColor::Black, UIFontPos::MiddleLeft, transformMatrix);
	}
}

void UISelectList::AddText(UIString text) {
	_list.push_back(text);
}

optional<UIString> UISelectList::GetText(int index) {
	if (index < 0 || index >= (int)_list.size()) {
		return nullopt;
	}

	return _list[index];
}

size_t UISelectList::GetListCount() {
	return _list.size();
}

vector<wstring> UISelectList::GetTextList() {
	return _list;
}

void UISelectList::ClearList() {
	_list.clear();

	_selectedectIndex = -1;
	_hoverIndex = -1;
	_isHover = false;
}

bool UISelectList::OnMouseMove(POINT pt) {
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	int oldIndex = _hoverIndex;

	_hoverIndex = point.y / 20;
	if (_hoverIndex >= (int)_list.size()) {
		_hoverIndex = (int)_list.size() - 1;
	}

	if (oldIndex != _hoverIndex) {
		UIRefresh();
	}
	return true;
}

void UISelectList::OnMouseLeave(POINT) {
	_hoverIndex = -1;
}

bool UISelectList::OnLButtonDown(POINT pt) {
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	_selectedectIndex = point.y / 20;
	p_parentUIContainer->SendMessageToBindWindow(WM_NOTIFY, _id, _selectedectIndex);
	return true;
}


UIComboBox::UIComboBox() {
	_isDrawBoader = true;
	_isDropDown = false;
	_isHover = false;

	_selectedectIndex = -1;
}

void UIComboBox::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw the border (offset = 1)
	RECT rc = _clientRC;
	OffsetRect(&rc, x_, y_);
	if (_isDrawBoader) {
		UIRect(rc, _z, renderLevel + 1)(!_isHover ? UIColor::PrimaryBlueLight : UIColor::Gold, transformMatrix);
	}

	{	// draw the arrow (offset = 0 for images)
		RECT rc2 = rc;
		rc2.left = rc2.right - 34;
		rc2.right = rc2.left + 32;
		rc2.top += GetRectHeight()(rc2) / 2 - 16;
		rc2.bottom = rc2.top + 32;
		UIImage(L"GUIResource.dll", IDB_ARROW1_DOWN, UIColor::Invalid, _z, renderLevel)(rc2, 1.f, 255, transformMatrix);
	}

	// draw the content (text offset = 5)
	if (_selectedectIndex != -1) {
	auto optText = _dropDownList.GetText(_selectedectIndex);
	wstring str = optText.has_value() ? optText.value() : L"";

	rc.left += 5;
	UIFont fontHelp(_z, gDefaultFontSize, renderLevel + 5);
	fontHelp(str, rc, UIColor::Black, UIFontPos::MiddleLeft, transformMatrix);
}	if (_isDropDown) {
		_dropDownList.Draw();
	}
}

void UIComboBox::AddText(UIString text) {
	_dropDownList.AddText(text);
}

UIString UIComboBox::GetText() {
	auto optV = _dropDownList.GetText(_selectedectIndex);
	if (optV.has_value()) {
		return optV.value();
	}

	return L"";
}

void UIComboBox::SetSelectIndex(int index) {
	_selectedectIndex = index;
}

void UIComboBox::SetText(UIString text) {
	vector<wstring> textList = _dropDownList.GetTextList();

	auto it = find(textList.begin(), textList.end(), (wstring)text);
	if (it != textList.end()) {
		_selectedectIndex = static_cast<int>(distance(textList.begin(), it));
	}
	else {
		_selectedectIndex = -1; // not found
	}
}

void UIComboBox::ClearList() {
	_dropDownList.ClearList();
}

int UIComboBox::GetSelectIndex() {
	return _selectedectIndex;
}

vector<wstring> UIComboBox::GetTextList() {
	return _dropDownList.GetTextList();
}

void UIComboBox::IsDrawBoader(bool flag) {
	_isDrawBoader = flag;
}

void UIComboBox::SetDropDown(bool flag) {
	// if (_isDropDown==flag) {
	// 	return;
	// }

	size_t listCount = _dropDownList.GetListCount();
	if (listCount == 0) {
		return;
	}

	_isDropDown = flag;
	_dropDownList.ShowWindow(flag);

	if (flag) {
		UIFrame::GetSingletonInstance()->SetHookWindowForPopup(&_dropDownList);
		_dropDownList.PlayAnimate();
	}
}

void UIComboBox::OnCreate() {
	_dropDownList._id = 0;
	_dropDownList.CreateWindowBase(this, NULL_RECT, 0, false);
}

void UIComboBox::OnKillFocus() {
	_isHover = false;
	SetDropDown(false);
}

bool UIComboBox::OnMouseMove(POINT) {
	if (_isHover == false) {
		_isHover = true;
		UIRefresh();
	}

	return true;
}

void UIComboBox::OnMouseLeave(POINT) {
	_isHover = false;
	UIRefresh();
}

bool UIComboBox::OnLButtonDbClk(POINT pt) {
	return OnLButtonDown(pt);
}

bool UIComboBox::OnLButtonDown(POINT) {
	SetDropDown(true); //!_isDropDown

	return true;
}

void UIComboBox::OnNotify(int id, LPARAM param) {
	if (id == 0) {
		_selectedectIndex = (int)param;
		SetDropDown(false);
		UIFrame::GetSingletonInstance()->SetHookWindowForPopup(NULL);
		_isHover = false;

		if (!OnNotifyEvent()) {
			SendMessageToParent(WM_NOTIFY, _id, _selectedectIndex);
		}
	}

	UIRefresh();
}


UIScrollBar::UIScrollBar(int coordFlag) {
	_coordFlag = coordFlag;
	_pageScale = 0.2;
	_posScale = 0;
}

void UIScrollBar::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw border (background offset = 0)
	RECT rc = _clientRC;
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryBlueLight, 20, transformMatrix);

	// draw bar (offset = 1)
	rc = _barRect;
	if (GetRectWidth()(rc) < 8 || GetRectHeight()(rc) < 8) {
		if (_coordFlag == 0) {
			if (_posScale < 0.5) {
				rc.right = rc.left + 8;
			}
			else {
				rc.left = rc.right - 8;
			}
		}
		else {
			if (_posScale < 0.5) {
				rc.bottom = rc.top + 8;
			}
			else {
				rc.top = rc.bottom - 8;
			}
		}
	}
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurple, 180, transformMatrix);
}

void UIScrollBar::CalcArea() {
	_scrollRect = _clientRC;
	CalcBarArea();
}

void UIScrollBar::CalcBarArea() {
	if (_coordFlag == 0) {
		_barRect.top = _scrollRect.top;
		_barRect.bottom = _scrollRect.bottom;

		_barRect.left = _scrollRect.left + (int)(_posScale * GetRectWidth()(_scrollRect));
		_barRect.right = _barRect.left + (int)(_pageScale * GetRectWidth()(_scrollRect));
	}
	else {
		_barRect.left = _scrollRect.left;
		_barRect.right = _scrollRect.right;

		_barRect.top = _scrollRect.top + (int)(_posScale * GetRectHeight()(_scrollRect));
		_barRect.bottom = _barRect.top + (int)(_pageScale * GetRectHeight()(_scrollRect));
	}
}

void UIScrollBar::SetPageScale(double s) {
	_pageScale = s;
	CalcBarArea();
}

double UIScrollBar::GetPageScale() {
	return _pageScale;
}

void UIScrollBar::SetPosScale(double s) {
	_posScale = s;
	if (_posScale > 1 - _pageScale) {
		_posScale = 1 - _pageScale;
	}
	CalcBarArea();
}

double UIScrollBar::GetPosScale() {
	return _posScale < (1 - _pageScale) ? _posScale : (1 - _pageScale);
}

void UIScrollBar::SetCoordXY(int coordFlag) {
	_coordFlag = coordFlag;
	CalcArea();
}

bool UIScrollBar::OnMouseMove(POINT pt) {
	// judge the mouse is out of the control range
	if (IsKeyDown()(VK_LBUTTON)) {
		POINT point = pt;
		point.x -= _abusolutePoint.x;
		point.y -= _abusolutePoint.y;

		if (!IsWindowFocus()) {
			return true;
		}

		if (_coordFlag == 0) {
			int dx = point.x - _prePoint.x;
			OffsetRect(&_barRect, dx, 0);
			if (_barRect.left < _scrollRect.left) {
				OffsetRect(&_barRect, _scrollRect.left - _barRect.left, 0);
			}
			else if (_barRect.right > _scrollRect.right) {
				OffsetRect(&_barRect, _scrollRect.right - _barRect.right, 0);
			}

			_posScale = (double)(_barRect.left - _scrollRect.left) / GetRectWidth()(_scrollRect);
			SendMessageToParent(WM_HSCROLL, SB_THUMBTRACK, (LPARAM)this);
		}
		else {
			int dy = point.y - _prePoint.y;
			OffsetRect(&_barRect, 0, dy);
			if (_barRect.top < _scrollRect.top) {
				OffsetRect(&_barRect, 0, _scrollRect.top - _barRect.top);
			}
			else if (_barRect.bottom > _scrollRect.bottom) {
				OffsetRect(&_barRect, 0, _scrollRect.bottom - _barRect.bottom);
			}

			_posScale = (double)(_barRect.top - _scrollRect.top) / GetRectHeight()(_scrollRect);
			SendMessageToParent(WM_VSCROLL, SB_THUMBTRACK, (LPARAM)this);
		}

		_prePoint = point;
		UIRefresh();
	}

	return true;
}

bool UIScrollBar::OnLButtonDown(POINT point) {
	POINT pt = point;
	pt.x -= _abusolutePoint.x;
	pt.y -= _abusolutePoint.y;
	_prePoint = pt;

	if (ContainsPoint()(pt, _barRect) == false) {
		if (_coordFlag == 0) {
			if (pt.x < _barRect.left) {
				_posScale -= _pageScale;
				_posScale = (double)(pt.x - GetRectWidth()(_barRect) / 2) / GetRectWidth()(_scrollRect);
				if (_posScale < 0) {
					_posScale = 0;
				}
				SendMessageToParent(WM_HSCROLL, SB_PAGELEFT, (LPARAM)this);
			}
			else if (pt.x > _barRect.right) {
				_posScale = (double)(pt.x - GetRectWidth()(_barRect) / 2) / GetRectWidth()(_scrollRect);
				if (_posScale > 1 - _pageScale) {
					_posScale = 1 - _pageScale;
				}
				SendMessageToParent(WM_HSCROLL, SB_PAGERIGHT, (LPARAM)this);
			}
		}
		else {
			if (pt.y < _barRect.top) {
				_posScale = (double)(pt.y - GetRectHeight()(_barRect) / 2) / GetRectHeight()(_scrollRect);
				if (_posScale < 0) {
					_posScale = 0;
				}
				SendMessageToParent(WM_VSCROLL, SB_PAGEUP, (LPARAM)this);
			}
			else if (pt.y > _barRect.bottom) {
				_posScale = (double)(pt.y - GetRectHeight()(_barRect) / 2) / GetRectHeight()(_scrollRect);
				if (_posScale > 1 - _pageScale) {
					_posScale = 1 - _pageScale;
				}
				SendMessageToParent(WM_VSCROLL, SB_PAGEDOWN, (LPARAM)this);
			}
		}
		CalcBarArea();
	}
	else {
		UIFrame::GetSingletonInstance()->SetHookWindowForScrollBar(this);
	}

	UIRefresh();
	return true;
}

bool UIScrollBar::OnLButtonDbClk(POINT pt) {
	return OnLButtonDown(pt);
}





UIGrid::GridCellInfo::GridCellInfo(wstring str) {
	_text = str;
	//_wordPos = UIFontPos::MiddleLeft;

	// Default is edit control
	_pCtrl = NULL;
	_controlType = CTRL_EDIT;
	_isHold = false;

	_color = UIColor::Black;
}

void UIGrid::GridCellInfo::CreateCellControl(UIContainer* pUIContainer, int firstRowPos, int firstColumnPos, int id) {
	if (_pCtrl != NULL) {
		return;
	}

	if (_controlType == CTRL_EDIT) {
		UIEdit* pControl = new UIEdit;
		_pCtrl = pControl;

		pControl->CreateControl(8000, pUIContainer, CreateRect()(_pos.left + firstColumnPos, _pos.top + firstRowPos, _pos.right + 1 + firstColumnPos, _pos.bottom + 1 + firstRowPos), 0, true);
		pControl->SetWindowFocus();
		pControl->SetText(_text);
		pControl->SelectAllText();
	}
	else if (_controlType == CTRL_CHECKBUTTON) {
		UICheckButton* pControl = new UICheckButton;
		_pCtrl = pControl;

		pControl->CreateControl(id, pUIContainer, CreateRect()(_pos.left + firstColumnPos, _pos.top + firstRowPos, _pos.right + 1 + firstColumnPos, _pos.bottom + 1 + firstRowPos), 0, true);
		pControl->SetText(_text);
		_isHold = true;
	}
	else if (_controlType == CTRL_COMBOBOX) {
		UIComboBox* pControl = new UIComboBox;
		_pCtrl = pControl;

		pControl->CreateControl(id, pUIContainer, CreateRect()(_pos.left + firstColumnPos, _pos.top + firstRowPos, _pos.right + 1 + firstColumnPos, _pos.bottom + 1 + firstRowPos), 0, true);
		pControl->IsDrawBoader(false);
		_isHold = true;
	}
	else if (_controlType == CTRL_BUTTON) {
		UIButton* pControl = new UIButton;
		_pCtrl = pControl;

		pControl->CreateControl(id, pUIContainer, CreateRect()(_pos.left + firstColumnPos, _pos.top + firstRowPos, _pos.right + 1 + firstColumnPos, _pos.bottom + 1 + firstRowPos), 0, true);
		pControl->SetText(_text);
		_isHold = true;
	}
}

bool UIGrid::GridCellInfo::DeleteCellControl() {
	// Check if the control exists
	if (_pCtrl == NULL) {
		return true;
	}
	if (_isHold == true) {
		return true;
	}

	if (_controlType == CTRL_EDIT) {
		// Get the control pointer
		UIEdit* pControl = (UIEdit*)_pCtrl;

		// Get the control string value and set it to the cell
		wstring str = pControl->GetText();
		_text = str;

		// Destroy the edit control
		pControl->DestroyWindowBase();

	}
	else if (_controlType == CTRL_CHECKBUTTON) {
		// Get the control pointer
		UICheckButton* pControl = (UICheckButton*)_pCtrl;
		pControl->DestroyWindowBase();
	}
	else if (_controlType == CTRL_COMBOBOX) {
		// Get the control pointer
		UIComboBox* pControl = (UIComboBox*)_pCtrl;
		pControl->DestroyWindowBase();
	}
	else if (_controlType == CTRL_BUTTON) {
		// Get the control pointer
		UIButton* pControl = (UIButton*)_pCtrl;
		pControl->DestroyWindowBase();
	}

	delete _pCtrl; // delete the control pointer
	_pCtrl = NULL;
	return true;
}

void UIGrid::GridCellInfo::MoveCellControl(const RECT& rc) {
	if (_pCtrl == NULL) {
		return;
	}

	_pCtrl->MoveWindow(rc);
}

void UIGrid::GridCellInfo::DrawCellControl() {
	if (_pCtrl == NULL) {
		return;
	}

	_pCtrl->ShowWindow(true);
	_pCtrl->Draw();
}

UIGrid::UIGrid() : _xScroll(0), _yScroll(1) {
	_fontHeight = gDefaultFontSize;

	_firstRowPos = 0;
	_firstColumnPos = 0;

	_rowNum = 0;
	_columnNum = 0;

	_heightSum = 0;
	_widthSum = 0;

	_isFirstRowFix = false;
	_isFirstColumnFix = false;

	_isXScrollShow = false;
	_isYScrollShow = false;
	_xScrollBarArea = NULL_RECT;
	_yScrollBarArea = NULL_RECT;

	_selectedInfo = SELECTION_NONE;

	_isDraw = true;
}

void UIGrid::Draw() {
	if (!_isDraw) {
		return;
	}
	if (_rowNum == 0 || _columnNum == 0) {
		return;
	}

	_inheritedTransformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	// Draw the border (offset = 1)
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	RECT rc = _clientRC;
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryBlue, _inheritedTransformMatrix);

	// Draw the scroll bar
	DrawScrollBar();

	// Calculate the starting row and column to draw
	CalcDrawBeginRowColumn(_beginDrawRow, _beginDrawColumn);

	// Draw the selected content (background offset = 0, selection offset = 1)
	DrawSelected();

	// Draw the grid dashed line (offset = 1)
	DrawGrid();

	// Draw the cell content (text offset = 5)
	DrawCells();
}

void UIGrid::DrawScrollBar() {
	if (_isXScrollShow) {
		_xScroll.MoveWindow(_xScrollBarArea);
		_xScroll.Draw();
	}

	if (_isYScrollShow) {
		_yScroll.MoveWindow(_yScrollBarArea);
		_yScroll.Draw();
	}
}

void UIGrid::DrawSelected() {
	int renderLevel = GetRenderLayout();
	UIColor fixColor = UIColor(102, 204, 255);
	UIColor selectColor = UIColor(202, 228, 255);

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	RECT rc;

	// Draw the fix area normal background (offset = 0 for backgrounds)
	if (_isFirstRowFix && _isFirstColumnFix) {
		rc = CreateRect()(_gridArea.left, _gridArea.top, _gridArea.right, _gridArea.top + _rowHeightList[0]);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(fixColor, 240, _inheritedTransformMatrix);

		int rcTop = _gridArea.top + _rowHeightList[0] < _gridArea.bottom ? (_gridArea.top + _rowHeightList[0]) : _gridArea.bottom;

		rc = CreateRect()(_gridArea.left, rcTop, _gridArea.left + _columnWidthList[0], _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(fixColor, 240, _inheritedTransformMatrix);
	}
	else if (_isFirstRowFix) {
		rc = CreateRect()(_gridArea.left, _gridArea.top, _gridArea.right, _gridArea.top + _rowHeightList[0]);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(fixColor, 240, _inheritedTransformMatrix);
	}
	else if (_isFirstColumnFix) {
		rc = CreateRect()(_gridArea.left, _gridArea.top, _gridArea.left + _columnWidthList[0], _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(fixColor, 240, _inheritedTransformMatrix);
	}

	// Draw the selected identifier
	if (_selectedInfo == SELECTION_ALL) {
		DrawSelectedALL(selectColor);
	}
	else if (_selectedInfo == SELECTION_CELL) {
		DrawSelectedCELL();
	}
	else if (_selectedInfo == SELECTION_CELLS) {
		DrawSelectedCELLS(selectColor);
	}
	else if (_selectedInfo == SELECTION_ROW) {
		DrawSelectedROW(selectColor);
	}
	else if (_selectedInfo == SELECTION_COLUMN) {
		DrawSelectedCOLUMN(selectColor);
	}
}

void UIGrid::DrawSelectedALL(UIColor& selectColor) {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// Draw the unfix area (offset = 0 for selection backgrounds)
	RECT rc = CreateRect()(_unfixGridArea.left, _unfixGridArea.top, _unfixGridArea.right, _unfixGridArea.bottom);
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(selectColor, 180, _inheritedTransformMatrix);
}

void UIGrid::DrawSelectedCELL() {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	RECT rc;

	GridCellInfo* pSelCell = &_cellArray[_selectedRowBegin][_selectedColumnBegin];
	RECT& pos = pSelCell->_pos;

	{	// Draw the unfix area (offset = 1 for selection highlight)
		rc = _unfixGridArea;
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		// Draw the selected cell
		if (pSelCell->_pCtrl == NULL) {
			rc = CreateRect()(pos.left + _firstColumnPos, pos.top + _firstRowPos, pos.right + _firstColumnPos, pos.bottom + _firstRowPos);
			OffsetRect(&rc, x_, y_);
			UIRect(rc, _z - gDeltaZ, renderLevel + 1)(UIColor::PrimaryPurple, _inheritedTransformMatrix); // z-0.01f should > 0.0f
		}
	}

	// Draw the fix area cell row and column information (offset = 1)
	// Draw the selected row information
	if (_isFirstColumnFix) {
		rc = CreateRect()(_gridArea.left, _unfixGridArea.top, _gridArea.right, _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		rc = CreateRect()(_cellArray[0][0]._pos.left, pos.top + _firstRowPos, _cellArray[0][0]._pos.right, pos.bottom + _firstRowPos);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurple, 250, _inheritedTransformMatrix);
	}
	// Draw the selected column information
	if (_isFirstRowFix) {
		rc = CreateRect()(_unfixGridArea.left, _gridArea.top, _gridArea.right, _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		rc = CreateRect()(pos.left + _firstColumnPos, _cellArray[0][0]._pos.top, pos.right + _firstColumnPos, _cellArray[0][0]._pos.bottom);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurple, 250, _inheritedTransformMatrix);
	}
}

void UIGrid::DrawSelectedCELLS(UIColor& selectColor) {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	RECT rc;

	UINT beginRow, endRow, beginColumn, endColumn;
	CalcCellsRange(beginRow, endRow, beginColumn, endColumn);

	GridCellInfo* pCellBegin = &_cellArray[beginRow][beginColumn];
	GridCellInfo* pCellEnd = &_cellArray[endRow][endColumn];

	RECT& posBegin = pCellBegin->_pos;
	RECT& posEnd = pCellEnd->_pos;

	{	// Draw the unfix area (offset = 0 for selection background)
		rc = _unfixGridArea;
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		// Draw the selected cell
		rc = CreateRect()(posBegin.left + _firstColumnPos + 1, posBegin.top + _firstRowPos + 1, posEnd.right + _firstColumnPos, posEnd.bottom + _firstRowPos);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(selectColor, 180, _inheritedTransformMatrix);
	}

	// Draw the fix area cell row and column information (offset = 1)
	// Draw the selected row information
	if (_isFirstColumnFix) {
		rc = CreateRect()(_gridArea.left, _unfixGridArea.top, _gridArea.right, _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		rc = CreateRect()(_cellArray[0][0]._pos.left, posBegin.top + _firstRowPos, _cellArray[0][0]._pos.right, posEnd.bottom + _firstRowPos);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurple, 250, _inheritedTransformMatrix);
	}
	// Draw the selected column information
	if (_isFirstRowFix) {
		rc = CreateRect()(_unfixGridArea.left, _gridArea.top, _gridArea.right, _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		rc = CreateRect()(posBegin.left + _firstColumnPos, _cellArray[0][0]._pos.top, posEnd.right + _firstColumnPos, _cellArray[0][0]._pos.bottom);
		OffsetRect(&rc, x_, y_);
		UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurple, 250, _inheritedTransformMatrix);
	}
}

void UIGrid::DrawSelectedROW(UIColor& selectColor) {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	RECT rc;

	{	// Draw the unfix area (offset = 0 for selection background)
		rc = _unfixGridArea;
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		for (auto i = _selectedRowList.begin(); i != _selectedRowList.end(); ++i) {
			RECT& pos = _cellArray[*i][0]._pos;
			rc = CreateRect()(pos.right, pos.top + _firstRowPos, _gridArea.right, pos.bottom + _firstRowPos);
			OffsetRect(&rc, x_, y_);
			UIRect(rc, _z, renderLevel + 1)(selectColor, 180, _inheritedTransformMatrix);
		}
	}

	{	// Draw the fix area (offset = 1)
		rc = CreateRect()(_gridArea.left, _unfixGridArea.top + 1, _gridArea.right, _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		for (auto i = _selectedRowList.begin(); i != _selectedRowList.end(); ++i) {
			RECT& pos = _cellArray[*i][0]._pos;
			rc = CreateRect()(pos.left, pos.top + _firstRowPos, pos.right, pos.bottom + _firstRowPos);
			OffsetRect(&rc, x_, y_);
			UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurple, 250, _inheritedTransformMatrix);
		}
	}
}

void UIGrid::DrawSelectedCOLUMN(UIColor& selectColor) {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	RECT rc;

	{	// Draw the unfix area (offset = 0 for selection background)
		rc = _unfixGridArea;
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		for (auto i = _selectedColumnList.begin(); i != _selectedColumnList.end(); ++i) {
			RECT& pos = _cellArray[0][*i]._pos;
			rc = CreateRect()(pos.left + _firstColumnPos, pos.bottom, pos.right + _firstColumnPos, _gridArea.bottom);
			OffsetRect(&rc, x_, y_);
			UIRect(rc, _z, renderLevel + 1)(selectColor, 180, _inheritedTransformMatrix);
		}
	}

	{	// Draw the fix area (offset = 1)
		rc = CreateRect()(_unfixGridArea.left + 1, _gridArea.top, _gridArea.right, _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		//UIScreenClipRectGuard uiClip(rc);

		for (auto i = _selectedColumnList.begin(); i != _selectedColumnList.end(); ++i) {
			RECT& pos = _cellArray[0][*i]._pos;
			rc = CreateRect()(pos.left + _firstColumnPos, pos.top, pos.right + _firstColumnPos, pos.bottom);
			OffsetRect(&rc, x_, y_);
			UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurple, 250, _inheritedTransformMatrix);
		}
	}
}

void UIGrid::DrawGrid() {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// Draw the horizontal line (offset = 1 for grid lines)
	int top = _isFirstRowFix ? _rowHeightList[0] : 0;
	for (UINT i = _beginDrawRow; i < _rowNum; ++i) {
		int yPos = _cellArray[i][0]._pos.bottom + _firstRowPos;

	if (yPos < top) {
		continue;
	}
	else if (yPos > _gridArea.bottom + 1) {
		break;
	}

	UILine(_gridArea.left + x_, yPos + y_, _gridArea.right + x_, yPos + y_, _z, 1.0f, renderLevel + 1)(UIColor::PrimaryGrayLight, _inheritedTransformMatrix);
}
if (_beginDrawRow != 0 && _isFirstRowFix) {
	int yPos = _cellArray[0][0]._pos.bottom;
	UILine(_gridArea.left + x_, yPos + y_, _gridArea.right + x_, yPos + y_, _z, 1.0f, renderLevel + 1)(UIColor::PrimaryGrayLight, _inheritedTransformMatrix);
}
	// Draw the vertical line (offset = 1 for grid lines)
	int left = _isFirstColumnFix ? _columnWidthList[0] : 0;
	for (UINT i = _beginDrawColumn; i < _columnNum; ++i) {
		int xPos = _cellArray[0][i]._pos.right + _firstColumnPos;

		if (xPos < left) {
			continue;
		}
		else if (xPos > _gridArea.right + 1) {
			break;
		}

		UILine(xPos + x_, _gridArea.top + y_, xPos + x_, _gridArea.bottom + y_, _z, 1.0f, renderLevel + 1)(UIColor::PrimaryGrayLight, _inheritedTransformMatrix);
	}
	if (_beginDrawColumn != 0 && _isFirstColumnFix) {
		int xPos = _cellArray[0][0]._pos.right;
		UILine(xPos + x_, _gridArea.top + y_, xPos + x_, _gridArea.bottom + y_, _z, 1.0f, renderLevel + 1)(UIColor::PrimaryGrayLight, _inheritedTransformMatrix);
	}
}

void UIGrid::DrawCells() {
	int renderLevel = GetRenderLayout();
	// Calculate the relative gridUnfix area
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;
	RECT rc;

	// Calculate the start row and column
	UINT endRow = _rowNum;
	UINT endColumn = _columnNum;

	// Draw the _unfixGridArea area content
	if (_isFirstRowFix == true) {
		_beginDrawRow = _beginDrawRow > 1 ? _beginDrawRow : 1;
	}
	if (_isFirstColumnFix == true) {
		_beginDrawColumn = _beginDrawColumn > 1 ? _beginDrawColumn : 1;
	}

	// Create font helper with text offset = 5
	UIFont fontHelp(_z - gDeltaZ, _fontHeight, renderLevel + 5);

	int topLimit = (_unfixGridArea.bottom - _firstRowPos) < _heightSum ? (_unfixGridArea.bottom - _firstRowPos) : _heightSum;
	int rightLimit = (_unfixGridArea.right - _firstColumnPos) < _widthSum ? (_unfixGridArea.right - _firstColumnPos) : _widthSum;
	//
	for (UINT row = _beginDrawRow; row < _rowNum; ++row) {
		if (_cellArray[row][0]._pos.top > topLimit) {
			endRow = row;
			break;
		}
		for (UINT column = _beginDrawColumn; column < _columnNum; ++column) {
			if (_cellArray[0][column]._pos.left > rightLimit) {
				endColumn = column;
				break;
			}

			GridCellInfo& cell = _cellArray[row][column];
			//
			rc = _unfixGridArea;
			OffsetRect(&rc, x_, y_);
			UIScreenClipRectGuard uiClip(rc);
			//
			if (cell._pCtrl == NULL) {
				rc = CreateRect()(cell._pos.left + _firstColumnPos + 5, cell._pos.top + _firstRowPos, cell._pos.right + _firstColumnPos - 5, cell._pos.bottom + _firstRowPos);
				OffsetRect(&rc, x_, y_);
				rc.left += 2;
				fontHelp(cell._text, rc, cell._color, UIFontPos::MiddleLeft, _inheritedTransformMatrix);
			}
			else if (cell._pCtrl != NULL) {
				rc = CreateRect()(cell._pos.left + _firstColumnPos, cell._pos.top + _firstRowPos, cell._pos.right + _firstColumnPos, cell._pos.bottom + _firstRowPos);
				cell.MoveCellControl(rc);
				cell.DrawCellControl();
			}
		}
	}

	// Draw the fix row (text offset = 5, already in fontHelp)
	if (_isFirstRowFix) {
		rc = CreateRect()(_unfixGridArea.left, _gridArea.top, _gridArea.right, _unfixGridArea.top);
		OffsetRect(&rc, x_, y_);
		UIScreenClipRectGuard uiClip(rc);

		for (UINT column = _beginDrawColumn; column < endColumn; ++column) {
			GridCellInfo& cell = _cellArray[0][column];
			if (cell._pCtrl == NULL) {
				rc = CreateRect()(cell._pos.left + _firstColumnPos, _gridArea.top, cell._pos.right + _firstColumnPos, _gridArea.top + _rowHeightList[0]);
				OffsetRect(&rc, x_, y_);
				fontHelp(cell._text, rc, cell._color, UIFontPos::MiddleCenter, _inheritedTransformMatrix);
			}
			else if (cell._pCtrl != NULL) {
				rc = CreateRect()(cell._pos.left + _firstColumnPos, _gridArea.top, cell._pos.right + _firstColumnPos, _gridArea.top + _rowHeightList[0]);
				cell.MoveCellControl(rc);
				cell.DrawCellControl();
			}
		}
	}

	// Draw the fix column (text offset = 5, already in fontHelp)
	if (_isFirstColumnFix) {
		rc = CreateRect()(_gridArea.left, _unfixGridArea.top, _unfixGridArea.left, _gridArea.bottom);
		OffsetRect(&rc, x_, y_);
		UIScreenClipRectGuard uiClip(rc);

		for (UINT row = _beginDrawRow; row < endRow; ++row) {
			GridCellInfo& cell = _cellArray[row][0];
			if (cell._pCtrl == NULL) {
				rc = CreateRect()(_gridArea.left, cell._pos.top + _firstRowPos, _gridArea.left + _columnWidthList[0], cell._pos.bottom + _firstRowPos);
				OffsetRect(&rc, x_, y_);
				fontHelp(cell._text, rc, cell._color, UIFontPos::MiddleCenter, _inheritedTransformMatrix);
			}
			else if (cell._pCtrl != NULL) {
				rc = CreateRect()(_gridArea.left, cell._pos.top + _firstRowPos, _gridArea.left + _columnWidthList[0], cell._pos.bottom + _firstRowPos);
				cell.MoveCellControl(rc);
				cell.DrawCellControl();
			}
		}
	}

	// draw the first cell in the fix area
	{
		rc = CreateRect()(_gridArea.left - 1, _gridArea.top - 1, _gridArea.right + 1, _gridArea.bottom + 1);
		OffsetRect(&rc, x_, y_);
		UIScreenClipRectGuard uiClip(rc);
		// 
		if (_isFirstRowFix && _isFirstColumnFix) {
			GridCellInfo& cell = _cellArray[0][0];
			if (cell._pCtrl == NULL) {
				rc = CreateRect()(_gridArea.left, _gridArea.top, _gridArea.left + _columnWidthList[0], _gridArea.top + _rowHeightList[0]);
				OffsetRect(&rc, x_, y_);
				fontHelp(cell._text, rc, cell._color, UIFontPos::MiddleCenter, _inheritedTransformMatrix);
			}
			else if (cell._pCtrl != NULL) {
				rc = CreateRect()(cell._pos.left, cell._pos.top, cell._pos.right, cell._pos.bottom);
				cell.MoveCellControl(rc);
				cell.DrawCellControl();
			}
		}
	}
}

void UIGrid::CalcArea() {
	if (GetRectWidth()(_clientRC) == 0 || GetRectHeight()(_clientRC) == 0) {
		_isDraw = false;
		//return;
	}
	else {
		_isDraw = true;
	}

	CalcNoScrollRect();
	CalcGridRect();
	CalcScrollBarRect();
}

void UIGrid::CalcNoScrollRect() {
	_noScrollArea = _clientRC;
	// remove the border
	_noScrollArea.left += 1;
	_noScrollArea.right -= 1;
	_noScrollArea.top += 1;
	_noScrollArea.bottom -= 1;

	// first calculation
	bool xFlag = true;
	bool yFlag = true;
	if (_widthSum > GetRectWidth()(_clientRC) - 2) {
		xFlag = false;
		_noScrollArea.bottom -= 15;
	}
	if (_heightSum > GetRectHeight()(_clientRC) - 2) {
		yFlag = false;
		_noScrollArea.right -= 15;
	}

	// second calculation to prevent the generated scroll bar from affecting the table area
	if (_widthSum > GetRectWidth()(_noScrollArea) && xFlag) {
		_noScrollArea.bottom -= 15;
	}
	if (_heightSum > GetRectHeight()(_noScrollArea) && yFlag) {
		_noScrollArea.right -= 15;
	}
}

void UIGrid::CalcGridRect() {
	// calculate the grid area
	_gridArea.left = _noScrollArea.left;
	_gridArea.top = _noScrollArea.top;
	//
	_gridArea.right = _gridArea.left + _widthSum + _firstColumnPos;
	_gridArea.bottom = _gridArea.top + _heightSum + _firstRowPos;
	//
	if (_gridArea.right > _noScrollArea.right) {
		_gridArea.right = _noScrollArea.right;
	}
	if (_gridArea.bottom > _noScrollArea.bottom) {
		_gridArea.bottom = _noScrollArea.bottom;
	}
	//
	if (_gridArea.right < _noScrollArea.right && _firstColumnPos < 0) {
		_firstColumnPos += _noScrollArea.right - _gridArea.right;
		_gridArea.right = _noScrollArea.right;
	}
	if (_gridArea.bottom < _noScrollArea.bottom && _firstRowPos < 0) {
		_firstRowPos += _noScrollArea.bottom - _gridArea.bottom;
		_gridArea.bottom = _noScrollArea.bottom;
	}

	// calculate the _unfixGridArea area
	_unfixGridArea = _gridArea;
	if (_isFirstRowFix && _rowHeightList.size() > 0) {
		_unfixGridArea.top += _rowHeightList[0];
	}
	if (_isFirstColumnFix && _columnWidthList.size() > 0) {
		_unfixGridArea.left += _columnWidthList[0];
	}
}

void UIGrid::CalcScrollBarRect() {
	if (_widthSum <= GetRectWidth()(_noScrollArea)) {
		_isXScrollShow = false;
		_firstColumnPos = 0;
		_xScroll.SetPosScale(0);
	}
	else {
		// calculate the scroll bar information
		_isXScrollShow = true;
		_xScroll.SetPosScale((double)(-_firstColumnPos) / _widthSum);
		_xScroll.SetPageScale((double)GetRectWidth()(_gridArea) / _widthSum);

		// calculate the scroll bar position
		_xScrollBarArea.left = _noScrollArea.left;
		_xScrollBarArea.right = _noScrollArea.right;
		_xScrollBarArea.top = _noScrollArea.bottom;
		_xScrollBarArea.bottom = _clientRC.bottom - 1;
	}

	if (_heightSum <= GetRectHeight()(_noScrollArea)) {
		_isYScrollShow = false;
		_firstRowPos = 0;
		_yScroll.SetPosScale(0);
	}
	else {
		// calculate the scroll bar information
		_isYScrollShow = true;
		_yScroll.SetPosScale((double)(-_firstRowPos) / _heightSum);
		_yScroll.SetPageScale((double)GetRectHeight()(_gridArea) / _heightSum);

		// calculate the scroll bar position
		_yScrollBarArea.left = _noScrollArea.right;
		_yScrollBarArea.right = _clientRC.right - 1;
		_yScrollBarArea.top = _noScrollArea.top;
		_yScrollBarArea.bottom = _noScrollArea.bottom;
	}
}

// calculate the starting row and column based on the scroll bar position information
void UIGrid::CalcDrawBeginRowColumn(UINT& beginRow, UINT& beginColumn) {
	beginRow = 0;
	beginColumn = 0;

	if (_isYScrollShow) {
		long _top = _noScrollArea.top - _firstRowPos;
		// 
		beginRow = (UINT)(_rowNum * _yScroll.GetPosScale());
		if (beginRow == _rowNum) {
			beginRow -= 1;
		}
		else {
			while (_cellArray[beginRow][0]._pos.bottom < _top && beginRow < _rowNum) {
				beginRow++;
			}
			while (_cellArray[beginRow][0]._pos.top > _top && beginRow > 0) {
				--beginRow;
			}
		}
	}

	if (_isXScrollShow) {
		long _left = _noScrollArea.left - _firstColumnPos;
		beginColumn = (UINT)(_columnNum * _xScroll.GetPosScale());
		if (beginColumn == _columnNum) {
			beginColumn -= 1;
		}
		else {
			while (_cellArray[0][beginColumn]._pos.right < _left && beginColumn < _columnNum) {
				beginColumn++;
			}
			while (_cellArray[0][beginColumn]._pos.left > _left && beginColumn > 0) {
				--beginColumn;
			}
		}
	}

	for (UINT r = 0; r < _rowNum; ++r) {
		for (UINT c = 0; c < _columnNum; ++c) {
			GridCellInfo& cell = _cellArray[r][c];
			if (cell._pCtrl != NULL) {
				cell._pCtrl->ShowWindow(false);
			}
		}
	}
}

void UIGrid::CalcCellsPos() {
	if (_rowNum == 0 || _columnNum == 0) {
		return;
	}

	// initialize the first cell
	{
		GridCellInfo& cell = _cellArray[0][0];
		cell._pos.left = _gridArea.left;
		cell._pos.top = _gridArea.top;
		cell._pos.right = cell._pos.left + _columnWidthList[0];
		cell._pos.bottom = cell._pos.top + _rowHeightList[0];
	}
	// initialize the first row cell
	for (UINT colum = 1; colum < _columnNum; ++colum) {
		GridCellInfo& cell = _cellArray[0][colum];
		cell._pos.left = _cellArray[0][colum - 1]._pos.right;
		cell._pos.top = _gridArea.top;
		cell._pos.right = cell._pos.left + _columnWidthList[colum];
		cell._pos.bottom = cell._pos.top + _rowHeightList[0];
	}
	// initialize the first column cell
	for (UINT row = 1; row < _rowNum; ++row) {
		GridCellInfo& cell = _cellArray[row][0];
		cell._pos.left = _gridArea.left;
		cell._pos.top = _cellArray[row - 1][0]._pos.bottom;
		cell._pos.right = cell._pos.left + _columnWidthList[0];
		cell._pos.bottom = cell._pos.top + _rowHeightList[row];
	}
	// initialize the remaining cell
	for (UINT row = 1; row < _rowNum; ++row) {
		for (UINT colum = 1; colum < _columnNum; ++colum) {
			GridCellInfo& cell = _cellArray[row][colum];
			cell._pos.left = _cellArray[row][colum - 1]._pos.right;
			cell._pos.top = _cellArray[row - 1][colum]._pos.bottom;
			cell._pos.right = cell._pos.left + _columnWidthList[colum];
			cell._pos.bottom = cell._pos.top + _rowHeightList[row];
		}
	}
}

bool UIGrid::CalcCellIndexUnfix(UINT& row, UINT& column, POINT point) {
	if (_unfixGridArea.top == _unfixGridArea.bottom || _unfixGridArea.left == _unfixGridArea.right) {
		return false;
	}

	int xCoord = point.x - _firstColumnPos;
	int yCoord = point.y - _firstRowPos;

	// judge the row
	row = _beginDrawRow;
	while (_cellArray[row][0]._pos.bottom < yCoord) {
		++row;
	}

	// judge the column
	column = _beginDrawColumn;
	while (_cellArray[0][column]._pos.right < xCoord) {
		++column;
	}

	return true;
}

void UIGrid::CalcCellsRange(UINT& beginRow, UINT& endRow, UINT& beginColumn, UINT& endColumn) {
	beginRow = _selectedRowBegin < _selectedRowEnd ? _selectedRowBegin : _selectedRowEnd;
	endRow = _selectedRowBegin > _selectedRowEnd ? _selectedRowBegin : _selectedRowEnd;
	beginColumn = _selectedColumnBegin < _selectedColumnEnd ? _selectedColumnBegin : _selectedColumnEnd;
	endColumn = _selectedColumnBegin > _selectedColumnEnd ? _selectedColumnBegin : _selectedColumnEnd;
}

void UIGrid::SetNoSel() {
	// release the last activated cell
	if (_selectedInfo == SELECTION_CELL) {
		GridCellInfo& activeCell = _cellArray[_selectedRowBegin][_selectedColumnBegin];
		activeCell.DeleteCellControl();
	}

	_selectedInfo = SELECTION_NONE;
}

void UIGrid::SetRowColumn(UINT rowNum, UINT columnNum) {
	// the row and column number are the same as before, do not process
	if (_rowNum == rowNum && _columnNum == columnNum) {
		return;
	}

	// set SELECTION_NONE and clear the possible generated controls
	SetNoSel();
	//
	UINT r = _isFirstRowFix ? 1 : 0;
	for (; r < _rowNum; ++r) {
		for (UINT c = 0; c < _columnNum; ++c) {
			GridCellInfo& cell = _cellArray[r][c];
			cell._isHold = false;
			cell.DeleteCellControl();
		}
	}

	// set the new row and column number
	_rowNum = rowNum;
	_columnNum = columnNum;

	// 
	_firstRowPos = 0;
	_firstColumnPos = 0;
	_xScroll.SetPosScale(0);
	_yScroll.SetPosScale(0);

	// initialize the row height and column width	
	if (_rowHeightList.size() < _rowNum) {
		int dx = _rowNum - (int)_rowHeightList.size();
		while (dx-- > 0) {
			_rowHeightList.push_back(20);
		}
	}
	else if (_rowHeightList.size() > _rowNum) {
		_rowHeightList.erase(_rowHeightList.begin() + _rowNum, _rowHeightList.end());
	}
	// 
	if (_columnWidthList.size() < _columnNum) {
		int dx = _columnNum - (int)_columnWidthList.size();
		while (dx-- > 0) {
			_columnWidthList.push_back(60);
		}
	}
	else if (_columnWidthList.size() > _columnNum) {
		_columnWidthList.erase(_columnWidthList.begin() + _columnNum, _columnWidthList.end());
	}

	// calculate the sum of row height and column width
	_heightSum = accumulate(_rowHeightList.begin(), _rowHeightList.end(), 0);
	_widthSum = accumulate(_columnWidthList.begin(), _columnWidthList.end(), 0);
	// 
	CalcArea();

	// initialize the cell matrix
	_cellArray.clear();
	for (UINT row = 0; row < _rowNum; ++row) {
		_cellArray.push_back(vector<GridCellInfo>());
		for (UINT colum = 0; colum < _columnNum; ++colum) {
			_cellArray[row].push_back(GridCellInfo(L""));
		}
	}
	// 
	CalcCellsPos();
}

void UIGrid::GetRowColumn(UINT& rowNum, UINT& columnNum) {
	rowNum = _rowNum;
	columnNum = _columnNum;
}

void UIGrid::SetRowHeight(UINT row, int height) {
	if (row >= _rowHeightList.size()) {
		return;
	}

	_heightSum += height - _rowHeightList[row];
	_rowHeightList[row] = height;

	CalcGridRect();
	CalcScrollBarRect();
	CalcCellsPos();
}

void UIGrid::SetColumnWidth(UINT column, int width) {
	if (column >= _columnWidthList.size()) {
		return;
	}

	_widthSum += width - _columnWidthList[column];
	_columnWidthList[column] = width;

	CalcGridRect();
	CalcScrollBarRect();
	CalcCellsPos();
}

void UIGrid::SetRowFix() {
	if (_rowNum == 0) {
		return;
	}

	_isFirstRowFix = true;
	CalcGridRect();
}

void UIGrid::SetColumnFix() {
	if (_columnNum == 0) {
		return;
	}

	_isFirstColumnFix = true;
	CalcGridRect();
}

void UIGrid::SetCellText(UINT row, UINT column, UIString text, bool isAutoWidth) {
	if (row >= _rowNum || column >= _columnNum) {
		return;
	}

	if (_cellArray[row][column]._pCtrl == NULL) {
		_cellArray[row][column]._text = text;
	}
	else {
		if (_cellArray[row][column]._controlType == GridCellInfo::CTRL_EDIT) {
			UIEdit& edit = *(UIEdit*)(_cellArray[row][column]._pCtrl);
			edit.SetText(text);
		}
		else if (_cellArray[row][column]._controlType == GridCellInfo::CTRL_CHECKBUTTON) {
			UICheckButton& but = *(UICheckButton*)(_cellArray[row][column]._pCtrl);
			but.SetText(text);
		}
	}

	// automatically calculate the width
	if (isAutoWidth) {
		UIFont fontHelp(_z, gDefaultFontSize);
		SIZE sz = fontHelp.GetDrawAreaSize(text);
		if (sz.cx > _columnWidthList[column]) {
			SetColumnWidth(column, sz.cx);
		}
	}
}

optional<UIString> UIGrid::GetCellText(UINT row, UINT column) {
	if (row >= _rowNum || column >= _columnNum) {
		return nullopt;
	}

	if (_cellArray[row][column]._pCtrl == NULL) {
		return _cellArray[row][column]._text;
	}
	else {
		if (_cellArray[row][column]._controlType == GridCellInfo::CTRL_EDIT) {
			UIEdit& edit = *(UIEdit*)(_cellArray[row][column]._pCtrl);
			return edit.GetText();
		}
		else if (_cellArray[row][column]._controlType == GridCellInfo::CTRL_CHECKBUTTON) {
			UICheckButton& but = *(UICheckButton*)(_cellArray[row][column]._pCtrl);
			return but.GetText();
		}
		else if (_cellArray[row][column]._controlType == GridCellInfo::CTRL_COMBOBOX) {
			UIComboBox& comboBox = *(UIComboBox*)(_cellArray[row][column]._pCtrl);
			return comboBox.GetText();
		}
	}
	return nullopt;
}

optional<int> UIGrid::GetCellTextToInt(UINT row, UINT column) {
	auto textOpt = GetCellText(row, column);
	if (!textOpt.has_value()) {
		return nullopt;
	}

	wstring text = (wstring)textOpt.value();
	return text.empty() ? 0 : stoi(text);
}

optional<float> UIGrid::GetCellTextToFloat(UINT row, UINT column) {
	auto textOpt = GetCellText(row, column);
	if (!textOpt.has_value()) {
		return nullopt;
	}

	wstring text = textOpt.value();
	return text.empty() ? 0.0f : stof(text);
}

optional<double> UIGrid::GetCellTextToDouble(UINT row, UINT column) {
	auto textOpt = GetCellText(row, column);
	if (!textOpt.has_value()) {
		return nullopt;
	}

	wstring text = textOpt.value();
	return text.empty() ? 0.0 : stod(text);
}

void UIGrid::SetCellColor(UINT row, UINT column, const UIColor& color) {
	if (row >= _rowNum || column >= _columnNum) {
		return;
	}

	_cellArray[row][column]._color = color;
}

void UIGrid::SetCellFontHeight(float h) {
	_fontHeight = h;
}

void UIGrid::AddRow(vector<UIString>& row) {
	vector<vector<UIString>> rowList;
	rowList.push_back(row);
	AddRows(rowList);
}

void UIGrid::AddRows(vector<vector<UIString>>& rowList) {
	for (UINT i = 0; i < rowList.size(); ++i) {
		vector<UIString>& rowCache = rowList[i];

		//	update the row height list
		_rowHeightList.push_back(20);
		_heightSum += 20;
		//	increase the corresponding row cell
		_cellArray.push_back(vector<GridCellInfo>());
		for (UINT colum = 0; colum < _columnNum; ++colum) {
			_cellArray[_rowNum].push_back(GridCellInfo(rowCache[colum]));
		}
		//	calculate the position of each cell in the row
		for (UINT colum = 0; colum < _columnNum; ++colum) {
			GridCellInfo& cell = _cellArray[_rowNum][colum];
			cell._pos.left = _cellArray[_rowNum - 1][colum]._pos.left;
			cell._pos.top = _cellArray[_rowNum - 1][colum]._pos.bottom;
			cell._pos.right = cell._pos.left + _columnWidthList[colum];
			cell._pos.bottom = cell._pos.top + _rowHeightList[_rowNum];
		}
		//	finally, adjust the row number
		++_rowNum;
	}

	CalcArea();
}

void UIGrid::ClearUnfixRows() {
	//	set SELECTION_NONE and clear the possible generated controls
	SetNoSel();

	//	delete the hold controls in each unfix row
	UINT r = _isFirstRowFix ? 1 : 0;
	for (; r < _rowNum; ++r) {
		for (UINT c = 0; c < _columnNum; ++c) {
			GridCellInfo& cell = _cellArray[r][c];
			cell._isHold = false;
			cell.DeleteCellControl();
		}
	}

	//	no fix row
	if (!_isFirstRowFix) {
		_rowNum = 0; //	adjust the row number first to prevent accessing non-existent memory during simultaneous drawing
		_cellArray.clear();
		_rowHeightList.clear();
		_columnWidthList.clear();
		_heightSum = 0;
		_widthSum = 0;
	}
	else { //	exist fix row
		if (_rowNum == 1) return;

		_rowNum = 1; //	adjust the row number first to prevent accessing non-existent memory during simultaneous drawing
		_cellArray.erase(++_cellArray.begin(), _cellArray.end());
		_rowHeightList.erase(++_rowHeightList.begin(), _rowHeightList.end());
		_heightSum = _rowHeightList[0];
	}

	CalcArea();
}

void UIGrid::ClearAllCells() {
	SetNoSel();

	//	delete the hold controls in each unfix row
	for (UINT r = _isFirstRowFix ? 1 : 0; r < _rowNum; ++r) {
		for (UINT c = _isFirstColumnFix ? 1 : 0; c < _columnNum; ++c) {
			GridCellInfo& cell = _cellArray[r][c];
			cell._isHold = false;
			cell.DeleteCellControl();
			cell._text = L"";
		}
	}
}

void UIGrid::SetAreaCells(UINT row, UINT column, vector<vector<UIString>>& rowList) {
	//	missing judgment check
	for (UINT i = 0; i < rowList.size(); ++i) {
		vector<UIString>& rowCache = rowList[i];

		for (UINT j = 0; j < rowCache.size(); ++j) {
			if (_cellArray[row + i][column + j]._pCtrl == NULL) {
				_cellArray[row + i][column + j]._text = rowCache[j];
			}
			else { //	if the cell control exists, set the control string
				if (_cellArray[row][column]._controlType == GridCellInfo::CTRL_EDIT) {
					UIEdit& edit = *(UIEdit*)(_cellArray[row + i][column + j]._pCtrl);
					edit.SetText(rowCache[j]);
				}
				else if (_cellArray[row][column]._controlType == GridCellInfo::CTRL_CHECKBUTTON) {
					UICheckButton& but = *(UICheckButton*)(_cellArray[row + i][column + j]._pCtrl);
					but.SetText(rowCache[j]);
				}
			}
		}
	}
}

void UIGrid::SetCellCheckButton(int id, UINT row, UINT column, UIString str) {
	if (row >= _rowNum || column >= _columnNum) {
		return;
	}

	GridCellInfo& cell = _cellArray[row][column];
	if (cell._pCtrl != NULL) {
		cell.DeleteCellControl();
	}

	cell._text = str;
	cell._controlType = GridCellInfo::CTRL_CHECKBUTTON;
	cell.CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos, id);
}

bool UIGrid::GetCellCheckState(UINT row, UINT column, bool& checkState) {
	if (row >= _rowNum || column >= _columnNum) {
		return false;
	}

	GridCellInfo& cell = _cellArray[row][column];

	if (cell._pCtrl == NULL || cell._controlType != GridCellInfo::CTRL_CHECKBUTTON) {
		return false;
	}

	checkState = static_cast<UICheckButton*>(cell._pCtrl)->GetCheck();
	return true;
}

void UIGrid::SetCellCheckState(UINT row, UINT column, bool checkState) {
	if (row >= _rowNum || column >= _columnNum) {
		return;
	}

	GridCellInfo& cell = _cellArray[row][column];

	if (cell._pCtrl != NULL && cell._controlType == GridCellInfo::CTRL_CHECKBUTTON) {
		static_cast<UICheckButton*>(cell._pCtrl)->SetCheck(checkState);
	}
}

void UIGrid::SetCellComboBox(int id, UINT row, UINT column) {
	if (row >= _rowNum || column >= _columnNum) {
		return;
	}

	GridCellInfo& cell = _cellArray[row][column];
	if (cell._pCtrl != NULL) {
		cell._isHold = false;
		cell.DeleteCellControl();
	}

	cell._controlType = GridCellInfo::CTRL_COMBOBOX;
	cell.CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos, id);
}

void UIGrid::AddCellComboBoxText(UINT row, UINT column, UIString text) {
	if (row >= _rowNum || column >= _columnNum) {
		return;
	}

	GridCellInfo& cell = _cellArray[row][column];
	if (cell._pCtrl != NULL && cell._controlType == GridCellInfo::CTRL_COMBOBOX) {
		UIComboBox* pControl = (UIComboBox*)cell._pCtrl;
		pControl->AddText(text);
	}
}

//UIComboBox* UIGrid::GetCellComboBox(UINT row, UINT column)
//{
//	if (row>_rowNum || column>_columnNum)
//		return 0;
//
//	GridCellInfo& cell = _cellArray[row][column];
//
//	if (cell._pCtrl == NULL || cell._controlType!=GridCellInfo::CTRL_COMBOBOX)
//		return 0;
//
//	return static_cast<UIComboBox*>(cell._pCtrl);
//}

void UIGrid::SetCellButton(int id, UINT row, UINT column, UIString str) {
	if (row >= _rowNum || column >= _columnNum) {
		return;
	}

	GridCellInfo& cell = _cellArray[row][column];
	cell._text = str;
	if (cell._pCtrl != NULL) {
		cell._isHold = false;
		cell.DeleteCellControl();
	}

	cell._controlType = GridCellInfo::CTRL_BUTTON;
	cell.CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos, id);
}

bool UIGrid::GetSelectCell(UINT& row, UINT& column) {
	if (_selectedInfo != SELECTION_CELL) {
		return false;
	}

	row = _selectedRowBegin;
	column = _selectedColumnBegin;
	return true;
}

bool UIGrid::GetSelectCells(UINT& beginRow, UINT& beginColumn, UINT& endRow, UINT& endColumn) {
	if (_selectedInfo != SELECTION_CELLS) {
		return false;
	}

	beginRow = _selectedRowBegin;
	beginColumn = _selectedColumnBegin;
	endRow = _selectedRowEnd;
	endColumn = _selectedColumnEnd;
	return true;
}

bool UIGrid::GetSelectRows(vector<UINT>& selectList) {
	if (_selectedInfo != SELECTION_ROW) {
		return false;
	}

	selectList = _selectedRowList;
	sort(selectList.begin(), selectList.end());

	return true;
}

bool UIGrid::GetSelectColumns(vector<UINT>& selectList) {
	if (_selectedInfo != SELECTION_COLUMN) {
		return false;
	}

	selectList = _selectedColumnList;
	sort(selectList.begin(), selectList.end());

	return true;
}

void UIGrid::OnCreate() {
	_xScroll.CreateControl(0, &_uiContainer, _xScrollBarArea);
	_yScroll.CreateControl(0, &_uiContainer, _yScrollBarArea);

	CalcArea();
}

bool UIGrid::OnLButtonDown(POINT pt) {
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	//	not in grid area
	if (ContainsPoint()(point, _gridArea) == false) {
		return true;
	}

	//	unfix area processing
	if (ContainsPoint()(point, _unfixGridArea)) {
		if (OnLButtonDownUnfix(point) == false) {
			return true;
		}
	}
	else { //	fix area processing
		OnLButtonDownFix(point);
	}

	//	single row selected, send selected message to parent window
	if (_selectedInfo == SELECTION_ROW && _selectedRowList.size() == 1) {
		//	notify the parent window to select the new single row
		_nmGrid._code = 2;
		_nmGrid._row = _selectedRowList[0];
		_nmGrid._column = 0;
		SendMessageToParent(WM_NOTIFY, _id, (LPARAM)&_nmGrid);
	}
	else if (_selectedInfo == SELECTION_COLUMN && _selectedColumnList.size() == 1) {
		//	notify the parent window to select the new single column
		_nmGrid._code = 3;
		_nmGrid._row = 0;
		_nmGrid._column = _selectedColumnList[0];
		SendMessageToParent(WM_NOTIFY, _id, (LPARAM)&_nmGrid);
	}
	else if (_selectedInfo == SELECTION_CELL) { //	cell selected
		_nmGrid._code = 4;
		_nmGrid._row = _selectedRowBegin;
		_nmGrid._column = _selectedColumnBegin;
		SendMessageToParent(WM_NOTIFY, _id, (LPARAM)&_nmGrid);
	}

	UIRefresh();
	return true;
}

bool UIGrid::OnLButtonDownUnfix(POINT point) {
	//	judge row and column
	UINT row, column;
	if (!CalcCellIndexUnfix(row, column, point)) {
		return false;
	}

	if (_selectedInfo == SELECTION_CELL) {
		if (_selectedRowBegin == row && _selectedColumnBegin == column) { //	same, activate the cell control
			_cellArray[_selectedRowBegin][_selectedColumnBegin].CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos);
		}
		else {
			_cellArray[_selectedRowBegin][_selectedColumnBegin].DeleteCellControl();
			_selectedRowBegin = row;
			_selectedColumnBegin = column;
		}
	}
	else {
		_selectedInfo = SELECTION_CELL;
		_selectedRowBegin = row;
		_selectedColumnBegin = column;
	}

	return true;
}

void UIGrid::OnLButtonDownFix(POINT point) {
	//	_pos convert to coordinate value
	int xCoord = point.x - _firstColumnPos;
	int yCoord = point.y - _firstRowPos;

	//	first release the possible activated cell
	if (_selectedInfo == SELECTION_CELL) {
		_cellArray[_selectedRowBegin][_selectedColumnBegin].DeleteCellControl();
	}

	GridCellInfo& cell = _cellArray[0][0];
	if (_isFirstRowFix && _isFirstColumnFix && cell._pos.right >= point.x && cell._pos.bottom >= point.y) {
		_selectedInfo = SELECTION_ALL;
	}
	else if ((_isFirstRowFix && _isFirstColumnFix && cell._pos.right < point.x) ||
		(_isFirstRowFix && !_isFirstColumnFix && cell._pos.left < point.x)) {
		if (_selectedInfo != SELECTION_COLUMN) {
			_selectedColumnList.clear();
			_selectedInfo = SELECTION_COLUMN;
		}

		//	judge column
		UINT column = _beginDrawColumn;
		while (_cellArray[0][column]._pos.right < xCoord) ++column;

		if (IsKeyDown()(VK_CONTROL)) {
			_selectedColumnList.push_back(column);
		}
		else if (IsKeyDown()(VK_SHIFT)) {
			if (_selectedColumnList.size() == 0) {
				_selectedColumnList.push_back(column);
			}
			else {
				UINT preColumn = _selectedColumnList.back();
				while (preColumn < column) {
					_selectedColumnList.push_back(++preColumn);
				}
				while (preColumn > column) {
					_selectedColumnList.push_back(--preColumn);
				}
				//	remove duplicate elements
				sort(_selectedColumnList.begin(), _selectedColumnList.end());
				_selectedColumnList.erase(unique(_selectedColumnList.begin(), _selectedColumnList.end()), _selectedColumnList.end());
			}
		}
		else {
			_selectedColumnList.clear();
			_selectedColumnList.push_back(column);
		}
	}
	else if ((_isFirstRowFix && _isFirstColumnFix && cell._pos.bottom < point.y) ||
		(!_isFirstRowFix && _isFirstColumnFix && cell._pos.top < point.y)) {
		if (_selectedInfo != SELECTION_ROW) {
			_selectedRowList.clear();
			_selectedInfo = SELECTION_ROW;
		}

		//	judge row
		UINT row = _beginDrawRow;
		while (_cellArray[row][0]._pos.bottom < yCoord) {
			++row;
		}

		if (IsKeyDown()(VK_CONTROL)) {
			auto it = find(_selectedRowList.begin(), _selectedRowList.end(), row);
			if (it == _selectedRowList.end()) {
				_selectedRowList.push_back(row);
			}
			else {
				_selectedRowList.erase(it);
			}
		}
		else if (IsKeyDown()(VK_SHIFT)) {
			if (_selectedRowList.size() == 0) {
				_selectedRowList.push_back(row);
			}
			else {
				UINT preRow = _selectedRowList.back();
				while (preRow < row) {
					_selectedRowList.push_back(++preRow);
				}
				while (preRow > row) {
					_selectedRowList.push_back(--preRow);
				}
				//	remove duplicate elements
				sort(_selectedRowList.begin(), _selectedRowList.end());
				_selectedRowList.erase(unique(_selectedRowList.begin(), _selectedRowList.end()), _selectedRowList.end());
			}
		}
		else {
			_selectedRowList.clear();
			_selectedRowList.push_back(row);
		}
	}
}

bool UIGrid::OnLButtonDbClk(POINT pt) {
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	//	check if the active cell exists
	if (_selectedInfo != SELECTION_CELL) {
		return true;
	}

	if (ContainsPoint()(point, _unfixGridArea)) {
		_cellArray[_selectedRowBegin][_selectedColumnBegin].CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos);
		UIRefresh();
		return true;
	}

	return OnLButtonDown(pt);
}

void UIGrid::OnHscroll(int code, UIScrollBar* pScrollBar) {
	double posScale = pScrollBar->GetPosScale();
	double pageScale = pScrollBar->GetPageScale();

	switch (code) {
	case SB_LINELEFT: {  // left arrow
		_firstColumnPos += 60;
		if (_firstColumnPos > 0) {
			_firstColumnPos = 0;
		}
		double scale = -(double)_firstColumnPos / _widthSum;
		pScrollBar->SetPosScale(scale);
	} break;
	case SB_LINERIGHT: {  // right arrow
		_firstColumnPos -= 60;
		if ((-_firstColumnPos) > _widthSum * (1 - pageScale)) {
			_firstColumnPos = (int)(-_widthSum * (1 - pageScale));
		}
		double scale = -(double)_firstColumnPos / _widthSum;
		pScrollBar->SetPosScale(scale);
	} break;
	case SB_THUMBTRACK: {  // mouse drag
		_firstColumnPos = -(int)(posScale * _widthSum);
	} break;
	case SB_PAGERIGHT: {  // right page
		_firstColumnPos = -(int)(posScale * _widthSum);
	} break;
	case SB_PAGELEFT: {  // left page
		_firstColumnPos = -(int)(posScale * _widthSum);
	} break;
	}

	// 
	UIRefresh();
}

void UIGrid::OnVscroll(int code, UIScrollBar* pScrollBar) {
	double posScale = pScrollBar->GetPosScale();
	double pageScale = pScrollBar->GetPageScale();

	switch (code) {
	case SB_LINEUP: {
		_firstRowPos += 20;
		if (_firstRowPos > 0) {
			_firstRowPos = 0;
		}
		double scale = -(double)_firstRowPos / _heightSum;
		pScrollBar->SetPosScale(scale);
	} break;
	case SB_LINEDOWN: {
		_firstRowPos -= 20;
		if ((-_firstRowPos) > _heightSum * (1 - pageScale)) {
			_firstRowPos = (int)(-_heightSum * (1 - pageScale));
		}
		double scale = -(double)_firstRowPos / _heightSum;
		pScrollBar->SetPosScale(scale);
	} break;
	case SB_THUMBTRACK: {
		_firstRowPos = -(int)(posScale * _heightSum);
	} break;
	case SB_PAGEUP: {
		_firstRowPos = -(int)(posScale * _heightSum);
	} break;
	case SB_PAGEDOWN: {
		_firstRowPos = -(int)(posScale * _heightSum);
	} break;
	}

	// 
	UIRefresh();
}

bool UIGrid::OnMouseWheel(short zDelta) {
	if (_isYScrollShow == false) {
		return true;
	}

	if (zDelta == 120) {
		OnVscroll(SB_LINEUP, &_yScroll);
	}
	else if (zDelta == -120) {
		OnVscroll(SB_LINEDOWN, &_yScroll);
	}

	UIRefresh();
	return true;
}

bool UIGrid::OnMouseMove(POINT pt) {
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	if (ContainsPoint()(point, _gridArea)) {
		if (IsKeyDown()(VK_LBUTTON)) {
			if (_selectedInfo == SELECTION_CELL || _selectedInfo == SELECTION_CELLS) {
				if (ContainsPoint()(point, _unfixGridArea)) {
					if (CalcCellIndexUnfix(_selectedRowEnd, _selectedColumnEnd, point)) {
						if (_selectedRowBegin == _selectedRowEnd && _selectedColumnBegin == _selectedColumnEnd) {
							_selectedInfo = SELECTION_CELL;
						}
						else {
							SetNoSel();
							_selectedInfo = SELECTION_CELLS;
							UIRefresh();
						}
					}
				}
			}
		}
	}

	return true;
}

void UIGrid::OnKeyDown(TCHAR nChar) {
	if (nChar == VK_DOWN || nChar == VK_UP || nChar == VK_LEFT || nChar == VK_RIGHT) {
		OnKeyDownArrows(nChar);
	}
	else if (nChar == (TCHAR)VK_PROCESSKEY) { //	IME processing
		OnKeyDownProcessKey();
	}
	else if (nChar == VK_BACK) { //	backspace
		OnKeyDownBack();
	}
	else if (nChar == VK_DELETE) { //	delete
		OnKeyDownDelete();
	}
	else if (nChar == VK_RETURN) { //	enter
		OnKeyDownReturn();
	}

	UIRefresh();
}

void UIGrid::OnKeyDownArrows(TCHAR nChar) {
	GridCellInfo* pSelCell = NULL;
	bool calcXScroll = false;
	bool calcYScroll = false;

	if (_selectedInfo == SELECTION_CELL) {
		pSelCell = &_cellArray[_selectedRowBegin][_selectedColumnBegin];

		// delete the active edit control
		pSelCell->DeleteCellControl();

		bool sendFlag = false;

		if (nChar == VK_UP) {
			if ((_isFirstRowFix && (_selectedRowBegin > 1)) || ((!_isFirstRowFix) && (_selectedRowBegin > 0))) {
				--_selectedRowBegin;
				sendFlag = true;
			}
		}
		else if (nChar == VK_DOWN) {
			if (_selectedRowBegin < _rowNum - 1) {
				++_selectedRowBegin;
				sendFlag = true;
			}
		}
		else if (nChar == VK_LEFT) {
			if ((_isFirstColumnFix && (_selectedColumnBegin > 1)) || ((!_isFirstColumnFix) && (_selectedColumnBegin > 0))) {
				--_selectedColumnBegin;
				sendFlag = true;
			}
		}
		else if (nChar == VK_RIGHT) {
			if (_selectedColumnBegin < _columnNum - 1) {
				++_selectedColumnBegin;
				sendFlag = true;
			}
		}

		calcXScroll = true;
		calcYScroll = true;
		pSelCell = &_cellArray[_selectedRowBegin][_selectedColumnBegin];

		if (sendFlag) {
			_nmGrid._code = 4;
			_nmGrid._row = _selectedRowBegin;
			_nmGrid._column = _selectedColumnBegin;
			SendMessageToParent(WM_NOTIFY, _id, (LPARAM)&_nmGrid);
		}

	}
	else if (_selectedInfo == SELECTION_ROW) {
		if (nChar == VK_LEFT || nChar == VK_RIGHT) {
			return;
		}

		if (_selectedRowList.size() != 1) {
			return;
		}

		if (nChar == VK_UP) {
			if ((_isFirstRowFix && _selectedRowList[0] == 1) || (_selectedRowList[0] == 0)) {
				return;
			}
			--_selectedRowList[0];
		}
		else if (nChar == VK_DOWN) {
			if (_selectedRowList[0] == _rowNum - 1) {
				return;
			}
			++_selectedRowList[0];
		}

		calcYScroll = true;
		pSelCell = &_cellArray[_selectedRowList[0]][0];

		// notify parent window to select new row
		_nmGrid._code = 2;
		_nmGrid._row = _selectedRowList[0];
		_nmGrid._column = 0;
		SendMessageToParent(WM_NOTIFY, _id, (LPARAM)&_nmGrid);
	}
	else if (_selectedInfo == SELECTION_COLUMN) {
		if (nChar == VK_UP || nChar == VK_DOWN) {
			return;
		}

		if (_selectedColumnList.size() != 1) {
			return;
		}

		if (nChar == VK_LEFT) {
			if ((_isFirstColumnFix && _selectedColumnList[0] == 1) || (_selectedColumnList[0] == 0)) {
				return;
			}
			--_selectedColumnList[0];
		}
		else if (nChar == VK_RIGHT) {
			if (_selectedColumnList[0] == _columnNum - 1) {
				return;
			}
			++_selectedColumnList[0];
		}

		calcXScroll = true;
		pSelCell = &_cellArray[0][_selectedColumnList[0]];

		// notify parent window to select new column
		_nmGrid._code = 3;
		_nmGrid._row = 0;
		_nmGrid._column = _selectedColumnList[0];
		SendMessageToParent(WM_NOTIFY, _id, (LPARAM)&_nmGrid);
	}

	if (calcXScroll) {
		// left and right arrow
		int dx = pSelCell->_pos.left + _firstColumnPos;
		if (dx < _unfixGridArea.left) {
			_firstColumnPos -= dx - _unfixGridArea.left;
		}
		dx = pSelCell->_pos.right + _firstColumnPos;
		if (dx > _unfixGridArea.right) {
			_firstColumnPos -= dx - _unfixGridArea.right;
		}
		// set x scroll bar
		_xScroll.SetPosScale(static_cast<double>(-_firstColumnPos) / _widthSum);
	}
	if (calcYScroll) {
		// up and down arrow
		int dy = pSelCell->_pos.top + _firstRowPos;
		if (dy < _unfixGridArea.top) {
			_firstRowPos -= dy - _unfixGridArea.top;
		}
		dy = pSelCell->_pos.bottom + _firstRowPos;
		if (dy > _unfixGridArea.bottom) {
			_firstRowPos -= dy - _unfixGridArea.bottom;
		}
		// set y scroll bar
		_yScroll.SetPosScale(static_cast<double>(-_firstRowPos) / _heightSum);
	}
}

void UIGrid::OnKeyDownProcessKey() {
	if (_selectedInfo == SELECTION_CELL) {
		_cellArray[_selectedRowBegin][_selectedColumnBegin].CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos);
	}
}

void UIGrid::OnKeyDownBack() {
	if (_selectedInfo == SELECTION_CELL) {
		GridCellInfo* pSelCell = &_cellArray[_selectedRowBegin][_selectedColumnBegin];
		pSelCell->_text = L"";
		pSelCell->CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos);
	}
}

void UIGrid::OnKeyDownReturn() {
	if (_selectedInfo == SELECTION_CELL) {
		// delete the possible edit control
		_cellArray[_selectedRowBegin][_selectedColumnBegin].DeleteCellControl();

		//OnKeyDown(VK_DOWN);

		_nmGrid._code = 1;
		_nmGrid._row = _selectedRowBegin;
		_nmGrid._column = _selectedColumnBegin;
		SendMessageToParent(WM_NOTIFY, _id, (LPARAM)&_nmGrid);

		// point to the next cell
		if (_selectedRowBegin < _rowNum - 1) {
			++_selectedRowBegin;
		}
	}
}

void UIGrid::OnKeyDownDelete() {
	if (_selectedInfo == SELECTION_CELL) {
		_cellArray[_selectedRowBegin][_selectedColumnBegin]._text = L"";
	}
	else if (_selectedInfo == SELECTION_CELLS) {
		UINT beginRow, endRow, beginColumn, endColumn;
		CalcCellsRange(beginRow, endRow, beginColumn, endColumn);

		for (UINT r = beginRow; r <= endRow; ++r) {
			for (UINT c = beginColumn; c <= endColumn; ++c) {
				_cellArray[r][c]._text = L"";
			}
		}
	}
	else if (_selectedInfo == SELECTION_ROW) {
		for (UINT r = 0; r < _selectedRowList.size(); ++r) {
			for (UINT c = 1; c < _columnNum; ++c) {
				_cellArray[_selectedRowList[r]][c]._text = L"";
			}
		}
	}
	else if (_selectedInfo == SELECTION_COLUMN) {
		for (UINT r = 1; r < _rowNum; ++r) {
			for (UINT c = 0; c < _selectedColumnList.size(); ++c) {
				_cellArray[r][_selectedColumnList[c]]._text = L"";
			}
		}
	}
	else if (_selectedInfo == SELECTION_ALL) {
		for (UINT r = 1; r < _rowNum; ++r) {
			for (UINT c = 1; c < _columnNum; ++c) {
				_cellArray[r][c]._text = L"";
			}
		}
	}
}

void UIGrid::OnChar(TCHAR nChar) {
	if (nChar == VK_RETURN) {
		return;
	}

	if (_selectedInfo != SELECTION_CELL) {
		return;
	}
	GridCellInfo* pSelCell = &_cellArray[_selectedRowBegin][_selectedColumnBegin];

	if (pSelCell->_controlType == GridCellInfo::CTRL_EDIT) {
		// create edit control
		pSelCell->CreateCellControl(&_uiContainer, _firstRowPos, _firstColumnPos);
		UIEdit* pEdit = (UIEdit*)(pSelCell->_pCtrl);

		// send the trigger character to the edit control
		UIPostMessage(pEdit, WM_CHAR, nChar, 0);
	}

	UIRefresh();
}

bool UIGrid::OnCopy() {
	bool copyFlag = false;
	wstring str;

	if (_selectedInfo == SELECTION_CELL) {
		copyFlag = true;

		str = _cellArray[_selectedRowBegin][_selectedColumnBegin]._text;
		str += L"\r\n";
	}
	else if (_selectedInfo == SELECTION_CELLS) {
		copyFlag = true;

		UINT beginRow, endRow, beginColumn, endColumn;
		CalcCellsRange(beginRow, endRow, beginColumn, endColumn);

		for (UINT r = beginRow; r <= endRow; ++r) {
			for (UINT c = beginColumn; c <= endColumn; ++c) {
				str += _cellArray[r][c]._text;
				if (c != endColumn) {
					str += L"	";
				}
			}

			str += L"\r\n";
		}
	}
	else if (_selectedInfo == SELECTION_ROW && _selectedRowList.size() != 0) {
		copyFlag = true;

		for (UINT r = 0; r < _selectedRowList.size(); ++r) {
			for (UINT c = 1; c < _columnNum; ++c) {
				str += _cellArray[_selectedRowList[r]][c]._text;
				if (c != _columnNum - 1) {
					str += L"	";
				}
			}

			str += L"\r\n";
		}
	}
	else if (_selectedInfo == SELECTION_COLUMN && _selectedColumnList.size() != 0) {
		copyFlag = true;

		for (UINT r = 1; r < _rowNum; ++r) {
			for (UINT c = 0; c < _selectedColumnList.size(); ++c) {
				str += _cellArray[r][_selectedColumnList[c]]._text;
				if (c != _selectedColumnList.size() - 1) {
					str += L"	";
				}
			}

			str += L"\r\n";
		}
	}
	else if (_selectedInfo == SELECTION_ALL) {
		copyFlag = true;

		for (UINT r = 1; r < _rowNum; ++r) {
			for (UINT c = 1; c < _columnNum; ++c) {
				str += _cellArray[r][c]._text;
				if (c != _columnNum - 1) {
					str += L"	";
				}
			}

			str += L"\r\n";
		}
	}

	if (copyFlag) {
		StringCopyToClipboard()(str, UIFrame::GetSingletonInstance()->GetWindowHandle());
	}

	return true;
}

bool UIGrid::OnPaste() {
	// Get the string from the clipboard
	wstring line;
	StringPasteFromClipboard()(line, UIFrame::GetSingletonInstance()->GetWindowHandle());

	// parse the string
	vector<vector<wstring>>allLineWordsList;
	StringHelper::SplitWStringToLineWords(line, allLineWordsList, L"	");

	// calculate the starting position of pasting
	UINT beginRow = 0, beginColumn = 0;
	bool pasteFlag = false;
	if (_selectedInfo == SELECTION_CELL) {
		pasteFlag = true;

		beginRow = _selectedRowBegin;
		beginColumn = _selectedColumnBegin;
	}
	else if (_selectedInfo == SELECTION_CELLS) {
		pasteFlag = true;

		beginRow = _selectedRowBegin < _selectedRowEnd ? _selectedRowBegin : _selectedRowEnd;
		beginColumn = _selectedColumnBegin < _selectedColumnEnd ? _selectedColumnBegin : _selectedColumnEnd;
	}
	else if (_selectedInfo == SELECTION_ROW) {
		pasteFlag = true;

		beginRow = _selectedRowList[0];
		beginColumn = 1;
	}
	else if (_selectedInfo == SELECTION_COLUMN) {
		pasteFlag = true;

		beginRow = 1;
		beginColumn = _selectedColumnList[0];
	}
	else if (_selectedInfo == SELECTION_ALL) {
		pasteFlag = true;

		beginRow = 1;
		beginColumn = 1;
	}

	if (pasteFlag) {
		for (UINT c = beginRow; (c - beginRow < allLineWordsList.size()) && c < _rowNum; ++c) {
			for (UINT r = beginColumn; (r - beginColumn < allLineWordsList[c - beginRow].size()) && r < _columnNum; ++r) {
				_cellArray[c][r]._text = allLineWordsList[c - beginRow][r - beginColumn];
			}
		}

		UIRefresh();
	}

	return true;
}

bool UIGrid::OnCut() {
	if (_selectedInfo != SELECTION_CELL) {
		return true;
	}
	GridCellInfo* pSelCell = &_cellArray[_selectedRowBegin][_selectedColumnBegin];

	OnCopy();
	pSelCell->_text = L"";

	UIRefresh();
	return true;
}

void UIGrid::OnNotify(int id, LPARAM param) {
	SendMessageToParent(WM_NOTIFY, (WPARAM)id, (LPARAM)param);
}



UIChart::CurveInfo::CurveInfo() {
	_isXCoordInOrder = true; // default the points are in order in x axis

	_beginPointIndex = -1;
	_endPointIndex = -1;

	_color = UIColor::Black;
	_isSelected = false;
	_isShow = true;
	_isLine = true;
}

bool UIChart::CurveInfo::CalcCoordRange(float& xMin, float& xMax, float& yMin, float& yMax) {
	VECTOR_POINT2::iterator itor = _pointList.begin();
	if (itor == _pointList.end()) {
		return false;
	}

	// init the min and max value
	xMin = itor->_x;
	xMax = xMin;
	yMin = itor->_y;
	yMax = yMin;

	// judge the following points
	while (++itor != _pointList.end()) {
		if (xMin > itor->_x) {
			xMin = itor->_x;
		}

		if (xMax < itor->_x) {
			xMax = itor->_x;
		}

		if (yMin > itor->_y) {
			yMin = itor->_y;
		}

		if (yMax < itor->_y) {
			yMax = itor->_y;
		}
	}

	return true;
}

// judge if the point p0 is near the line (p1,p2)
bool UIChart::CurveInfo::JudgePointNearLine(POINT& p0, POINT& p1, POINT& p2)
{
	// handle the case when the line is vertical
	if (p1.x == p2.x) {
		if (abs(p1.x - p0.x) <= 5) {
			return true;
		}
		else {
			return false;
		}
	}

	// handle the case when the line is parallel
	if (p1.y == p2.y) {
		if (abs(p1.y - p0.y) <= 5) {
			return true;
		}
		else {
			return false;
		}
	}

	// normal case, calculate the K and B value of the line
	double k = static_cast<double>(p2.y - p1.y) / (p2.x - p1.x);
	double b = static_cast<double>(p1.y - k * p1.x);
	// calculate the K and B value of the line perpendicular to the line
	double kVert = -1 / k;
	double bVert = static_cast<double>(p0.y - kVert * p0.x);

	// calculate the intersection point
	POINT intersectPoint;
	intersectPoint.x = static_cast<long>((b - bVert) / (kVert - k));
	intersectPoint.y = static_cast<long>(k * intersectPoint.x + b);

	// calculate the distance between two points
	double distanceValue = (intersectPoint.y - p0.y) * (intersectPoint.y - p0.y) + (intersectPoint.x - p0.x) * (intersectPoint.x - p0.x);

	// judge if the distance is near
	return distanceValue <= 25;
}

bool UIChart::CurveInfo::operator==(wstring textName)
{
	return _name == textName;
}

UIChart::UIChart() {
	_isRoomEnoughDraw = true;

	_rowNum = 8;
	_columnNum = 6;

	_isY1CoordRangeCalc = false;
	_isY2CoordRangeCalc = false;
	_isDrawZoomRect = false;

	_coordToPosScaleX = 0;
	_coordToPosScaleY1 = 0;
	_coordToPosScaleY2 = 0;

	_rPointPosUndo = NULL_POINT;
	_rPointPosMove = NULL_POINT;
	_lPointBeginPos = NULL_POINT;
	_lPointEndPos = NULL_POINT;

	_isMove = false;

	_isXCoordLimit = false;
	_isY1CoordLimit = false;
	_isY2CoordLimit = false;
	_xCoordMinLimit = _xCoordMaxLimit = 0.0;
	_y1CoordMinLimit = _y1CoordMaxLimit = 0.0;
	_y2CoordMinLimit = _y2CoordMaxLimit = 0.0;
	_isXCoordSymmetry = false;
	_isY1CoordSymmetry = false;
	_isY2CoordSymmetry = false;
}

void UIChart::Draw() {
	if (!_isShow) {
		return;
	}

	_inheritedTransformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw the border
	RECT rc = _clientRC;
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryBlue, _inheritedTransformMatrix);

	// check if the area is enough to draw
	if (_isRoomEnoughDraw == false) {
		return;
	}

	// draw the grid
	DrawGrid();

	if (_isY1CoordRangeCalc || _isY2CoordRangeCalc) {
		DrawXCoordLable();							// draw the x axis label
		DrawXCoord();								// draw the x axis coordinate
	}

	if (_isY1CoordRangeCalc) {
		DrawY1CoordLable();							// draw the y axis label
		DrawY1Coord();								// draw the y axis coordinate

		DrawCurveList1();
	}

	if (_isY2CoordRangeCalc) {
		DrawY2CoordLable();							// draw the y axis label
		DrawY2Coord();								// draw the y axis coordinate

		DrawCurveList2();
	}

	DrawZoomRect();									// draw the zoom frame
	DrawMousePosAndToolTip();						// draw the mouse position information and tooltip information
}

void UIChart::DrawGrid() {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw the border
	RECT rc = _gridRect;
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(UIColor::Black, _inheritedTransformMatrix);

	int dxPos = GetRectWidth()(_gridRect) / _columnNum;
	int dyPos = GetRectHeight()(_gridRect) / _rowNum;

	// draw _rowNum rows
	for (UINT i = 1; i < _rowNum; ++i) {
		int yPos = _gridRect.top + dyPos * i;
		UILine(_gridRect.left + 1 + x_, yPos + y_, _gridRect.right - 1 + x_, yPos + y_, _z, 1.0f, renderLevel + 1)(UIColor::PrimaryGrayLight, _inheritedTransformMatrix);
	}

	// draw _columnNum column
	for (UINT i = 1; i < _columnNum; ++i) {
		int xPos = _gridRect.left + dxPos * i;
		UILine(xPos + x_, _gridRect.top + 1 + y_, xPos + x_, _gridRect.bottom - 1 + y_, _z, 1.0f, renderLevel + 1)(UIColor::PrimaryGrayLight, _inheritedTransformMatrix);
	}
}

void UIChart::DrawXCoordLable() {
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	UIFont fontHelp(_z, gDefaultFontSize, GetRenderLayout() + 5);

	// Draw x axis name
	SIZE szLabel = fontHelp.GetDrawAreaSize(L"X");
	RECT rc = CreateRect()(UIShape2D::CreatePoint()((_clientRC.right - szLabel.cx) / 2, _clientRC.bottom - szLabel.cy - 3), szLabel);
	OffsetRect(&rc, x_, y_);
	fontHelp(L"X", rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);
}

void UIChart::DrawY1CoordLable() {
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	UIFont fontHelp(_z, gDefaultFontSize, GetRenderLayout() + 5);

	// Draw y1 axis name
	SIZE szLabel = fontHelp.GetDrawAreaSize(L"Y1");
	RECT rc = CreateRect()(UIShape2D::CreatePoint()(3, (_clientRC.bottom - szLabel.cy) / 2), szLabel);
	OffsetRect(&rc, x_, y_);
	fontHelp(L"Y1", rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);
}

void UIChart::DrawY2CoordLable() {
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	UIFont fontHelp(_z, gDefaultFontSize, GetRenderLayout() + 5);

	// Draw y2 axis name
	SIZE szLabel = fontHelp.GetDrawAreaSize(L"Y2");
	RECT rc = CreateRect()(UIShape2D::CreatePoint()(_clientRC.right - szLabel.cx - 3, (_clientRC.bottom - szLabel.cy) / 2), szLabel);
	OffsetRect(&rc, x_, y_);
	fontHelp(L"Y2", rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);
}

void UIChart::DrawXCoord() {
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	UIFont fontHelp(_z, gDefaultFontSize, GetRenderLayout() + 5);

	// draw abscissa
	float dxCoord = (_xCoordRange.second - _xCoordRange.first) / _columnNum;
	float xCoord = _xCoordRange.first;
	int dxPos = (_gridRect.right - _gridRect.left) / _columnNum;
	int xPos = _gridRect.left;
	for (UINT i = 0; i < _columnNum + 1; ++i) {
		wstring str = format(L"{:.1f}", xCoord);
		SIZE strSize = fontHelp.GetDrawAreaSize(str);

		RECT rc = CreateRect()(UIShape2D::CreatePoint()(xPos - strSize.cx / 2, _gridRect.bottom + strSize.cy / 2), strSize);
		OffsetRect(&rc, x_, y_);
		fontHelp(str, rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);

		xCoord += dxCoord;
		xPos += dxPos;
	}

	// calculate the scales
	_coordToPosScaleX = (_xCoordRange.second - _xCoordRange.first) / (_gridRect.right - _gridRect.left);
}

void UIChart::DrawY1Coord() {
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	UIFont fontHelp(_z, gDefaultFontSize, GetRenderLayout() + 5);

	// draw ordinate
	if (_isY1CoordRangeCalc) {
		float dyCoord = (_y1CoordRange.second - _y1CoordRange.first) / _rowNum;
		float yCoord = _y1CoordRange.first;
		int dyPos = (_gridRect.bottom - _gridRect.top) / _rowNum;
		int yPos = _gridRect.bottom;
		for (UINT i = 0; i < _rowNum + 1; ++i) {
			wstring str = format(L"{:.1f}", yCoord);
			SIZE strSize = fontHelp.GetDrawAreaSize(str);

			RECT rc = CreateRect()(UIShape2D::CreatePoint()(_gridRect.left - strSize.cx > 0 ? _gridRect.left - strSize.cx : 0, yPos - strSize.cy / 2), strSize);
			OffsetRect(&rc, x_, y_);
			fontHelp(str, rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);

			yCoord += dyCoord;
			yPos -= dyPos;
		}
	}

	_coordToPosScaleY1 = (_y1CoordRange.second - _y1CoordRange.first) / (_gridRect.bottom - _gridRect.top);
}

void UIChart::DrawY2Coord() {
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	UIFont fontHelp(_z, gDefaultFontSize, GetRenderLayout() + 5);

	if (_isY2CoordRangeCalc) {
		float dyCoord = (_y2CoordRange.second - _y2CoordRange.first) / _rowNum;
		float yCoord = _y2CoordRange.first;
		int dyPos = (_gridRect.bottom - _gridRect.top) / _rowNum;
		int yPos = _gridRect.bottom;
		for (UINT i = 0; i < _rowNum + 1; ++i) {
			wstring str = format(L"{:.1f}", yCoord);
			SIZE strSize = fontHelp.GetDrawAreaSize(str);

			RECT rc = CreateRect()(UIShape2D::CreatePoint()(_gridRect.right + strSize.cx < _clientRC.right ? _gridRect.right : _clientRC.right - strSize.cx - 5, yPos - strSize.cy / 2), strSize);
			OffsetRect(&rc, x_, y_);
			fontHelp(str, rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);

			yCoord += dyCoord;
			yPos -= dyPos;
		}
	}

	_coordToPosScaleY2 = (_y2CoordRange.second - _y2CoordRange.first) / (_gridRect.bottom - _gridRect.top);
}

void UIChart::DrawMousePosAndToolTip() {
	if (_curveList1.size() == 0 && _curveList2.size() == 0) {
		return;
	}

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	UIFont fontHelp(_z, gDefaultFontSize, GetRenderLayout() + 5);

	// show mouse position
	{
		SIZE strSize = fontHelp.GetDrawAreaSize(_mouseCoordStr);

		RECT rc = CreateRect()(UIShape2D::CreatePoint()(_gridRect.right - strSize.cx - 2, _gridRect.top - strSize.cy - 2), strSize);
		OffsetRect(&rc, x_, y_);
		fontHelp(_mouseCoordStr, rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);
	}

	if (_isShowToolTip) {
		SIZE strSize = fontHelp.GetDrawAreaSize(_tooltipStr);

		RECT rc = CreateRect()(UIShape2D::CreatePoint()(_gridRect.left + 2, _gridRect.top - strSize.cy - 2), strSize);
		OffsetRect(&rc, x_, y_);
		fontHelp(_tooltipStr, rc, UIColor::Black, UIFontPos::MiddleLeft, _inheritedTransformMatrix);
	}
}

void UIChart::DrawCurveList1() {
	if (_curveList1.empty()) {
		return;
	}

	// Painting all of the curves
	for (CURVE_LIST::iterator itor = _curveList1.begin(); itor != _curveList1.end(); ++itor) {
		if (itor->_isShow == true) {
			DrawCurve(*itor, 1);
		}
	}
}

void UIChart::DrawCurveList2() {
	if (_curveList2.empty()) {
		return;
	}

	// Painting all of the curves
	for (CURVE_LIST::iterator itor = _curveList2.begin(); itor != _curveList2.end(); ++itor) {
		if (itor->_isShow == true) {
			DrawCurve(*itor, 2);
		}
	}
}

void UIChart::DrawCurve(CurveInfo& curve, int yFlag) {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// line check
	if (curve._pointList.size() == 0) {
		return;
	}
	if (curve._beginPointIndex < 0) {
		return;
	}

	bool prePointInBoxFlag;
	POINT prePointPos = { 0, 0 }, curPointPos = { 0, 0 };

	// calculate the range of the points to be drawn (firstItor, lastItor)
	VECTOR_POINT2::iterator firstItor, lastItor;
	firstItor = curve._pointList.begin() + curve._beginPointIndex;
	if (firstItor != curve._pointList.begin()) {
		--firstItor;
	}
	// 
	if (curve._endPointIndex == curve._pointList.size()) {
		lastItor = curve._pointList.end();
	}
	else {
		lastItor = curve._pointList.begin() + curve._endPointIndex;
		++lastItor;
		if (lastItor != curve._pointList.end()) {
			++lastItor;
		}
	}

	// Processing the first point
	VECTOR_POINT2::iterator itor = firstItor;
	VECTOR_POINT2::iterator preItor = itor;
	if (IsCoordPointInCoordRange(*itor, yFlag) == false) {
		prePointInBoxFlag = false;
	}
	else {
		TransfromCoordToPos(curPointPos, *itor, yFlag);
		Draw2DPoint(curPointPos, curve._color, curve._isSelected);

		prePointPos = curPointPos;
		prePointInBoxFlag = true;
	}
	++itor;
	if (itor >= lastItor) {
		return;
	}

	// Processing the remaining point
	do {
		if (IsCoordPointInCoordRange(*itor, yFlag) == false) { // this point out the range
			if (curve._isLine == false) {
				goto _CALCNEXTPOINT;
			}

			if (prePointInBoxFlag == true) { // the previous point is in the range
				UIPointFloat2 IntersectionPointCoord = CalcIntersectionPoint(*preItor, *itor, 2, yFlag);
				POINT IntersectionPointPos;
				TransfromCoordToPos(IntersectionPointPos, IntersectionPointCoord, yFlag);

				UILine(prePointPos.x + x_, prePointPos.y + y_, IntersectionPointPos.x + x_, IntersectionPointPos.y + y_, _z, 1.0f, renderLevel + 1)(curve._color, _inheritedTransformMatrix);
			}
			// 
			prePointInBoxFlag = false;
		}
		else { // this point is in the range
			TransfromCoordToPos(curPointPos, *itor, yFlag);
			Draw2DPoint(curPointPos, curve._color, curve._isSelected);

			if (curve._isLine == false) {
				goto _CALCNEXTPOINT;
			}

			if (prePointInBoxFlag == true) { // the previous point is in the range
				UILine(prePointPos.x + x_, prePointPos.y + y_, curPointPos.x + x_, curPointPos.y + y_, _z, 1.0f, renderLevel + 1)(curve._color, _inheritedTransformMatrix);
			}
			else { // the previous point is out the range
				UIPointFloat2 IntersectionPointCoord = CalcIntersectionPoint(*preItor, *itor, 1, yFlag);
				POINT IntersectionPointPos;
				TransfromCoordToPos(IntersectionPointPos, IntersectionPointCoord, yFlag);

				UILine(IntersectionPointPos.x + x_, IntersectionPointPos.y + y_, curPointPos.x + x_, curPointPos.y + y_, _z, 1.0f, renderLevel + 1)(curve._color, _inheritedTransformMatrix);
			}
			// 
			prePointPos = curPointPos;
			prePointInBoxFlag = true;
		}

	_CALCNEXTPOINT:  // calculate the next point to be drawn
		preItor = itor;
		++itor;
	} while (itor < lastItor);
}

void UIChart::Draw2DPoint(POINT& pointPos, UIColor& color, bool bigPointFlag) {
	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	if (bigPointFlag) {
		UIPoint(pointPos.x + x_, pointPos.y + y_, _z, renderLevel + 1)(color, _inheritedTransformMatrix);
	}
}

void UIChart::DrawZoomRect() {
	if (_isDrawZoomRect == false) {
		return;
	}

	int renderLevel = GetRenderLayout();
	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// start point block
	UIRect(_lPointBeginPos.x - 2 + x_, _lPointBeginPos.y - 2 + y_, _lPointBeginPos.x + 2 + x_, _lPointBeginPos.y + 2 + y_, _z, renderLevel + 1)(UIColor::PrimaryBlue, 180, _inheritedTransformMatrix);

	// end point block
	UIRect(_lPointEndPos.x - 2 + x_, _lPointEndPos.y - 2 + y_, _lPointEndPos.x + 2 + x_, _lPointEndPos.y + 2 + y_, _z, renderLevel + 1)(UIColor::PrimaryBlue, 180, _inheritedTransformMatrix);

	// middle transparent area
	RECT rc;
	rc.left = _lPointBeginPos.x <= _lPointEndPos.x ? _lPointBeginPos.x : _lPointEndPos.x;
	rc.right = _lPointBeginPos.x > _lPointEndPos.x ? _lPointBeginPos.x : _lPointEndPos.x;
	rc.top = _lPointBeginPos.y <= _lPointEndPos.y ? _lPointBeginPos.y : _lPointEndPos.y;
	rc.bottom = _lPointBeginPos.y > _lPointEndPos.y ? _lPointBeginPos.y : _lPointEndPos.y;
	OffsetRect(&rc, x_, y_);

	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryBlueLight, 50, _inheritedTransformMatrix);
}

void UIChart::CalcArea() {
	// check if the area is enough to draw
	if (GetRectWidth()(_clientRC) < 100 || GetRectHeight()(_clientRC) < 80) {
		_isRoomEnoughDraw = false;
	}
	else {
		_isRoomEnoughDraw = true;
	}

	// grid area
	_gridRect = _clientRC;
	_gridRect.left += 50;
	_gridRect.right -= 50;
	_gridRect.top += 30;
	_gridRect.bottom -= 50;

	// adjust the grid area
	int dxPos = GetRectWidth()(_gridRect) / _columnNum;
	int dyPos = GetRectHeight()(_gridRect) / _rowNum;
	_gridRect.right = _gridRect.left + dxPos * _columnNum;
	_gridRect.bottom = _gridRect.top + dyPos * _rowNum;
}

void UIChart::CalcCurveListDrawRange(int mode) {
	// related checks
	if (_isY1CoordRangeCalc) {
		for (CURVE_LIST::iterator itor = _curveList1.begin(); itor != _curveList1.end(); ++itor) {
			CalcCurveDrawRange(*itor, mode);
		}
	}

	if (_isY2CoordRangeCalc) {
		for (CURVE_LIST::iterator itor = _curveList2.begin(); itor != _curveList2.end(); ++itor) {
			CalcCurveDrawRange(*itor, mode);
		}
	}
}

// [_beginPointIndex, _endPointIndex]
// the draw range is only calculated for the x-axis, and the y-axis is not distinguished
void UIChart::CalcCurveDrawRange(CurveInfo& curve, int mode) {
	// check if there are points
	if (curve._pointList.size() == 0) {
		return;
	}

	// x-axis data is not in order
	if (curve._isXCoordInOrder == false) {
		curve._beginPointIndex = 0;
		curve._endPointIndex = static_cast<int>(curve._pointList.size()) - 1;
		return;
	}

	// x-axis data is in order
	// handle the extreme case where the coordinate point set is not in the coordinate range
	if (curve._pointList.begin()->_x > _xCoordRange.second || curve._pointList.back()._x < _xCoordRange.first) {
		curve._beginPointIndex = -1;
		return;
	}

	// all of the following cases must exist points to be drawn in the coordinate range
	VECTOR_POINT2::iterator beginPointItor, endPointItor;
	if (mode == NOTUSELASTDATA) { // do not use the last data
	_NOTUSELASTDATE:
		beginPointItor = find_if(curve._pointList.begin(), curve._pointList.end(), [v = _xCoordRange.first](const UIPointFloat2& p) { return p._x >= v; });
		endPointItor = find_if(beginPointItor, curve._pointList.end(), [v = _xCoordRange.second](const UIPointFloat2& p) { return p._x > v; });
		--endPointItor;
	}
	else if (curve._beginPointIndex < 0) { // the last data does not exist
		if (mode == RIGHTMOVE) { // curve right move
			beginPointItor = find_if(curve._pointList.rbegin(), curve._pointList.rend(), [v = _xCoordRange.first](const UIPointFloat2& p) { return p._x < v; }).base();
			endPointItor = find_if(beginPointItor, curve._pointList.end(), [v = _xCoordRange.second](const UIPointFloat2& p) { return p._x > v; });
			--endPointItor;
		}
		else if (mode == LEFTMOVE) { // curve left move
			goto _NOTUSELASTDATE;
		}
		else if (mode == ZOOM) { // zoom the curve
		}
		else if (mode == SHRINK) { // shrink the curve
			goto _NOTUSELASTDATE;
		}
		else if (mode == ADDPOINT) { // add the point
			if (curve._pointList.back()._x >= _xCoordRange.first && curve._pointList.back()._x <= _xCoordRange.second) {
				beginPointItor = curve._pointList.end() - 1;
				endPointItor = beginPointItor;
			}
		}
	}
	else { // use the last data to accelerate the calculation
		beginPointItor = curve._pointList.begin() + curve._beginPointIndex;
		if (curve._endPointIndex == curve._pointList.size()) {
			endPointItor = curve._pointList.end();
		}
		else {
			endPointItor = curve._pointList.begin() + curve._endPointIndex;
		}

		if (mode == RIGHTMOVE) { // curve right move
			// use the last data to accelerate the calculation
			VECTOR_POINT2::reverse_iterator ri(beginPointItor + 1);
			ri = find_if(ri, curve._pointList.rend(), [v = _xCoordRange.first](const UIPointFloat2& p) { return p._x < v; });
			beginPointItor = ri.base();
			//
			VECTOR_POINT2::reverse_iterator ri2(endPointItor + 1);
			ri2 = find_if(ri2, curve._pointList.rend(), [v = _xCoordRange.second](const UIPointFloat2& p) { return p._x < v; });
			endPointItor = ri2.base() - 1;
		}
		else if (mode == LEFTMOVE) { // curve left move
			// use the last data to accelerate the calculation
			beginPointItor = find_if(beginPointItor, curve._pointList.end(), [v = _xCoordRange.first](const UIPointFloat2& p) { return p._x > v; });
			// 
			endPointItor = find_if(endPointItor, curve._pointList.end(), [v = _xCoordRange.second](const UIPointFloat2& p) { return p._x > v; });
			--endPointItor;
		}
		else if (mode == ZOOM) { // zoom the curve
			beginPointItor = find_if(beginPointItor, curve._pointList.end(), [v = _xCoordRange.first](const UIPointFloat2& p) { return p._x > v; });

			VECTOR_POINT2::reverse_iterator ri2(endPointItor + 1);
			ri2 = find_if(ri2, curve._pointList.rend(), [v = _xCoordRange.second](const UIPointFloat2& p) { return p._x < v; });
			endPointItor = ri2.base() - 1;
		}
		else if (mode == SHRINK) { // shrink the curve
			VECTOR_POINT2::reverse_iterator ri(beginPointItor + 1);
			ri = find_if(ri, curve._pointList.rend(), [v = _xCoordRange.first](const UIPointFloat2& p) { return p._x < v; });
			beginPointItor = ri.base();

			endPointItor = find_if(endPointItor, curve._pointList.end(), [v = _xCoordRange.second](const UIPointFloat2& p) { return p._x > v; });
			--endPointItor;
		}
		else if (mode == ADDPOINT) { // add the point
			// use the last data to accelerate the calculation
			endPointItor = find_if(endPointItor, curve._pointList.end(), [v = _xCoordRange.second](const UIPointFloat2& p) { return p._x > v; });
			--endPointItor;
		}
	}

	// check if the end point is before the begin point
	if (endPointItor < beginPointItor) {
		endPointItor = beginPointItor;
	}

	// convert the window iterator to the index
	if (beginPointItor == curve._pointList.end()) {
		curve._beginPointIndex = -1;
		return;
	}
	else {
		curve._beginPointIndex = static_cast<int>(beginPointItor - curve._pointList.begin());
		curve._endPointIndex = static_cast<int>(endPointItor - curve._pointList.begin());
	}
}

void UIChart::CalcCoordSymmetry(RANGE_FLOAT& rangeCoord) {
	if (rangeCoord.first <= 0 && rangeCoord.second >= 0) {
		if (rangeCoord.second > -rangeCoord.first) {
			rangeCoord.first = -rangeCoord.second;
		}
		else {
			rangeCoord.second = -rangeCoord.first;
		}
	}
	else if (rangeCoord.first > 0) {
		rangeCoord.first = -rangeCoord.second;
	}
	else if (rangeCoord.second < 0) {
		rangeCoord.second = -rangeCoord.first;
	}
}

bool UIChart::IsCoordPointInCoordRange(UIPointFloat2& pointCoord, int yFlag) {
	RANGE_FLOAT yCoordRange = (yFlag == 1) ? _y1CoordRange : _y2CoordRange;

	return ((pointCoord._x >= _xCoordRange.first && pointCoord._x <= _xCoordRange.second) &&
		(pointCoord._y >= yCoordRange.first && pointCoord._y <= yCoordRange.second));
}

bool UIChart::IsCoordPointInbox(UIPointFloat2& pointCoord, UIPointFloat2& p1, UIPointFloat2& p2, bool nearYFlag) {
	RANGE_FLOAT xCoordRange, yCoordRange;
	xCoordRange.first = p1._x < p2._x ? p1._x : p2._x;
	xCoordRange.second = p1._x > p2._x ? p1._x : p2._x;
	yCoordRange.first = p1._y < p2._y ? p1._y : p2._y;
	yCoordRange.second = p1._y > p2._y ? p1._y : p2._y;

	if (nearYFlag) {
		yCoordRange.first -= 3;
		yCoordRange.second += 3;
	}

	return ((pointCoord._x >= xCoordRange.first && pointCoord._x <= xCoordRange.second) &&
		(pointCoord._y >= yCoordRange.first && pointCoord._y <= yCoordRange.second));
}

// judge whether the postion point in a box range 
bool UIChart::IsPosPointInbox(POINT& pointPos, const RECT& rect) {
	return !(pointPos.x < rect.left || pointPos.x > rect.right || pointPos.y < rect.top || pointPos.y > rect.bottom);
}

bool UIChart::IsPointNearCurveLine(POINT& point, CurveInfo& curve, int yFlag) {
	if (!curve._isLine) {
		return false;
	}

	VECTOR_POINT2::iterator beginPointItor, endPointItor;
	beginPointItor = curve._pointList.begin() + curve._beginPointIndex;
	endPointItor = curve._pointList.begin() + curve._endPointIndex;

	bool prePointInBoxFlag;
	POINT prePointPos, curPointPos;

	UIPointFloat2 pointCoord;
	TransfromPosToCoord(point, pointCoord, yFlag);

	// calculate the range to be judged
	VECTOR_POINT2::iterator firstItor, lastItor;
	firstItor = beginPointItor;
	if (firstItor != curve._pointList.begin()) {
		--firstItor;
	}
	lastItor = endPointItor + 1;
	if (lastItor != curve._pointList.end()) {
		++lastItor;
	}

	// process first point
	VECTOR_POINT2::iterator itor = firstItor;
	if (IsCoordPointInCoordRange(*itor, yFlag) == false) {
		prePointInBoxFlag = false;
	}
	else {
		prePointInBoxFlag = true;
	}

	// process the remaining
	while (++itor != lastItor) {
		if (IsCoordPointInCoordRange(*itor, yFlag) == false) {
			if (prePointInBoxFlag == false) {
				continue;
			}
			prePointInBoxFlag = false;
		}
		else {
			prePointInBoxFlag = true;
		}

		// The current point is not within the box
		if (IsCoordPointInbox(pointCoord, *(itor - 1), *itor, true) == false) {
			continue;
		}

		//
		TransfromCoordToPos(prePointPos, *(itor - 1), yFlag);
		TransfromCoordToPos(curPointPos, *itor, yFlag);

		// 
		if (curve.JudgePointNearLine(point, prePointPos, curPointPos)) {
			return true;
		}
	}

	return false;
}

bool UIChart::IsPointNearCurve(POINT& point, int yFlag) {
	CURVE_LIST& cuveList = yFlag == 1 ? _curveList1 : _curveList2;
	for (CURVE_LIST::iterator i = cuveList.begin(); i != cuveList.end(); ++i) {
		// related checks
		if (i->_isShow == false || i->_pointList.size() == 0 || i->_beginPointIndex < 0) {
			continue;
		}

		//
		if (IsPointNearCurvePoint(point, *i, yFlag)) {
			i->_isSelected = true;
			return true;
		}
	}

	return false;
}

bool UIChart::IsPointNearCurvePoint(POINT& point, CurveInfo& curve, int yFlag) {
	VECTOR_POINT2::iterator beginPointItor, endPointItor;
	beginPointItor = curve._pointList.begin() + curve._beginPointIndex;
	endPointItor = curve._pointList.begin() + curve._endPointIndex;

	// judge all points
	for (VECTOR_POINT2::iterator it = beginPointItor; it != endPointItor + 1; ++it) {
		if (!IsCoordPointInCoordRange(*it, yFlag)) {
			continue;
		}

		// judge whether the point is near by pos
		POINT pointPos;
		TransfromCoordToPos(pointPos, *it, yFlag);
		if ((pointPos.x - point.x) * (pointPos.x - point.x) + (pointPos.y - point.y) * (pointPos.y - point.y) > 16) {
			continue;
		}

		_tooltipStr = format(L"{} name={};coord=[{:.3f}, {:.3f}];index={}",
			yFlag == 1 ? L"Y1" : L"Y2",
			curve._name,
			it->_x,
			it->_y,
			it - curve._pointList.begin());
		return true;
	}

	return false;
}

void UIChart::TransfromCoordToPos(POINT& pointPos, UIPointFloat2& pointCoord, int yFlag) {
	pointPos.x = _gridRect.left + static_cast<long>((pointCoord._x - _xCoordRange.first) / _coordToPosScaleX);

	pointPos.y = yFlag == 1 ? _gridRect.bottom - static_cast<long>((pointCoord._y - _y1CoordRange.first) / _coordToPosScaleY1) :
		_gridRect.bottom - static_cast<long>((pointCoord._y - _y2CoordRange.first) / _coordToPosScaleY2);
}

bool UIChart::TransfromPosToCoord(POINT& pointPos, UIPointFloat2& pointCoord, int yFlag)
{
	// Judge whether in the box
	if (!IsPosPointInbox(pointPos, _gridRect)) {
		return false;
	}

	pointCoord._x = _xCoordRange.first + static_cast<float>(pointPos.x - _gridRect.left) * _coordToPosScaleX;
	pointCoord._y = yFlag == 1 ? _y1CoordRange.first + static_cast<float>(_gridRect.bottom - pointPos.y) * _coordToPosScaleY1 :
		_y2CoordRange.first + static_cast<float>(_gridRect.bottom - pointPos.y) * _coordToPosScaleY2;

	return true;
}

// calculate the intersection of two points and the box, must have one point in the box and one outside
/*
  1	|		2	  | 3
 ---|-------------|----
	|			  |
  4	|			  | 5
	|             |
 ---|-------------|----
  6	|		7	  | 8
*/
UIPointFloat2 UIChart::CalcIntersectionPoint(UIPointFloat2& prePoint, UIPointFloat2& curPoint, int outBoxIndex, int yFlag) {
	UIPointFloat2 intersectPoint;
	UIPointFloat2 outBoxPoint;

	RANGE_FLOAT yCoordRange = (yFlag == 1) ? _y1CoordRange : _y2CoordRange;

	if (outBoxIndex == 1) {
		outBoxPoint = prePoint;
	}
	else {
		outBoxPoint = curPoint;
	}

	// Exclusion of the vertical
	if (prePoint._x == curPoint._x) {
		intersectPoint._x = curPoint._x;
		if (outBoxPoint._y < yCoordRange.first) {
			intersectPoint._y = yCoordRange.first;
			return intersectPoint;
		}
		else {
			intersectPoint._y = yCoordRange.second;
			return intersectPoint;
		}
	}

	// Find the K B value
	float k = (curPoint._y - prePoint._y) / (curPoint._x - prePoint._x);
	float b = curPoint._y - k * (curPoint._x);

	// judge the regional
	if ((outBoxPoint._x >= _xCoordRange.first) && (outBoxPoint._x <= _xCoordRange.second)) {
		if (outBoxPoint._y > yCoordRange.first) {  // regional2
			CalcTopIntersectionPoint(intersectPoint, k, b, yFlag);
		}
		else {  // regional7
			CalcBottomIntersectionPoint(intersectPoint, k, b, yFlag);
		}
	}
	else if ((outBoxPoint._y >= yCoordRange.first) && (outBoxPoint._y <= yCoordRange.second)) {
		if (outBoxPoint._x < _xCoordRange.first) {  // regional4
			CalcLeftIntersectionPoint(intersectPoint, k, b, yFlag);
		}
		else {  // regional5
			CalcRightIntersectionPoint(intersectPoint, k, b, yFlag);
		}
	}
	else if ((outBoxPoint._x < _xCoordRange.first) && (outBoxPoint._y > yCoordRange.second)) {// regional1
		if (CalcLeftIntersectionPoint(intersectPoint, k, b, yFlag) == false) {
			CalcTopIntersectionPoint(intersectPoint, k, b, yFlag);
		}
	}
	else if ((outBoxPoint._x > _xCoordRange.first) && (outBoxPoint._y > yCoordRange.second)) { // regional3
		if (CalcRightIntersectionPoint(intersectPoint, k, b, yFlag) == false) {
			CalcTopIntersectionPoint(intersectPoint, k, b, yFlag);
		}
	}
	else if ((outBoxPoint._x < _xCoordRange.first) && (outBoxPoint._y < yCoordRange.first)) { // regional6
		if (CalcLeftIntersectionPoint(intersectPoint, k, b, yFlag) == false) {
			CalcBottomIntersectionPoint(intersectPoint, k, b, yFlag);
		}
	}
	else if ((outBoxPoint._x > _xCoordRange.first) && (outBoxPoint._y < yCoordRange.first)) { // regional8
		if (CalcRightIntersectionPoint(intersectPoint, k, b, yFlag) == false) {
			CalcBottomIntersectionPoint(intersectPoint, k, b, yFlag);
		}
	}

	return intersectPoint;
}

// calculate the intersection of line and box's left line (coordinate)
bool UIChart::CalcLeftIntersectionPoint(UIPointFloat2& pointCoord, float& k, float& b, int yFlag) {
	RANGE_FLOAT yCoordRange = (yFlag == 1) ? _y1CoordRange : _y2CoordRange;

	pointCoord._x = _xCoordRange.first;
	pointCoord._y = k * (pointCoord._x) + b;
	return pointCoord._y >= yCoordRange.first && pointCoord._y <= yCoordRange.second;
}

// calculate the intersection of line and box's top line (coordinate)
bool UIChart::CalcTopIntersectionPoint(UIPointFloat2& pointCoord, float& k, float& b, int yFlag) {
	RANGE_FLOAT yCoordRange = (yFlag == 1) ? _y1CoordRange : _y2CoordRange;

	pointCoord._y = yCoordRange.second;
	pointCoord._x = (pointCoord._y - b) / k;
	return pointCoord._x >= _xCoordRange.first && pointCoord._x <= _xCoordRange.second;
}

// calculate the intersection of line and box's right line (coordinate)
bool UIChart::CalcRightIntersectionPoint(UIPointFloat2& pointCoord, float& k, float& b, int yFlag) {
	RANGE_FLOAT yCoordRange = (yFlag == 1) ? _y1CoordRange : _y2CoordRange;

	pointCoord._x = _xCoordRange.second;
	pointCoord._y = k * (pointCoord._x) + b;
	return pointCoord._y >= yCoordRange.first && pointCoord._y <= yCoordRange.second;
}

// calculate the intersection of line and box's bottom line (coordinate)
bool UIChart::CalcBottomIntersectionPoint(UIPointFloat2& pointCoord, float& k, float& b, int yFlag) {
	RANGE_FLOAT yCoordRange = (yFlag == 1) ? _y1CoordRange : _y2CoordRange;

	pointCoord._y = yCoordRange.first;
	pointCoord._x = (pointCoord._y - b) / k;
	return pointCoord._x >= _xCoordRange.first && pointCoord._x <= _xCoordRange.second;
}

void UIChart::CalcXYCoordRange() {
	vector<float> xMinList, xMaxList;
	vector<float> y1MinList, y1MaxList;
	vector<float> y2MinList, y2MaxList;

	float xMin, xMax, yMin, yMax;
	for (auto itor = _curveList1.begin(); itor != _curveList1.end(); ++itor) {
		if (!itor->CalcCoordRange(xMin, xMax, yMin, yMax)) {
			continue;
		}

		xMinList.push_back(xMin);
		xMaxList.push_back(xMax);
		y1MinList.push_back(yMin);
		y1MaxList.push_back(yMax);
	}

	for (auto itor = _curveList2.begin(); itor != _curveList2.end(); ++itor) {
		if (!itor->CalcCoordRange(xMin, xMax, yMin, yMax)) {
			continue;
		}

		xMinList.push_back(xMin);
		xMaxList.push_back(xMax);
		y2MinList.push_back(yMin);
		y2MaxList.push_back(yMax);
	}

	if (xMinList.size() > 0) {
		_xCoordRange.first = *min_element(xMinList.begin(), xMinList.end());
		_xCoordRange.second = *max_element(xMaxList.begin(), xMaxList.end());
	}

	if (y1MinList.size() > 0) {
		_isY1CoordRangeCalc = true;
		_y1CoordRange.first = *min_element(y1MinList.begin(), y1MinList.end());
		_y1CoordRange.second = *max_element(y1MaxList.begin(), y1MaxList.end());
	}

	if (y2MinList.size() > 0) {
		_isY2CoordRangeCalc = true;
		_y2CoordRange.first = *min_element(y2MinList.begin(), y2MinList.end());
		_y2CoordRange.second = *max_element(y2MaxList.begin(), y2MaxList.end());
	}

	// set filter
	if (_isXCoordLimit) {
		if (_xCoordRange.first < _xCoordMinLimit) {
			_xCoordRange.first = _xCoordMinLimit;
		}
		if (_xCoordRange.second > _xCoordMaxLimit) {
			_xCoordRange.second = _xCoordMaxLimit;
		}
	}
	if (_isY1CoordLimit) {
		if (_y1CoordRange.first < _y1CoordMinLimit) {
			_y1CoordRange.first = _y1CoordMinLimit;
		}
		if (_y1CoordRange.second > _y1CoordMaxLimit) {
			_y1CoordRange.second = _y1CoordMaxLimit;
		}
	}
	if (_isY2CoordLimit) {
		if (_y2CoordRange.first < _y2CoordMinLimit) {
			_y2CoordRange.first = _y2CoordMinLimit;
		}
		if (_y2CoordRange.second > _y2CoordMaxLimit) {
			_y2CoordRange.second = _y2CoordMaxLimit;
		}
	}
	if (_isXCoordSymmetry) {
		CalcCoordSymmetry(_xCoordRange);
	}
	if (_isY1CoordSymmetry) {
		CalcCoordSymmetry(_y1CoordRange);
	}
	if (_isY2CoordSymmetry) {
		CalcCoordSymmetry(_y2CoordRange);
	}

	CalcCurveListDrawRange(NOTUSELASTDATA);
}

void UIChart::AddCurve1(UIString textName) {
	AddCurve(1, textName);
}

void UIChart::AddCurve1(UIString textName, float xValue, float yValue) {
	AddCurve(1, textName, xValue, yValue);
}

void UIChart::AddCurve1(UIString textName, vector<float>& xList, vector<float>& yList, bool isXCoordInOrder) {
	AddCurve(1, textName, xList, yList, isXCoordInOrder);
}

void UIChart::AddCurve2(UIString textName) {
	AddCurve(2, textName);
}

void UIChart::AddCurve2(UIString textName, float xValue, float yValue) {
	AddCurve(2, textName, xValue, yValue);
}

void UIChart::AddCurve2(UIString textName, vector<float>& xList, vector<float>& yList, bool isXCoordInOrder) {
	AddCurve(2, textName, xList, yList, isXCoordInOrder);
}

void UIChart::SetCurve1Color(UIString textName, UIColor color) {
	SetCurveColor(1, textName, color);
}

void UIChart::SetCurve2Color(UIString textName, UIColor color) {
	SetCurveColor(2, textName, color);
}

void UIChart::SetXCoordLimit(float min, float max) {
	_xCoordMinLimit = min;
	_xCoordMaxLimit = max;
	_isXCoordLimit = true;
}

void UIChart::SetY1CoordLimit(float min, float max) {
	_y1CoordMinLimit = min;
	_y1CoordMaxLimit = max;
	_isY1CoordLimit = true;
}

void UIChart::SetY2CoordLimit(float min, float max) {
	_y2CoordMinLimit = min;
	_y2CoordMaxLimit = max;
	_isY2CoordLimit = true;
}

void UIChart::SetXCoordSymmetry() {
	_isXCoordSymmetry = true;
}

void UIChart::SetY1CoordSymmetry() {
	_isY1CoordSymmetry = true;
}

void UIChart::SetY2CoordSymmetry() {
	_isY2CoordSymmetry = true;
}

void UIChart::SetCurve1Select(UIString textName) {
	SetCurveSelect(1, textName);
}

void UIChart::SetCurve2Select(UIString textName) {
	SetCurveSelect(2, textName);
}

void UIChart::Clear() {
	_isY1CoordRangeCalc = false;
	_curveList1.clear();
	_curveList2.clear();

	_xCoordHistory.clear();
	_y1CoordHistory.clear();
	_y2CoordHistory.clear();
}

void UIChart::SaveCurCoordRange() {
	if (_isY1CoordRangeCalc || _isY2CoordRangeCalc) {
		_xCoordHistory.push_back(_xCoordRange);
		_y1CoordHistory.push_back(_y1CoordRange);
		_y2CoordHistory.push_back(_y2CoordRange);
	}
}

void UIChart::SetXYCoordRange(float xMin, float xMax, float y1Min, float y1Max, float y2Min, float y2Max) {
	SaveCurCoordRange();

	_xCoordRange.first = xMin;
	_xCoordRange.second = xMax;
	_y1CoordRange.first = y1Min;
	_y1CoordRange.second = y1Max;
	_y2CoordRange.first = y2Min;
	_y2CoordRange.second = y2Max;

	_isY1CoordRangeCalc = _curveList1.size() > 0 ? true : false;
	_isY2CoordRangeCalc = _curveList2.size() > 0 ? true : false;

	CalcCurveListDrawRange(NOTUSELASTDATA);
}

void UIChart::GetXYCoordRange(float& xMin, float& xMax, float& y1Min, float& y1Max, float& y2Min, float& y2Max) {
	xMin = _xCoordRange.first;
	xMax = _xCoordRange.second;
	y1Min = _y1CoordRange.first;
	y1Max = _y1CoordRange.second;
	y2Min = _y2CoordRange.first;
	y2Max = _y2CoordRange.second;
}

bool UIChart::OnLButtonDown(POINT pt) {
	// get relative position
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	// judge whether the point is in the grid
	if (!IsPosPointInbox(point, _gridRect)) {
		return true;
	}

	// save the point
	_lPointBeginPos = point;

	// set the point
	for_each(_curveList1.begin(), _curveList1.end(), [](CurveInfo& curve) {curve._isSelected = false; });
	for_each(_curveList2.begin(), _curveList2.end(), [](CurveInfo& curve) {curve._isSelected = false; });

	if (_isY1CoordRangeCalc) {
		if (IsPointNearCurve(point, 1)) {
			return true;
		}
	}

	if (_isY2CoordRangeCalc) {
		if (IsPointNearCurve(point, 2)) {
			return true;
		}
	}

	return true;
}

bool UIChart::OnLButtonUp(POINT) {
	if (_isDrawZoomRect) {
		_isDrawZoomRect = false;

		if (!_isY1CoordRangeCalc && !_isY2CoordRangeCalc) {
			return true;
		}

		if (abs(_lPointEndPos.y - _lPointBeginPos.y) <= 3 && abs(_lPointEndPos.x - _lPointBeginPos.x) <= 3) {
			return true;
		}

		bool xFlag = false;
		float xMin = 0, xMax = 0, y1Min = 0, y1Max = 0;
		if (_isY1CoordRangeCalc) {
			// calculate the zoom rect
			UIPointFloat2 startPointCoord, endPointCoord;
			TransfromPosToCoord(_lPointBeginPos, startPointCoord, 1);
			TransfromPosToCoord(_lPointEndPos, endPointCoord, 1);

			xMin = startPointCoord._x < endPointCoord._x ? startPointCoord._x : endPointCoord._x;
			xMax = startPointCoord._x > endPointCoord._x ? startPointCoord._x : endPointCoord._x;
			y1Min = startPointCoord._y < endPointCoord._y ? startPointCoord._y : endPointCoord._y;
			y1Max = startPointCoord._y > endPointCoord._y ? startPointCoord._y : endPointCoord._y;

			xFlag = true;
		}

		float y2Min = 0, y2Max = 0;
		if (_isY2CoordRangeCalc) {
			// calculate the zoom rect
			UIPointFloat2 startPointCoord, endPointCoord;
			TransfromPosToCoord(_lPointBeginPos, startPointCoord, 2);
			TransfromPosToCoord(_lPointEndPos, endPointCoord, 2);

			if (!xFlag) {
				xMin = startPointCoord._x < endPointCoord._x ? startPointCoord._x : endPointCoord._x;
				xMax = startPointCoord._x > endPointCoord._x ? startPointCoord._x : endPointCoord._x;
			}
			y2Min = startPointCoord._y < endPointCoord._y ? startPointCoord._y : endPointCoord._y;
			y2Max = startPointCoord._y > endPointCoord._y ? startPointCoord._y : endPointCoord._y;
		}

		SetXYCoordRange(xMin, xMax, y1Min, y1Max, y2Min, y2Max);
	}

	return true;
}

bool UIChart::OnRButtonDown(POINT pt) {
	// get relative position
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	_rPointPosUndo = point;

	_isMove = true;
	_rPointPosMove = point;

	return true;
}

bool UIChart::OnRButtonUp(POINT pt) {
	// get relative position
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	if (_rPointPosUndo.x == point.x && _rPointPosUndo.y == point.y) {
		if (_xCoordHistory.size() > 0) {
			_xCoordRange = _xCoordHistory.back();
			_y1CoordRange = _y1CoordHistory.back();
			_y2CoordRange = _y2CoordHistory.back();
			_xCoordHistory.pop_back();
			_y1CoordHistory.pop_back();
			_y2CoordHistory.pop_back();

			CalcCurveListDrawRange(SHRINK);
			UIRefresh();
		}
	}

	_isMove = false;
	return true;
}

bool UIChart::OnMouseMove(POINT pt) {
	// get relative position
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	if (IsKeyDown()(VK_LBUTTON)) { // draw zoom frame
		if (IsPosPointInbox(point, _gridRect)) {
			_isDrawZoomRect = true;
			_lPointEndPos = point;
		}
	}
	else if (IsKeyDown()(VK_RBUTTON)) { // move curve
		if (_isMove) {
			if (IsPosPointInbox(point, _gridRect)) {
				if (_rPointPosMove.x != point.x || _rPointPosMove.y != point.y) {
					float dxPos = (float)(point.x - _rPointPosMove.x);
					float dyPos = (float)(point.y - _rPointPosMove.y);

					float dxCoord = dxPos * _coordToPosScaleX;
					float dy1Coord = dyPos * _coordToPosScaleY1;
					float dy2Coord = dyPos * _coordToPosScaleY2;

					// new coord range
					_xCoordRange.first -= dxCoord;
					_xCoordRange.second -= dxCoord;
					_y1CoordRange.first += dy1Coord;
					_y1CoordRange.second += dy1Coord;
					_y2CoordRange.first += dy2Coord;
					_y2CoordRange.second += dy2Coord;

					CalcCurveListDrawRange(dxPos > 0 ? RIGHTMOVE : LEFTMOVE);

					_rPointPosMove = point;
					UIRefresh();
				}
			}
		}
	}
	else { // show the point on the curve
		_isShowToolTip = false;

		//
		CURVE_LIST::iterator it;
		bool breakFlag = false;
		int yFlag = 1;

		// find the selected curve
		for (it = _curveList1.begin(); it != _curveList1.end(); ++it) {
			if (it->_isSelected == true) {
				breakFlag = true;
				yFlag = 1;
				break;
			}
		}
		if (!breakFlag) {
			for (it = _curveList2.begin(); it != _curveList2.end(); ++it) {
				if (it->_isSelected == true) {
					breakFlag = true;
					yFlag = 2;
					break;
				}
			}
		}

		// judge whether to show the information on the selected curve
		if (breakFlag) {
			// judge whether to show the information
			if (it->_isShow && it->_pointList.size() > 0 && it->_beginPointIndex >= 0) {
				if (IsPointNearCurvePoint(point, *it, yFlag)) {
					_isShowToolTip = true;
				}
			}
		}
	}

	// show the mouse position
	UIPointFloat2 coordPoint1;
	UIPointFloat2 coordPoint2;
	TransfromPosToCoord(point, coordPoint1, 1);
	TransfromPosToCoord(point, coordPoint2, 2);
	_mouseCoordStr = format(L"{:.2f}, {:.2f}, {:.2f}", coordPoint1._x, coordPoint1._y, coordPoint2._y);

	UIRefresh();
	return true;
}

void UIChart::AddCurve(int yFlag, wstring textName) {
	CURVE_LIST& curveList = yFlag == 1 ? _curveList1 : _curveList2;

	CURVE_LIST::iterator itor = find(curveList.begin(), curveList.end(), textName);
	if (itor == curveList.end()) {
		// add one if not exist
		CurveInfo curve;
		curve._name = textName;
		curveList.push_back(curve);
	}
}

void UIChart::AddCurve(int yFlag, wstring textName, float xValue, float yValue) {
	CURVE_LIST& curveList = yFlag == 1 ? _curveList1 : _curveList2;

	CURVE_LIST::iterator itor = find(curveList.begin(), curveList.end(), textName);
	if (itor != curveList.end()) {
		CurveInfo& curve = *itor;

		// the point will affect the order of the curve
		if (curve._pointList.size() > 0 && curve._isXCoordInOrder) {
			if (xValue < curve._pointList.back()._x)
				curve._isXCoordInOrder = false;
		}

		curve._pointList.push_back(UIPointFloat2(xValue, yValue));

		// check whether the draw range is calculated
		if ((yFlag == 1 && _isY1CoordRangeCalc) || (yFlag == 2 && _isY2CoordRangeCalc)) {
			CalcCurveDrawRange(curve, curve._beginPointIndex > 0 ? NOTUSELASTDATA : ADDPOINT);
		}
	}
}

void UIChart::AddCurve(int yFlag, wstring textName, vector<float>& xList, vector<float>& yList, bool isXCoordInOrder) {
	CURVE_LIST& curveList = yFlag == 1 ? _curveList1 : _curveList2;

	CURVE_LIST::iterator itor = find(curveList.begin(), curveList.end(), textName);
	if (itor != curveList.end()) {
		curveList.erase(itor);
	}
	else {
		CurveInfo curve;
		curve._isXCoordInOrder = isXCoordInOrder;
		curve._name = textName;
		for (UINT i = 0; i < xList.size(); ++i) {
			curve._pointList.push_back(UIPointFloat2(xList[i], yList[i]));
		}
		curveList.push_back(curve);

		CalcCurveDrawRange(curve, NOTUSELASTDATA);
	}
}

void UIChart::SetCurveColor(int yFlag, wstring textName, UIColor& color) {
	CURVE_LIST& curveList = yFlag == 1 ? _curveList1 : _curveList2;

	CURVE_LIST::iterator itor = find(curveList.begin(), curveList.end(), textName);
	if (itor != curveList.end()) {
		CurveInfo& curve = *itor;
		curve._color = color;
	}
}

void UIChart::SetCurveSelect(int yFlag, wstring textName) {
	CURVE_LIST& curveList = yFlag == 1 ? _curveList1 : _curveList2;
	CURVE_LIST::iterator itor = find(curveList.begin(), curveList.end(), textName);
	if (itor != curveList.end()) {
		itor->_isSelected = true;
	}
}



UITab::UITab() {
	_isDraw = true;

	_xyFlag = X_FLAG;
	_selectedIndex = 0;
	_hoverIndex = -1;

	_isAnimate3D = true;

	_isTransmissionMsg = true;
}

void UITab::CalcArea() {
	if (_cellRCList.size() == 0 || GetRectWidth()(_clientRC) < (TAB_1 + TAB_2) || GetRectHeight()(_clientRC) < (TAB_1 + TAB_2)) {
		_isDraw = false;
		return;
	}

	_isDraw = true;

	CalcTabRect();
	CalcTabSelLine();
	CalcCellListRect();
}

void UITab::CalcTabRect() {
	_tabRC = _clientRC;

	if (_xyFlag == X_FLAG) {
		_tabRC.bottom = TAB_1;

		for (UINT i = 0; i < _tabRCList.size(); ++i) {
			_tabRCList[i] = _tabRC;
			_tabRCList[i].left = GetRectWidth()(_tabRC) * i / (LONG)_tabRCList.size();
			_tabRCList[i].right = GetRectWidth()(_tabRC) * (i + 1) / (LONG)_tabRCList.size();
		}
	}
	else {
		_tabRC.right = TAB_1;

		for (UINT i = 0; i < _tabRCList.size(); ++i) {
			_tabRCList[i] = _tabRC;
			_tabRCList[i].top = GetRectHeight()(_tabRC) * i / (LONG)_tabRCList.size();
			_tabRCList[i].bottom = GetRectHeight()(_tabRC) * (i + 1) / (LONG)_tabRCList.size();
		}
	}
}

void UITab::CalcTabSelLine() {
	if (_selectedIndex == -1) {
		return;
	}

	if (_xyFlag == X_FLAG) {
		_lineRC = _tabRCList[_selectedIndex];
		_lineRC.bottom = _lineRC.top + TAB_2;
	}
	else {
		_lineRC = _tabRCList[_selectedIndex];
		_lineRC.right = _lineRC.left + TAB_2;
	}
}

void UITab::CalcCellListRect() {
	_cellRC = _clientRC;

	if (_xyFlag == X_FLAG) {
		_cellRC.top += TAB_1;
	}
	else {
		_cellRC.left += TAB_1;
	}

	if (_xyFlag == X_FLAG) {
		int left = _cellRCList[_selectedIndex].left;
		_cellRCList[_selectedIndex] = _cellRC;
		OffsetRect(&_cellRCList[_selectedIndex], left, 0);

		int cellWidth = GetRectWidth()(_cellRC);
		for (UINT i = 0; i < _cellRCList.size(); ++i) {
			if (i == _selectedIndex) {
				continue;
			}

			int dx = (i - _selectedIndex) * cellWidth;
			_cellRCList[i] = _cellRC;
			OffsetRect(&_cellRCList[i], dx + left, 0);
		}
	}
	else {
		int top = _cellRCList[_selectedIndex].top;
		_cellRCList[_selectedIndex] = _cellRC;
		OffsetRect(&_cellRCList[_selectedIndex], 0, top);

		int cellHeight = GetRectHeight()(_cellRC);
		for (UINT i = 0; i < _cellRCList.size(); ++i) {
			if (i == _selectedIndex) {
				continue;
			}

			int dx = (i - _selectedIndex) * cellHeight;
			_cellRCList[i] = _cellRC;
			OffsetRect(&_cellRCList[i], 0, dx + top);
		}
	}
}

void UITab::Draw() {
	if (!_isDraw) {
		return;
	}

	_inheritedTransformMatrix = GetInheritedTransformMatrix();

	// all child windows are set to not accept messages
	for (UINT i = 0; i < _cellList.size(); ++i) {
		if (_cellList[i].p_win != NULL) {
			_cellList[i].p_win->ShowWindow(false);
		}
	}

	DrawTab();

	if (!IsAnimationRun()) {
		DrawTabSelLine();

		// draw the selected cell
		CellData& cell = _cellList[_selectedIndex];
		if (cell.p_win != NULL) {
			cell.p_win->MoveWindow(_cellRC);
			cell.p_win->ShowWindow(true);
			cell.p_win->Draw();
		}
	}
	else {
		DrawAnimate1();
	}
}

void UITab::DrawTab() {
	int renderLevel = GetRenderLayout();
	for (UINT i = 0; i < _tabRCList.size(); ++i) {
		RECT rc = _tabRCList[i];
		OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);

		if (_hoverIndex == (int)i) {
			UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurpleLight, 150, _inheritedTransformMatrix);
		}
		else {
			UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryPurpleLight, 255, _inheritedTransformMatrix);
		}

		UIFont(_z, gDefaultFontSize, renderLevel + 5).operator()(_cellList[i]._title, rc, UIColor::Black, UIFontPos::MiddleCenter, _inheritedTransformMatrix);
	}
}

void UITab::DrawTabSelLine() {
	int renderLevel = GetRenderLayout();
	RECT rc = _lineRC;
	OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryGreen, 255, _inheritedTransformMatrix);
}

void UITab::DrawAnimate1() {
	DrawAnimate1Cell();
	DrawAnimate1Tab();
}

void UITab::DrawAnimate1Cell() {
	if (_xyFlag == X_FLAG) {
		int dx = _cellRCList[_selectedIndex].left / (_maxFrame - _frameIndex + 1);

		for (UINT i = 0; i < _cellRCList.size(); ++i) {
			OffsetRect(&_cellRCList[i], -dx, 0);
		}
	}
	else {
		int dx = _cellRCList[_selectedIndex].top / (_maxFrame - _frameIndex + 1);

		for (UINT i = 0; i < _cellRCList.size(); ++i) {
			OffsetRect(&_cellRCList[i], 0, -dx);
		}
	}

	// move and draw cell
	RECT rc = _clientRC;
	OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
	UIScreenClipRectGuard uiClip(rc);

	for (UINT i = 0; i < _cellRCList.size(); ++i) {
		UIWindowBase* p_win = _cellList[i].p_win;
		if (p_win == NULL) {
			continue;
		}

		if (_xyFlag == X_FLAG) {
			if (_cellRCList[i].right < _cellRC.left || _cellRCList[i].left > _cellRC.right) {
				continue;
			}
			else {
				float k = (float)(_cellRCList[i].left - _cellRC.left) / (_cellRC.right - _cellRC.left);
				rc = _cellRCList[i];
				OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
				if (_isAnimate3D) {
					POINT center = GetRectCenter()(rc);
					XMMATRIX transformMatrix = UIZPlaneTransform::GetTransformMatrix(false, 0, 0, 0, false, 0, 0, true, (float)center.x, -k * XM_PI / 8, _z);
					p_win->SetTransformMatrix(transformMatrix);
				}
			}
		}
		else {
			if (_cellRCList[i].bottom < _cellRC.top || _cellRCList[i].top > _cellRC.bottom) {
				continue;
			}
			else {
				float k = (float)(_cellRCList[i].top - _cellRC.top) / (_cellRC.bottom - _cellRC.top);
				rc = _cellRCList[i];
				OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
				if (_isAnimate3D) {
					POINT center = GetRectCenter()(rc);
					XMMATRIX transformMatrix = UIZPlaneTransform::GetTransformMatrix(false, 0, 0, 0, true, (float)center.y, -k * XM_PI / 8, false, 0, 0, _z);
					p_win->SetTransformMatrix(transformMatrix);
				}
			}
		}

		p_win->MoveWindow(_cellRCList[i]);
		p_win->ShowWindow(true);
		p_win->Draw();
	}
}

void UITab::DrawAnimate1Tab() {
	int renderLevel = GetRenderLayout();
	if (_xyFlag == X_FLAG) {
		int dx = (_tabRCList[_selectedIndex].left - _lineRC.left) / (_maxFrame - _frameIndex + 1);
		OffsetRect(&_lineRC, dx, 0);
	}
	else {
		int dx = (_tabRCList[_selectedIndex].top - _lineRC.top) / (_maxFrame - _frameIndex + 1);
		OffsetRect(&_lineRC, 0, dx);
	}

	RECT rc = _lineRC;
	OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryGreen, 255, _inheritedTransformMatrix);
}

void UITab::SetCellNum(UINT num) {
	_tabRCList.resize(num);
	_cellList.resize(num);
	_cellRCList.resize(num);
	_selectedIndex = 0;

	CalcArea();
}

void UITab::SetCell(UINT index, UIString title, UIWindowBase* pWin) {
	_cellList[index]._title = title;
	_cellList[index].p_win = pWin;

	// fix UITab UILayoutCalc no working ??
	pWin->_clientRC = _cellRC;
}

void UITab::SetCurCell(UINT index) {
	if (index < _cellList.size()) {
		if (_selectedIndex == index) {
			return;
		}

		_selectedIndex = index;
		PlayAnimate(MAX_FRAME1);
	}
}

int UITab::GetCurCell() {
	return _selectedIndex;
}

void UITab::SetX() {
	_xyFlag = X_FLAG;

	CalcArea();
}

void UITab::SetY() {
	_xyFlag = Y_FLAG;

	CalcArea();
}

void UITab::SetAnimate3D(int isAnimate3D) {
	_isAnimate3D = isAnimate3D;
}

bool UITab::OnMouseMove(POINT pt) {
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	if (!ContainsPoint()(point, _tabRC)) {
		if (_hoverIndex != -1) {
			_hoverIndex = -1;
			UIRefresh();
		}
	}
	else {
		for (UINT i = 0; i < _tabRCList.size(); ++i) {
			if (ContainsPoint()(point, _tabRCList[i])) {
				if (_hoverIndex != (int)i) {
					_hoverIndex = i;
					UIRefresh();
				}
				return true;
			}
		}
	}

	return true;
}

void UITab::OnMouseLeave(POINT) {
	if (_hoverIndex != -1) {
		_hoverIndex = -1;
		UIRefresh();
	}
}

bool UITab::OnLButtonDown(POINT pt) {
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	if (!ContainsPoint()(point, _tabRC)) {
		return true;
	}

	for (UINT i = 0; i < _tabRCList.size(); ++i) {
		if (ContainsPoint()(point, _tabRCList[i])) {
			if (_selectedIndex == i) {
				return true;
			}

			SetCurCell(i);
			return true;
		}
	}

	return true;
}

UICanvas::UICanvas() {
	_isTransmissionMsg = true;

	_alpha = 0;
	_color = UIColor::Gray95;

	_layoutOrder = 0;
}

void UICanvas::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	if (_alpha != 0 && _color != UIColor::Invalid) {
		RECT rc = _clientRC;
		OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
		UIRect(rc, _z, renderLevel + 1)(_color, _alpha, transformMatrix);
	}

	if (p_UIContainer != NULL) {
		p_UIContainer->Draw();
	}
}

void UICanvas::SetColor(UIColor color) {
	_color = color;
}

void UICanvas::SetAlpha(UCHAR alpha) {
	_alpha = alpha;
}




void UIColorPanel::Draw() {
	if (!_isShow) {
		return;
	}

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();
	_z = 1.f;

	RECT rc = _clientRC;
	rc.left -= 5;
	rc.top -= 5;
	rc.right += 5;
	rc.bottom += 5;
	OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
	UIRect(rc, _z, renderLevel)(UIColor::Gray95, 255, transformMatrix);
}


UICanvas3D::UICanvas3D() {
	_isHover = false;
	_layoutOrder = 0;
}


void UICanvas3D::Draw() {
	if (!_isShow) {
		return;
	}

	int renderLevel = GetRenderLayout();

	float offsetX = 0.f;
	float offsetY = 0.f;

	if (_isHover) {
		POINT curPT = _curMousePos;
		curPT.x -= _abusolutePoint.x;
		curPT.y -= _abusolutePoint.y;

		// Calculate offset between recorded _curMousePos and center position
		POINT center = { GetRectWidth()(_clientRC) / 2, GetRectHeight()(_clientRC) / 2 };
		POINT offset = { curPT.x - center.x, curPT.y - center.y };

		// Calculate offset ratio
		offsetX = (float)offset.x / center.x;
		offsetY = (float)offset.y / center.y;
	}

	RECT rc = _clientRC;
	OffsetRect(&rc, _abusolutePoint.x, _abusolutePoint.y);
	POINT center = GetRectCenter()(rc);
	rc.left -= 5;
	rc.top -= 5;
	rc.right += 5;
	rc.bottom += 5;

	XMMATRIX localTransform = UIZPlaneTransform::GetTransformMatrix(false, 0, 0, 0, true, (float)center.y, -offsetY * XM_PI / 128,
		true, (float)center.x, -offsetX * XM_PI / 128, _z);
	SetTransformMatrix(localTransform);

	XMMATRIX transformMatrix = GetInheritedTransformMatrix();

	// Background offset = 1
	UIRect(rc, _z + gDeltaZ, renderLevel)(UIColor::Gray95, 255, transformMatrix);
	//UIFont(_z - gDeltaZ * 10, 40).operator()(L"UICanvas3D", rc, UIColor::Black, UIFontPos::MiddleCenter, transformMatrix);

	if (p_UIContainer != NULL) {
		p_UIContainer->Draw();
	}

}


bool UICanvas3D::OnMouseMove(POINT pt) {
	if (!_isHover) {
		OnMouseHoverEvent();
		_isHover = true;
	}

	_curMousePos = pt;
	UIRefresh();

	return true;
}


bool UICanvas3D::OnMouseLeave(POINT) {
	_isHover = false;
	UIRefresh();

	return true;
}

/*------------------------------------------------------- UIChart3D -------------------------------------------------------*/
// UIChart3D::CurveInfo implementation
UIChart3D::CurveInfo::CurveInfo() : _name(L""), _color(UIColor::White), _isShow(true) {
}

bool UIChart3D::CurveInfo::CalcCoordRange(float& xMin, float& xMax, float& yMin, float& yMax, float& zMin, float& zMax) {
	if (_pointList.empty()) {
		return false;
	}

	xMin = xMax = _pointList[0]._x;
	yMin = yMax = _pointList[0]._y;
	zMin = zMax = _pointList[0]._z;

	for (const auto& point : _pointList) {
		if (point._x < xMin) xMin = point._x;
		if (point._x > xMax) xMax = point._x;
		if (point._y < yMin) yMin = point._y;
		if (point._y > yMax) yMax = point._y;
		if (point._z < zMin) zMin = point._z;
		if (point._z > zMax) zMax = point._z;
	}

	return true;
}

bool UIChart3D::CurveInfo::operator==(wstring textName) {
	return _name == textName;
}

UIChart3D::UIChart3D() {
}

UIChart3D::~UIChart3D() {
	// No cleanup needed for direct member variables
}

void UIChart3D::CalcArea() {
	RECT ctrlRC = GetAbsoluteRect();

	// Set camera
	if (!_cameraCtrl.SetUpCamera(ctrlRC)) {
		return;
	}

	// Calculate 3D axes coordinates
	CalcAxes3D();
}

void UIChart3D::CalcAxes3D() {
	RECT ctrlRC = GetAbsoluteRect();

	// Calculate 3D axes coordinates
	LONG ctrlWidth = GetRectWidth()(ctrlRC);
	LONG ctrlHeight = GetRectHeight()(ctrlRC);
	LONG availablePixels = min(ctrlWidth, ctrlHeight) - 150;  // Reserve 75 pixels margin on each side

	// Calculate world space dimensions
	float worldHeight = _cameraCtrl.GetWorldHeight();
	float worldWidth = _cameraCtrl.GetWorldWidth();

	// Calculate world coordinate distance for 50 pixels margin
	float marginX = (((ctrlWidth - availablePixels) / 2.0f) / ctrlWidth) * worldWidth;
	float marginY = (((ctrlHeight - availablePixels) / 2.0f) / ctrlHeight) * worldHeight;

	// Calculate original world coordinate point (axes origin)
	float p0X = _cameraCtrl._target.x - (worldWidth / 2.0f) + marginX;
	float p0Y = _cameraCtrl._target.y - (worldHeight / 2.0f) + marginY;
	float p0Z = 0.0f;

	// Set axes origin
	_axesOrigin = { p0X, p0Y, p0Z };

	// Calculate world length for axes
	float worldLength = availablePixels * (worldHeight / ctrlHeight);
	_axisLengths = { worldLength, worldLength, worldLength };

	// Calculate axes endpoints
	_xAxisEnd = { _axesOrigin._x + worldLength, _axesOrigin._y, _axesOrigin._z };
	_yAxisEnd = { _axesOrigin._x, _axesOrigin._y + worldLength, _axesOrigin._z };
	_zAxisEnd = { _axesOrigin._x, _axesOrigin._y, _axesOrigin._z + worldLength };

	// Calculate tick mark length (1/40 of axis length, half the original size)
	float tickLength = worldLength / 40.0f;

	// Clear previous subdivision data
	_xAxisTicks.clear();
	_yAxisTicks.clear();
	_zAxisTicks.clear();
	_xTickLines.clear();
	_yTickLines.clear();
	_zTickLines.clear();
	_xArrowTriangle.clear();
	_yArrowTriangle.clear();
	_zArrowTriangle.clear();

	// Calculate X-axis subdivision points (5 intermediate points)
	for (int i = 1; i <= 5; ++i) {
		float t = i / 6.0f;  // Division ratio (1/6, 2/6, 3/6, 4/6, 5/6)
		UIPointFloat3 tickPoint = {
			_axesOrigin._x + t * worldLength,
			_axesOrigin._y,
			_axesOrigin._z
		};
		_xAxisTicks.push_back(tickPoint);

		// Create tick mark perpendicular to X-axis (in XY plane)
		UIPointFloat3 tickStart = { tickPoint._x, tickPoint._y - tickLength / 2, tickPoint._z };
		UIPointFloat3 tickEnd = { tickPoint._x, tickPoint._y + tickLength / 2, tickPoint._z };
		_xTickLines.push_back({ tickStart, tickEnd });
	}

	// Calculate Y-axis subdivision points (5 intermediate points)
	for (int i = 1; i <= 5; ++i) {
		float t = i / 6.0f;
		UIPointFloat3 tickPoint = {
			_axesOrigin._x,
			_axesOrigin._y + t * worldLength,
			_axesOrigin._z
		};
		_yAxisTicks.push_back(tickPoint);

		// Create tick mark perpendicular to Y-axis (in XY plane)
		UIPointFloat3 tickStart = { tickPoint._x - tickLength / 2, tickPoint._y, tickPoint._z };
		UIPointFloat3 tickEnd = { tickPoint._x + tickLength / 2, tickPoint._y, tickPoint._z };
		_yTickLines.push_back({ tickStart, tickEnd });
	}

	// Calculate Z-axis subdivision points (5 intermediate points)
	for (int i = 1; i <= 5; ++i) {
		float t = i / 6.0f;
		UIPointFloat3 tickPoint = {
			_axesOrigin._x,
			_axesOrigin._y,
			_axesOrigin._z + t * worldLength
		};
		_zAxisTicks.push_back(tickPoint);

		// Create tick mark perpendicular to Z-axis (in YZ plane)
		UIPointFloat3 tickStart = { tickPoint._x, tickPoint._y - tickLength / 2, tickPoint._z };
		UIPointFloat3 tickEnd = { tickPoint._x, tickPoint._y + tickLength / 2, tickPoint._z };
		_zTickLines.push_back({ tickStart, tickEnd });
	}

	// Calculate equilateral triangle arrows extending beyond axis endpoints
	float arrowSize = tickLength * 1.5f;  // Double the size of tick marks
	float halfArrowSize = arrowSize / 2.0f;
	float arrowHeight = arrowSize * 0.866f;  // Height of equilateral triangle (sqrt(3)/2)

	// X-axis arrow (in XY plane, extending beyond X axis endpoint)
	UIPointFloat3 xArrowTip = { _xAxisEnd._x + arrowHeight, _xAxisEnd._y, _xAxisEnd._z };
	_xArrowTriangle.push_back(xArrowTip);  // Tip point (extended beyond axis)
	_xArrowTriangle.push_back({ _xAxisEnd._x, _xAxisEnd._y - halfArrowSize, _xAxisEnd._z });  // Bottom left
	_xArrowTriangle.push_back({ _xAxisEnd._x, _xAxisEnd._y + halfArrowSize, _xAxisEnd._z });  // Bottom right

	// Y-axis arrow (in XY plane, extending beyond Y axis endpoint)
	UIPointFloat3 yArrowTip = { _yAxisEnd._x, _yAxisEnd._y + arrowHeight, _yAxisEnd._z };
	_yArrowTriangle.push_back(yArrowTip);  // Tip point (extended beyond axis)
	_yArrowTriangle.push_back({ _yAxisEnd._x - halfArrowSize, _yAxisEnd._y, _yAxisEnd._z });  // Bottom left
	_yArrowTriangle.push_back({ _yAxisEnd._x + halfArrowSize, _yAxisEnd._y, _yAxisEnd._z });  // Bottom right

	// Z-axis arrow (in YZ plane, extending beyond Z axis endpoint)
	UIPointFloat3 zArrowTip = { _zAxisEnd._x, _zAxisEnd._y, _zAxisEnd._z + arrowHeight };
	_zArrowTriangle.push_back(zArrowTip);  // Tip point (extended beyond axis)
	_zArrowTriangle.push_back({ _zAxisEnd._x, _zAxisEnd._y - halfArrowSize, _zAxisEnd._z });  // Bottom left
	_zArrowTriangle.push_back({ _zAxisEnd._x, _zAxisEnd._y + halfArrowSize, _zAxisEnd._z });  // Bottom right
}

void UIChart3D::DrawAxes3D() {
	// Draw the three main coordinate axes using UIPointFloat3 coordinates directly
	UILine3D(_axesOrigin, _xAxisEnd)(UIColor::Red, &_cameraCtrl);		// X-axis (Red)
	UILine3D(_axesOrigin, _yAxisEnd)(UIColor::Green, &_cameraCtrl);	    // Y-axis (Green)
	UILine3D(_axesOrigin, _zAxisEnd)(UIColor::Blue, &_cameraCtrl);	    // Z-axis (Blue)

	// Draw X-axis tick marks (perpendicular lines in XY plane)
	for (const auto& tickLine : _xTickLines) {
		UILine3D(tickLine.first, tickLine.second)(UIColor::Red, &_cameraCtrl);
	}

	// Draw Y-axis tick marks (perpendicular lines in XY plane)
	for (const auto& tickLine : _yTickLines) {
		UILine3D(tickLine.first, tickLine.second)(UIColor::Green, &_cameraCtrl);
	}

	// Draw Z-axis tick marks (perpendicular lines in YZ plane)
	for (const auto& tickLine : _zTickLines) {
		UILine3D(tickLine.first, tickLine.second)(UIColor::Blue, &_cameraCtrl);
	}

	// Draw X-axis arrow triangle (equilateral triangle in XY plane)
	if (_xArrowTriangle.size() == 3) {
		UITriangle3D(_xArrowTriangle[0], _xArrowTriangle[1], _xArrowTriangle[2])(UIColor::Red, 255, &_cameraCtrl);
	}

	// Draw Y-axis arrow triangle (equilateral triangle in XY plane)
	if (_yArrowTriangle.size() == 3) {
		UITriangle3D(_yArrowTriangle[0], _yArrowTriangle[1], _yArrowTriangle[2])(UIColor::Green, 255, &_cameraCtrl);
	}

	// Draw Z-axis arrow triangle (equilateral triangle in YZ plane)
	if (_zArrowTriangle.size() == 3) {
		UITriangle3D(_zArrowTriangle[0], _zArrowTriangle[1], _zArrowTriangle[2])(UIColor::Blue, 255, &_cameraCtrl);
	}
}

void UIChart3D::Draw() {
	XMMATRIX transformMatrix = GetInheritedTransformMatrix();
	int renderLevel = GetRenderLayout();

	LONG& x_ = _abusolutePoint.x;
	LONG& y_ = _abusolutePoint.y;

	// draw border
	RECT rc = _clientRC;
	OffsetRect(&rc, x_, y_);
	UIRect(rc, _z, renderLevel + 1)(UIColor::PrimaryBlue, transformMatrix);

	// Draw 3D axes using pre-calculated coordinates
	DrawAxes3D();

	// Draw all 3D curves
	DrawCurves();

	// Get command list
	UIFont3D(18.0f)(L"3DS OOPING", { _xAxisEnd._x / 2, _yAxisEnd._y / 2, 0.0f }, UIColor::Black, &_cameraCtrl);
}

bool UIChart3D::OnRButtonDown(POINT pt) {
	// Convert to control-relative coordinates
	POINT point = pt;
	point.x -= _abusolutePoint.x;
	point.y -= _abusolutePoint.y;

	_lastMousePos = point;
	_moveFlag = true;  // Set right button down flag
	return true;
}

bool UIChart3D::OnRButtonUp(POINT) {
	_moveFlag = false;  // Clear right button down flag
	return true;
}

void UIChart3D::OnMouseLeave(POINT) {
	_moveFlag = false;  // Clear right button down flag when mouse leaves
}

bool UIChart3D::OnMouseMove(POINT pt) {
	// Only allow movement when right button is down
	if (_moveFlag) {
		POINT point = pt;
		point.x -= _abusolutePoint.x;
		point.y -= _abusolutePoint.y;

		// Calculate mouse movement distance
		int deltaX = point.x - _lastMousePos.x;
		int deltaY = point.y - _lastMousePos.y;

		// Only rotate when there is actual movement
		if (deltaX != 0 || deltaY != 0 || _lastMousePos.x != 0 || _lastMousePos.y != 0) {
			// Convert to rotation angles (further reduced sensitivity)
			float sensitivity = 0.1f;
			float deltaYaw = deltaX * sensitivity;
			float deltaPitch = -deltaY * sensitivity;  // Y-axis inverted

			// Rotate camera
			_cameraCtrl.RotateCamera(deltaYaw, deltaPitch);

			// Refresh display
			UIRefresh();
		}

		// Update last mouse position
		_lastMousePos = point;
	}

	return true;
}

// Coordinate system setup functions
void UIChart3D::SetDataCoordRange(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax) {
	_xDataRange = { xMin, xMax };
	_yDataRange = { yMin, yMax };
	_zDataRange = { zMin, zMax };

	// Recalculate axes after range change
	CalcAxes3D();
}

// Coordinate transformation functions
UIPointFloat3 UIChart3D::DataToWorld(const UIPointFloat3& dataCoord) const {
	// Convert data coordinates to normalized coordinates [0,1]
	float normalizedX = (dataCoord._x - _xDataRange.first) / (_xDataRange.second - _xDataRange.first);
	float normalizedY = (dataCoord._y - _yDataRange.first) / (_yDataRange.second - _yDataRange.first);
	float normalizedZ = (dataCoord._z - _zDataRange.first) / (_zDataRange.second - _zDataRange.first);

	// Convert normalized coordinates to world coordinates
	UIPointFloat3 worldCoord;
	worldCoord._x = _axesOrigin._x + normalizedX * _axisLengths._x;
	worldCoord._y = _axesOrigin._y + normalizedY * _axisLengths._y;
	worldCoord._z = _axesOrigin._z + normalizedZ * _axisLengths._z;

	return worldCoord;
}

UIPointFloat3 UIChart3D::WorldToData(const UIPointFloat3& worldCoord) const {
	// Convert world coordinates to normalized coordinates [0,1]
	float normalizedX = (worldCoord._x - _axesOrigin._x) / _axisLengths._x;
	float normalizedY = (worldCoord._y - _axesOrigin._y) / _axisLengths._y;
	float normalizedZ = (worldCoord._z - _axesOrigin._z) / _axisLengths._z;

	// Convert normalized coordinates to data coordinates
	UIPointFloat3 dataCoord;
	dataCoord._x = _xDataRange.first + normalizedX * (_xDataRange.second - _xDataRange.first);
	dataCoord._y = _yDataRange.first + normalizedY * (_yDataRange.second - _yDataRange.first);
	dataCoord._z = _zDataRange.first + normalizedZ * (_zDataRange.second - _zDataRange.first);

	return dataCoord;
}

POINT UIChart3D::WorldToScreen(const UIPointFloat3& worldCoord) {
	// Apply view-projection transformation
	// This is a simplified implementation - in real scenario, you would use proper matrix multiplication

	// For now, implement basic isometric projection
	float screenX = worldCoord._x - 0.5f * worldCoord._z;
	float screenY = worldCoord._y - 0.5f * worldCoord._z;

	// Convert to screen coordinates relative to control
	RECT ctrlRC = GetAbsoluteRect();
	float centerX = (ctrlRC.left + ctrlRC.right) * 0.5f;
	float centerY = (ctrlRC.top + ctrlRC.bottom) * 0.5f;

	POINT screenPos;
	screenPos.x = static_cast<LONG>(centerX + screenX * 100.0f);  // Scale factor 100
	screenPos.y = static_cast<LONG>(centerY - screenY * 100.0f);  // Y-axis inverted for screen

	return screenPos;
}

UIPointFloat3 UIChart3D::ScreenToWorld(const POINT& screenPos, float depth) {
	// Convert screen coordinates back to world coordinates
	// This is the inverse of WorldToScreen

	RECT ctrlRC = GetAbsoluteRect();
	float centerX = (ctrlRC.left + ctrlRC.right) * 0.5f;
	float centerY = (ctrlRC.top + ctrlRC.bottom) * 0.5f;

	// Convert to normalized screen coordinates
	float normX = (screenPos.x - centerX) / 100.0f;  // Scale factor 100
	float normY = -(screenPos.y - centerY) / 100.0f; // Y-axis inverted

	// Apply inverse isometric projection (assuming depth)
	UIPointFloat3 worldCoord;
	worldCoord._x = normX + 0.5f * depth;
	worldCoord._y = normY + 0.5f * depth;
	worldCoord._z = depth;

	return worldCoord;
}

void UIChart3D::AddCurve(UIString curveName, const vector<UIPointFloat3>& points, UIColor color) {
	// Check if curve already exists
	for (auto& curve : _curveList) {
		if (curve._name == wstring(curveName)) {
			// Update existing curve
			curve._pointList = points;
			curve._color = color;
			return;
		}
	}

	// Add new curve
	CurveInfo newCurve;
	newCurve._name = wstring(curveName);
	newCurve._pointList = points;
	newCurve._color = color;
	newCurve._isShow = true;
	_curveList.push_back(newCurve);
}

void UIChart3D::AddCurve(UIString curveName, const vector<float>& xList, const vector<float>& yList, const vector<float>& zList, UIColor color) {
	if (xList.size() != yList.size() || yList.size() != zList.size()) {
		return; // Arrays must have same size
	}

	vector<UIPointFloat3> points;
	for (size_t i = 0; i < xList.size(); ++i) {
		points.push_back({ xList[i], yList[i], zList[i] });
	}

	AddCurve(curveName, points, color);
}

void UIChart3D::ClearAllCurves() {
	_curveList.clear();
}

void UIChart3D::CalcXYCoordRange() {
	if (_curveList.empty()) {
		return;
	}

	bool firstCurve = true;
	float xMin = 0.0f, xMax = 0.0f, yMin = 0.0f, yMax = 0.0f, zMin = 0.0f, zMax = 0.0f;

	for (const auto& curve : _curveList) {
		if (!curve._isShow || curve._pointList.empty()) {
			continue;
		}

		float curveXMin, curveXMax, curveYMin, curveYMax, curveZMin, curveZMax;
		if (const_cast<CurveInfo&>(curve).CalcCoordRange(curveXMin, curveXMax, curveYMin, curveYMax, curveZMin, curveZMax)) {
			if (firstCurve) {
				xMin = curveXMin; xMax = curveXMax;
				yMin = curveYMin; yMax = curveYMax;
				zMin = curveZMin; zMax = curveZMax;
				firstCurve = false;
			}
			else {
				if (curveXMin < xMin) xMin = curveXMin;
				if (curveXMax > xMax) xMax = curveXMax;
				if (curveYMin < yMin) yMin = curveYMin;
				if (curveYMax > yMax) yMax = curveYMax;
				if (curveZMin < zMin) zMin = curveZMin;
				if (curveZMax > zMax) zMax = curveZMax;
			}
		}
	}

	if (xMin == xMax) {
		xMax = xMin + 1.0f; // Avoid zero range
	}
	if (yMin == yMax) {
		yMax = yMin + 1.0f; // Avoid zero range
	}
	if (zMin == zMax) {
		zMax = zMin + 1.0f; // Avoid zero range
	}

	if (!firstCurve) {
		// Update data coordinate ranges
		_xDataRange = { xMin, xMax };
		_yDataRange = { yMin, yMax };
		_zDataRange = { zMin, zMax };
	}
}

void UIChart3D::DrawCurves() {
	for (const auto& curve : _curveList) {
		if (!curve._isShow || curve._pointList.size() < 2) {
			continue;
		}

		// Draw line segments between consecutive points
		for (size_t i = 0; i < curve._pointList.size() - 1; ++i) {
			UIPointFloat3 worldPoint1 = DataToWorld(curve._pointList[i]);
			UIPointFloat3 worldPoint2 = DataToWorld(curve._pointList[i + 1]);

			// Draw line segment
			UILine3D(worldPoint1, worldPoint2)(curve._color, &_cameraCtrl);
		}

		// Draw points (4 pixel radius)
		for (const auto& dataPoint : curve._pointList) {
			UIPointFloat3 worldPoint = DataToWorld(dataPoint);

			// Draw point as a small circle using UICircle3D
			//UICircle3D(worldPoint, 2.0f)(curve._color, &_cameraCtrl);
		}
	}
}