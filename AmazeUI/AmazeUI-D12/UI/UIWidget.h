#pragma once
#include "UIWindow.h"
#include "UIElement.h"
#include "UIDXFoundation.h"
#include "UIAnimation.h"

/*------------------------------------------------------- UIControlBase -------------------------------------------------------*/
/*

*/
class UIScrollBar;
template<class T>
class UIControlBase : public UIWindowBase {
public:
	bool HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
		T* pT = static_cast<T*>(this);

		bool isMsgHandled = DefHandleMessage(message, wParam, lParam);
		if (isMsgHandled) return true;

		switch (message) {
			case WM_CREATE: {
				pT->OnCreate();
			} break;
			case WM_DESTROY: {
				pT->OnDestroy();
			} break;
			case WM_SIZE: {
				pT->OnSize(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_MOUSEMOVE: {
				isMsgHandled = pT->OnMouseMove(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_MOUSELEAVE: {
				pT->OnMouseLeave(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_MOUSEWHEEL: {
				isMsgHandled = pT->OnMouseWheel((short)HIWORD(wParam));
			} break;
			case WM_LBUTTONDOWN: {
				isMsgHandled = pT->OnLButtonDown(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_LBUTTONUP: {
				isMsgHandled = pT->OnLButtonUp(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_RBUTTONDOWN: {
				isMsgHandled = pT->OnRButtonDown(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_RBUTTONUP: {
				isMsgHandled = pT->OnRButtonUp(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_LBUTTONDBLCLK: {
				isMsgHandled = pT->OnLButtonDbClk(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
			case WM_KEYDOWN: {
				pT->OnKeyDown((TCHAR)wParam);
			} break;
			case WM_CHAR: {
				pT->OnChar((TCHAR)wParam);
			} break;
			case WM_SETFOCUS: {
				pT->OnSetFocus();
			} break;
			case WM_KILLFOCUS: {
				pT->OnKillFocus();
			} break;
			case WM_COPY: {
				isMsgHandled = pT->OnCopy();
			} break;
			case WM_PASTE: {
				isMsgHandled = pT->OnPaste();
			} break;
			case WM_CUT: {
				isMsgHandled = pT->OnCut();
			} break;		
			case WM_HSCROLL: {
				pT->OnHscroll((int)wParam, (UIScrollBar*)lParam);
				isMsgHandled = true;
			} break;
			case WM_VSCROLL: {
				pT->OnVscroll((int)wParam, (UIScrollBar*)lParam);
				isMsgHandled = true;
			} break;
			case WM_NOTIFY: {
				pT->OnNotify((int)wParam, (LPARAM)lParam);
				isMsgHandled = true;
			} break;
		}

		return isMsgHandled;
	}

	bool CreateControl(UINT id, UIContainer* pUIContainer, const RECT& relativeRect = Shape2D::NULL_RECT, int layoutFlag = UILayoutCalc::NO_ZOOM, bool isShow = true, bool isOnHeap = false) {
		_id = id;
		return CreateWin(pUIContainer, relativeRect, layoutFlag, isShow, isOnHeap);
	}

	bool CreateControl(UINT id, UIWindowBase* pParent, const RECT& relativeRect = Shape2D::NULL_RECT, int layoutFlag = UILayoutCalc::NO_ZOOM, bool isShow = true, bool isOnHeap = false) {
		_id = id;
		return CreateWin(pParent, relativeRect, layoutFlag, isShow, isOnHeap);
	}

	UINT _id;

protected:
	// message processing
	void OnCreate()                                    { static_cast<T*>(this)->CalcArea(); }
	void OnDestroy()                                   {}
	void OnSize(POINT)								   { static_cast<T*>(this)->CalcArea(); }
	void OnMouseLeave(POINT)                           {}
	bool OnMouseMove(POINT)                            { return true; }
	bool OnMouseWheel(short)                    	   { return false; }
	bool OnLButtonDown(POINT)                          { return false; }
	bool OnLButtonUp(POINT)                            { return false; }
	bool OnLButtonDbClk(POINT)                         { return false; }
	bool OnRButtonDown(POINT)                          { return false; }
	bool OnRButtonUp(POINT)                            { return false; }
	void OnKeyDown(TCHAR)                              {}
	void OnChar(TCHAR)                                 {}
	void OnSetFocus()                                  {}
	void OnKillFocus()                                 {}
	bool OnCopy()                                      { return false; }
	bool OnPaste()                                     { return false; }
	bool OnCut()                                       { return false; }
	void OnHscroll(int, UIScrollBar*)  				   {}
	void OnVscroll(int, UIScrollBar*)  				   {}
	void OnNotify(int, LPARAM)                   	   {}

	// Internal utility functions
	void SendMessageToParent(UINT msg, WPARAM wParam, LPARAM lParam) {
		p_parentUIContainer->SendMessageToBindWin(msg, wParam, lParam);
	}
	void CalcArea() {}
};


/*------------------------------------------------------- UIWindow -------------------------------------------------------*/
template<class T>
class UIWindow : public UIWindowBase, public UIContainerHelp<UIWindow<T>> {
protected:
	// message processing
	virtual bool HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
		T* pT = static_cast<T*>(this);

		bool isMsgHandled = DefHandleMessage(message, wParam, lParam);
		if (isMsgHandled) {
			return true;
		}

		switch (message) {
			case WM_CREATE: {
				pT->OnCreate();
			} break;
			case WM_DESTROY: {
				pT->OnDestroy();
			} break;
			case WM_NOTIFY: {
				pT->OnNotify((int)wParam, (LPARAM)lParam);
			} break;
			case WM_SIZE: {
				pT->OnSize(Shape2D::CreatePoint()(LOWORD(lParam), HIWORD(lParam)));
			} break;
		};

		return isMsgHandled;
	}
	void OnCreate() {}
	void OnDestroy() {}
	void OnNotify(int, LPARAM) {}
	void OnSize(POINT) {}
};



/*------------------------------------------------------- UILayoutGrid -------------------------------------------------------*/
/*
UILayoutGrid provides similar grid layout calculation

row1: ctrl11 Interval1 ctrl12  Interval2 ctrl13 Interval3 ......
Interval_row
row2: ctrl21 Interval1 ctrl22  Interval2 ctrl23 Interval3 ......
Interval_row
......

*/
class UILayoutGrid {
public:
	// initialize
	void InitPoint(const POINT& relativePT);

	// set cell information
	void SetRowColumn(UINT row, UINT column, int width=100, int widthInterval=20, int height=30, int heightInterval=15);
	void SetCell(UINT bRow, UINT bColumn, UINT eRow, UINT eColumn, UIWindowBase* pWin);
	void SetCell(UINT row, UINT column, UIWindowBase* pWin);

	// set length and width and interval
	void SetColumnWidthInterval(UINT column, int width=100, int interval=20);
	void SetRowHeightInterval(UINT row, int heigth=30, int interval=20);

	// calculate position information
	void CalcGridPos();

private:
	struct CellInfo {
		UIWindowBase* p_win;
		int _endRow;
		int _endColumn;

		CellInfo();
	};

	typedef std::vector<CellInfo> ROW_TYPE;
	std::vector<ROW_TYPE> _grid;

	std::vector<int> _rowHeightList;
	std::vector<int> _columnWidthList;

	std::vector<int> _rowIntervalList;
	std::vector<int> _columnIntervalList;

	POINT _relativePoint;
};



/*------------------------------------------------------- UILable -------------------------------------------------------*/
class UILable : public UIControlBase<UILable> {
	friend UIControlBase;

public:
	UILable();
	~UILable() = default;

	void Draw();

	void SetText(UIString text, DWORD pos=UIFont::HLEFT_VCENTER);
	UIString GetText();
	void SetColor(UIColor& color);
	void SetFontHeight(float h);
	void SetPos(DWORD pos);

private:
	std::wstring _text;
	UIColor _color;
	float _fontHeight;
	DWORD _pos;
};

/*------------------------------------------------------- UIImageView -------------------------------------------------------*/
class UIImageView : public UIControlBase<UIImageView>, public UIAnimateEffectHitDrum {
	friend UIControlBase;

public:
	void Draw();
};

/*------------------------------------------------------- UIButton -------------------------------------------------------*/
class UIButton : public UIControlBase<UIButton>, public UIAnimateEffectHitDrum {
	friend UIControlBase;

public:
	UIButton();
	void Draw();
	void CalcArea();

	void SetText(UIString text);
	UIString GetText();

private:
	bool OnMouseMove(POINT pt);
	void OnMouseLeave(POINT pt);
	bool OnLButtonDown(POINT pt);
	bool OnLButtonUp(POINT pt);
	void OnKeyDown(TCHAR nChar);

	bool _isHover;
	bool _isLButtonDown;
	RECT _strRect;						// normal state display area
	RECT _strRect2;						// pressed state display area
	
	std::wstring _text;
};

/*------------------------------------------------------- UICheckButton -------------------------------------------------------*/
/*

*/
class UICheckButton : public UIControlBase<UICheckButton> {
	friend UIControlBase;

public:
	UICheckButton();
	void Draw();
	void CalcArea();

	void SetText(UIString text);
	UIString GetText();

	void SetCheck(bool f);
	bool GetCheck();

	void SetMutexList(std::vector<UICheckButton*>mutexList);

private:
	bool OnMouseMove(POINT pt);
	void OnMouseLeave(POINT pt);
	bool OnLButtonDown(POINT pt);

	RECT _checkRect;
	RECT _strRect;									// display area
	bool _isHover;									// mouse hover
	bool _isCheck;
	std::wstring _text;								// display wstring
	
	std::vector<UICheckButton*>_mutexList;			// mutex related
};
void UISetCheckButtonMutex(std::vector<UICheckButton*>& mutexList);
void UISetCheckButtonMutex(UICheckButton* but1, UICheckButton* but2);

/*------------------------------------------------------- UIEdit -------------------------------------------------------*/
/*
	void OnSetCursor(); has not been implemented
*/
class UIEdit : public UIControlBase<UIEdit> {
	friend UIControlBase;

public:
	UIEdit();
	void Draw();
	void CalcArea();

	void SetText(UIString text);
	UIString GetText();
	int GetTextToInt();
	float GetTextToFloat();
	double GetTextToDouble();

	void SelectAllText();

private:
	void OnDestroy();
	void OnSetFocus();
	void OnKillFocus();
	bool OnLButtonDown(POINT pt);
	bool OnLButtonDbClk(POINT pt);
	bool OnMouseMove(POINT pt);
	void OnKeyDown(TCHAR nChar);
	void OnChar(TCHAR nChar);
	bool OnPaste();
	bool OnCopy();
	bool OnCut();

	void EraseSelectArea();
	void CalcCaretPassChar(POINT& pt);
	void CalcBeginDrawXPos();

	float _fontHeight;					// font related
	UIColor _fontColor;

	RECT _drawRectAllow;				// drawing area allow drawing wstring
	RECT _drawRectReal;					// drawing wstring actual drawing area may exceed the allowed drawing area

	std::wstring _text;					// drawing wstring
	int _beginDrawXPos;					// drawing start x coordinate

	bool _isCNInput;					// Chinese input method
	
	size_t _caretPassChar;				// number of characters skipped by the insertion cursor
	bool _isDrawCaretImmd;

	int _isSelArea;						// 0:NoSelected 1:Ready to select 2: Selected
	size_t _beginAreaPassChar;			// number of characters skipped by the beginning of the selection area
};

/*------------------------------------------------------- UISelectList -------------------------------------------------------*/
/*
	UISelectList only supports popup creation
*/
class UISelectList : public UIControlBase<UISelectList>, public UIAnimateHelp {
	friend UIControlBase;

public:
	UISelectList();
	void Draw();

	void AddText(UIString text);
	std::optional<UIString> GetText(int index);
	void ClearList();
	size_t GetListCount();

private:
	bool OnMouseMove(POINT pt);
	void OnMouseLeave(POINT pt);
	bool OnLButtonDown(POINT pt);

	std::vector<std::wstring> _list;
	int _selectedectIndex;

	bool _isHover;
	int _hoverIndex;
};

/*------------------------------------------------------- UIComboBox -------------------------------------------------------*/
/*
	UIComboBox contains UISelectList
*/
class UIComboBox : public UIControlBase<UIComboBox>, public UIContainerHelp<UIComboBox> {
	friend UIControlBase;

public:
	UIComboBox();
	void Draw();

	void AddText(UIString text);
	UIString GetText();
	void SetSelectIndex(int index);
	void ClearList();
	int GetSelectIndex();

	void IsDrawBoader(bool flag);				// whether to draw the border

private:
	void OnCreate();
	void OnKillFocus();
	bool OnMouseMove(POINT pt);
	void OnMouseLeave(POINT pt);
	bool OnLButtonDbClk(POINT pt);
	bool OnLButtonDown(POINT pt);
	void OnNotify(int id, LPARAM param);

	void SetDropDown(bool flag);

	UISelectList _dropDownList;
	int _selectedectIndex;

	bool _isDropDown;
	bool _isHover;

	bool _isDrawBoader;
};

/*------------------------------------------------------- UIScrollBar -------------------------------------------------------*/
/*
	
*/
class UIScrollBar : public UIControlBase<UIScrollBar> {
	friend UIControlBase;

public:
	UIScrollBar(int coordFlag=0);
	void Draw();
	void CalcArea();
	
	void SetPageScale(double s);
	double GetPageScale();
	void SetPosScale(double s);
	double GetPosScale();
	void SetCoordXY(int coordFlag);

private:
	bool OnMouseMove(POINT pt);
	bool OnLButtonDown(POINT pt);
	bool OnLButtonDbClk(POINT pt);

	void CalcBarArea();

	int _coordFlag;						// 0: x axis  1: y axis
	double _pageScale;					// scroll length ratio to the middle area
	double _posScale;					// scroll start position ratio to the middle area

	bool _isHover;						// mouse hover
	POINT _prePoint;					// record the previous point
	
	// area information
	RECT _scrollRect;
	RECT _barRect;					
};

/*------------------------------------------------------- UIGrid -------------------------------------------------------*/
// notification information
struct NM_GRID {
	int _code;			// 1: VK_RETURN  2: selected row  3: selected column  4: selected cell
	int _row;
	int _column;
};

class UIGrid : public UIControlBase<UIGrid>, public UIContainerHelp<UIGrid> {
	friend UIControlBase;

	// Grid_Cell information
	struct GridCellInfo {
		GridCellInfo(std::wstring str);
		
		// create cell control  release control
		void CreateCellControl(UIContainer *pUIContainer, int firstRowPos, int firstColumnPos, int id=0);
		bool DeleteCellControl();
		void MoveCellControl(const RECT& rc);
		void DrawCellControl();

		RECT _pos;							// cell position	

		std::wstring _text;					// wstring content
		//int _wordPos;						// wstring position left, center, right
		UIColor _color;

		// cell control information
		enum {
			CTRL_EDIT = 0,
			CTRL_CHECKBUTTON,
			CTRL_COMBOBOX,
			CTRL_BUTTON
		};
		int _controlType;					// control type
		UIWindowBase* _pCtrl;				// UI control
		bool _isHold;						// whether to maintain
	};

public:
	UIGrid();
	void Draw();
	void CalcArea();

	// set and get row and column number
	void SetRowColumn(UINT rowNum, UINT columnNum);
	void GetRowColumn(UINT& rowNum, UINT& columnNum);

	// row height and column width
	void SetRowHeight(UINT row, int height);
	void SetColumnWidth(UINT column, int width);

	// set and get row and column fix
	void SetRowFix();
	void SetColumnFix();

	// content setting
	// set and get cell string
	void SetCellText(UINT row, UINT column, UIString text, bool isAutoWidth=false);
	std::optional<UIString> GetCellText(UINT row, UINT column);
	void SetCellColor(UINT row, UINT column, UIColor& color);
	void SetCellFontHeight(float h);
	// add one or more rows
	void AddRow(std::vector<UIString>& row);						
	void AddRows(std::vector<std::vector<UIString>>& rowList);
	// clear all rows or all cell contents except fixed rows
	void ClearUnfixRows();									
	void ClearAllCells();
	// set cell information in the area
	void SetAreaCells(UINT row, UINT column, std::vector<std::vector<UIString>>& rowList);

	// set cell control
	// set cell button control
	void SetCellButton(int id, UINT row, UINT column, UIString str);
	// set cell checkButton control
	void SetCellCheckButton(int id, UINT row, UINT column, UIString str);
	bool GetCellCheckState(UINT row, UINT column, bool& checkState);
	void SetCellCheckState(UINT row, UINT column, bool checkState);
	// set cell comboBox control
	void SetCellComboBox(int id, UINT row, UINT column);
	void AddCellComboBoxText(UINT row, UINT column, UIString text);

	// get all selected information
	bool GetSelectCell(UINT& row, UINT& column);
	bool GetSelectCells(UINT& beginRow, UINT& beginColumn, UINT& endRow, UINT& endColumn);
	bool GetSelectRows(std::vector<UINT>& selectList);
	bool GetSelectColumns(std::vector<UINT>& selectList);

	UIScrollBar _xScroll;									// scroll bar
	UIScrollBar _yScroll;
	bool _isXScrollShow;									// scroll bar show flag
	bool _isYScrollShow;

private:
	void OnCreate();
	bool OnLButtonDown(POINT pt);
	bool OnLButtonDownUnfix(POINT point);
	void OnLButtonDownFix(POINT point);
	bool OnLButtonDbClk(POINT pt);
	void OnHscroll(int code, UIScrollBar* pScrollBar);
	void OnVscroll(int code, UIScrollBar* pScrollBar);
	bool OnMouseWheel(short zDelta);
	bool OnMouseMove(POINT pt);
	void OnKeyDown(TCHAR nChar);
	//
	void OnKeyDownArrows(TCHAR nChar);
	void OnKeyDownProcessKey();
	void OnKeyDownBack();
	void OnKeyDownDelete();
	void OnKeyDownReturn();
	//
	void OnChar(TCHAR nChar);
	bool OnPaste();
	bool OnCopy();
	bool OnCut();
	void OnNotify(int id, LPARAM param);

	// internal drawing function
	void DrawScrollBar();
	void DrawSelected();
	void DrawSelectedALL(UIColor& selectColor);
	void DrawSelectedCELL();
	void DrawSelectedCELLS(UIColor& selectColor);
	void DrawSelectedROW(UIColor& selectColor);
	void DrawSelectedCOLUMN(UIColor& selectColor);
	void DrawGrid();
	void DrawCells();
	// internal calculation function
	void CalcNoScrollRect();
	void CalcGridRect();
	void CalcScrollBarRect();
	void CalcDrawBeginRowColumn(UINT& beginRow, UINT& beginColumn);
	void CalcCellsPos();
	bool CalcCellIndexUnfix(UINT& row, UINT& column, POINT point);				// calculate the cell in the unFixGrid range
	inline void CalcCellsRange(UINT& beginRow, UINT& endRow, UINT& beginColumn, UINT& endColumn);
	//	
	void SetNoSel();

	float _fontHeight;

	enum {
		SELECTION_NONE = 0,
		SELECTION_CELL,
		SELECTION_CELLS,
		SELECTION_ROW,
		SELECTION_COLUMN,
		SELECTION_ALL
	};
	int _selectedInfo;											// selected information
	UINT _selectedRowBegin, _selectedColumnBegin;				// selected cell
	UINT _selectedRowEnd, _selectedColumnEnd;
	std::vector<UINT> _selectedRowList, _selectedColumnList;	// selected row or column
	bool _allRowColumnActive;									// selected all row and column flag

	// drawing area
	RECT _noScrollArea;											// area without scroll bar
	RECT _gridArea;												// grid area in client area
	RECT _unfixGridArea;										// unfix area in grid area
	RECT _xScrollBarArea, _yScrollBarArea;						// scroll bar area

	bool _isDraw;
	
	UINT _rowNum;												// row and column number
	UINT _columnNum;
	int _firstRowPos;											// first row and column position
	int _firstColumnPos;
	bool _isFirstRowFix;										// first row and column fix flag
	bool _isFirstColumnFix;
	std::vector<int> _rowHeightList;							// row height and column width
	std::vector<int> _columnWidthList;
	int _heightSum;												// all row height
	int _widthSum;												// all column width	
	UINT _beginDrawRow, _beginDrawColumn;						// begin row and column

	// cell matrix
	typedef std::vector<std::vector<GridCellInfo> >_cellArrayType;
	_cellArrayType _cellArray;

	// parent window notification message
	NM_GRID _nmGrid;

	DirectX::XMMATRIX _inheritedTransformMatrix;
};

/*------------------------------------------------------- UIChart -------------------------------------------------------*/
class UIChart : public UIControlBase<UIChart> {
	friend UIControlBase;

	// type definition
	typedef std::pair<double, double> RANGE_DOUBLE;
	typedef std::vector<RANGE_DOUBLE> VECTOR_RANGE; 

	struct PointDouble {
		double _x;
		double _y;

		PointDouble(double x=0.0, double y=0.0) : _x(x), _y(y) {}
	};
	typedef std::vector<PointDouble> VECTOR_POINT; 

	struct CurveInfo {	
		CurveInfo();

		bool CalcCoordRange(double& xMin, double& xMax, double& yMin, double& yMax);		// calculate the coordinate range of the line
		bool JudgePointNearLine(POINT& p0, POINT& p1, POINT& p2);							// judge whether the point p0 is near the line (p1,p2)
		bool operator==(std::wstring textName);												// overload the comparison  name as key

		VECTOR_POINT _pointList;
		bool _isXCoordInOrder;																// flag to indicate whether the x-axis data is ordered (from small to large)
		int _beginPointIndex, _endPointIndex;												// draw the interval [beginPointIndex, endPointIndex]

		std::wstring _name;																	// line name, used as key
		UIColor _color;																		// line color
	
		bool _isSelected;																	// selected flag
		bool _isShow;																		// show flag
		bool _isLine;																		// line flag
	};
	typedef std::list<CurveInfo> CURVE_LIST;

public:
	UIChart();
	void CalcArea();
	void Draw();

	// add curve relative to the left axis
	void AddCurve1(UIString textName);
	void AddCurve1(UIString textName, double xValue, double yValue);
	void AddCurve1(UIString textName, std::vector<double>& xList, std::vector<double>& yList, bool isXCoordInOrder=true);
	// add curve relative to the right axis
	void AddCurve2(UIString textName);
	void AddCurve2(UIString textName, double xValue, double yValue);
	void AddCurve2(UIString textName, std::vector<double>& xList, std::vector<double>& yList, bool isXCoordInOrder=true);
	// clear all curves
	void Clear();

	// set curve color
	void SetCurve1Color(UIString textName, UIColor color);
	void SetCurve2Color(UIString textName, UIColor color);

	// set curve selected state
	void SetCurve1Select(UIString textName);
	void SetCurve2Select(UIString textName);

	// set and get curve range
	void SetXYCoordRange(double xMin, double xMax, double y1Min, double y1Max, double y2Min, double y2Max);
	void GetXYCoordRange(double& xMin, double& xMax, double& y1Min, double& y1Max, double& y2Min, double& y2Max);

	// set coordinate limit
	void SetXCoordLimit(double min, double max);
	void SetY1CoordLimit(double min, double max);
	void SetY2CoordLimit(double min, double max);

	// set coordinate symmetry
	void SetXCoordSymmetry();
	void SetY1CoordSymmetry();
	void SetY2CoordSymmetry();

	// calculate coordinate range
	void CalcXYCoordRange();

private:
	bool OnLButtonDown(POINT pt);
	bool OnLButtonUp(POINT pt);
	bool OnRButtonDown(POINT pt);
	bool OnRButtonUp(POINT pt);
	bool OnMouseMove(POINT pt);
	//
	void AddCurve(int yFlag, std::wstring textName);
	void AddCurve(int yFlag, std::wstring textName, double xValue, double yValue);
	void AddCurve(int yFlag, std::wstring textName, std::vector<double>& xList, std::vector<double>& yList, bool isXCoordInOrder=true);
	void SetCurveColor(int yFlag, std::wstring textName, UIColor& color);
	void SetCurveSelect(int yFlag, std::wstring textName);

	void DrawGrid();
	void DrawXCoordLable();
	void DrawY1CoordLable();
	void DrawY2CoordLable();
	void DrawXCoord();
	void DrawY1Coord();
	void DrawY2Coord();
	void DrawZoomRect();
	void DrawMousePosAndToolTip();
	void DrawCurveList1();
	void DrawCurveList2();
	void DrawCurve(CurveInfo& curve, int yFlag);
	void Draw2DPoint(POINT& pointPos, UIColor& color, bool bigPointFlag);

	// coordinate range related
	void SaveCurCoordRange();														// save current coordinate range
	void CalcCurveListDrawRange(int mode = LEFTMOVE);
	void CalcCurveDrawRange(CurveInfo& curve, int mode = LEFTMOVE);
	void CalcCoordSymmetry(RANGE_DOUBLE& rangeCoord);								// calculate coordinate symmetry

	// coordinate value and real position conversion
	void TransfromCoordToPos(POINT& pointPos, PointDouble& pointCoord, int yFlag);
	bool TransfromPosToCoord(POINT& pointPos, PointDouble& pointCoord, int yFlag);

	// point range judgment
	bool IsPosPointInbox(POINT& pointPos, const RECT& rect);
	bool IsCoordPointInbox(PointDouble& pointCoord, PointDouble& p1, PointDouble& p2, bool nearYFlag=false);
	bool IsCoordPointInCoordRange(PointDouble& pointCoord, int yFlag);

	// judge whether the point is near the curve
	bool IsPointNearCurve(POINT& point, int yFlag);
	bool IsPointNearCurvePoint(POINT& point, CurveInfo& curve, int yFlag);
	bool IsPointNearCurveLine(POINT& point, CurveInfo& curve, int yFlag);

	// calculate intersection point
	PointDouble CalcIntersectionPoint(PointDouble& prePoint, PointDouble& curPoint, int outBoxIndex, int yFlag);
	inline bool CalcLeftIntersectionPoint(PointDouble& pointCoord, double& k, double& b, int yFlag);
	inline bool CalcTopIntersectionPoint(PointDouble& pointCoord, double& k, double& b, int yFlag);
	inline bool CalcRightIntersectionPoint(PointDouble& pointCoord, double& k, double& b, int yFlag);
	inline bool CalcBottomIntersectionPoint(PointDouble& pointCoord, double& k, double& b, int yFlag);

	// internal calculation mode
	enum CalcModeInside {
		NOTUSELASTDATA = 0,
		LEFTMOVE,
		RIGHTMOVE,
		ZOOM,
		SHRINK,
		ADDPOINT
	};
		
	RECT _gridRect;															// area information
	
	UINT _rowNum;															// grid row and column number
	UINT _columnNum;

	CURVE_LIST _curveList1;													// curve list left y-axis
	CURVE_LIST _curveList2;													// curve list right y-axis
	
	double _coordToPosScaleX;												// ratio: coordinate/positon, for efficiency
	double _coordToPosScaleY1;												
	double _coordToPosScaleY2;

	RANGE_DOUBLE _xCoordRange, _y1CoordRange, _y2CoordRange;				// record x y-axis coordinate range
	VECTOR_RANGE _xCoordHistory, _y1CoordHistory, _y2CoordHistory;			// record history coordinate range
	bool _isY1CoordRangeCalc;												// xy coordinate range calculation flag
	bool _isY2CoordRangeCalc;												// xy coordinate range calculation flag
	
	bool _isDrawZoomRect;													// zoom frame information
	POINT _lPointBeginPos, _lPointEndPos;
	POINT _rPointPosUndo;
	
	bool _isMove;															// move information
	POINT _rPointPosMove;
	
	bool _isShowToolTip;													// tooltip information
	std::wstring _tooltipStr;
	
	std::wstring _mouseCoordStr;											// coordinate display
	bool _isRoomEnoughDraw;													// space enough to draw flag

	bool _isXCoordLimit, _isY1CoordLimit, _isY2CoordLimit;					// external calculation condition
	double _xCoordMinLimit, _xCoordMaxLimit;
	double _y1CoordMinLimit, _y1CoordMaxLimit;
	double _y2CoordMinLimit, _y2CoordMaxLimit;
	bool _isXCoordSymmetry, _isY1CoordSymmetry, _isY2CoordSymmetry;

	DirectX::XMMATRIX _inheritedTransformMatrix;
};

/*------------------------------------------------------- UITabX -------------------------------------------------------*/
class UITab : public UIControlBase<UITab>, public UIContainerHelp<UITab>, public UIAnimateHelp {
	friend UIControlBase;

	/*--------------------------------- control data ---------------------------------*/
public:
	UITab();

	void CalcArea();
	void Draw();

	void SetCellNum(UINT num);
	void SetCell(UINT index, UIString title, UIWindowBase* pWin);
	void SetCurCell(UINT index);
	void SetX();
	void SetY();

	int GetCurCell();

private:
	bool OnMouseMove(POINT pt);
	void OnMouseLeave(POINT pt);
	bool OnLButtonDown(POINT pt);

	void DrawTab();
	void DrawTabSelLine();
	void DrawAnimate1();
	void DrawAnimate1Cell();
	void DrawAnimate1Tab();

	void CalcCellListRect();
	void CalcTabRect();
	void CalcTabSelLine();

	enum {
		X_FLAG = 0,
		Y_FLAG = 1
	} _xyFlag;									// direction flag
	enum {
		TAB_1 = 30,
		TAB_2 = 3
	};											// width value
	enum {
		MAX_FRAME1 = 8
	};											// animation frame number

	bool _isDraw;

	// tab related
	RECT _tabRC;
	std::vector<RECT> _tabRCList;
	int _hoverIndex;
	//
	RECT _lineRC;

	// cellList
	struct CellData {
		std::wstring _title;
		UIWindowBase* p_win;

		CellData() : _title(L""), p_win(NULL) {}
	};
	std::vector<CellData> _cellList;
	std::vector<RECT> _cellRCList;
	RECT _cellRC;
	//
	UINT _selectedIndex;

	DirectX::XMMATRIX _inheritedTransformMatrix;
};

/*------------------------------------------------------- UICanvas -------------------------------------------------------*/
// canvas drawing board   message will be transmitted directly
class UICanvas : public UIWindowBase, public UIContainerHelp<UICanvas> {
public:
	UICanvas();
};

class UIColorBlock  : public UIWindowBase {
public:
	void Draw();
};




/*------------------------------------------------------- UIImageView -------------------------------------------------------*/
class UIImage3D : public UIControlBase<UIImage3D> {
	friend UIControlBase;

public:
	UIImage3D();

	void Draw();

private:
	bool OnMouseMove(POINT pt);
	bool OnMouseLeave(POINT pt);

	POINT _curMousePos;
	bool _isHover;
};
