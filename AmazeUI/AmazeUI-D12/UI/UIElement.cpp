#include "UIElement.h"
#include "UIDXFoundation.h"

using namespace std;
using namespace DirectX;
using namespace SimpleMath;
using namespace UIShape2D;

UIScreenClipRectGuard::UIScreenClipRectGuard(const RECT& clipRC, bool execute) {
	_execute = execute;
	UIDXFoundation::GetSingletonInstance()->BeginScreenClipRect(clipRC, _execute);
}

UIScreenClipRectGuard::~UIScreenClipRectGuard() {
	UIDXFoundation::GetSingletonInstance()->EndScreenClipRect(_execute);
}

UIPoint::UIPoint(LONG x, LONG y, float z, int renderLevel) {
	_point = XMFLOAT2((float)x, (float)y);
	_z = z;
	_renderLevel = renderLevel;
}

void UIPoint::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DPoint(_point, _z, color, 4, _renderLevel, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DPoint(_point, _z, color, 4, _renderLevel);
	}
}

UIPoints::UIPoints(const vector<POINT>& points, float z, int renderLevel) {
	_points.resize(points.size());
	for (size_t i = 0; i < points.size(); i++) {
		_points[i] = XMFLOAT2((float)points[i].x, (float)points[i].y);
	}
	_z = z;
	_renderLevel = renderLevel;
}

void UIPoints::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {	
		UIDXFoundation::GetSingletonInstance()->Draw3DPoints(_points, _z, color, 4.f, _renderLevel, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DPoints(_points, _z, color, 4.f, _renderLevel);
	}
}

UILine::UILine(LONG beginX, LONG beginY, LONG endX, LONG endY, float z, float width, int renderLevel) {
	_start = XMFLOAT2((float)beginX, (float)beginY);
	_end = XMFLOAT2((float)endX, (float)endY);
	_z = z;
	_width = width;
	_renderLevel = renderLevel;
}

void UILine::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DLine(_start, _end, _z, color, _width, _renderLevel, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DLine(_start, _end, _z, color, _width, _renderLevel);
	}
}

UIRect::UIRect(LONG beginX, LONG beginY, LONG endX, LONG endY, float z, int renderLevel) {
	_start = XMFLOAT2((float)beginX, (float)beginY);
	_end = XMFLOAT2((float)endX, (float)endY);
	_z = z;
	_renderLevel = renderLevel;
}

UIRect::UIRect(const RECT& rect, float z, int renderLevel) {
	_start = XMFLOAT2((float)rect.left, (float)rect.top);
	_end = XMFLOAT2((float)(rect.right), (float)(rect.bottom));
	_z = z;
	_renderLevel = renderLevel;
}

void UIRect::operator()(const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DRectOutline(_start, _end, _z, color, 1.f, _renderLevel, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DRectOutline(_start, _end, _z, color, 1.f, _renderLevel);
	}
}

void UIRect::operator()(const UIColor& color, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DRectSolid(_start, _end, _z, color, alpha, _renderLevel, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DRectSolid(_start, _end, _z, color, alpha, _renderLevel);
	}
}

void UIRect::operator()(const UIColor& colorLT, const UIColor& colorRT, const UIColor& colorLB, const UIColor& colorRB, UCHAR alpha, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DRectSolid(_start, _end, _z, colorLT, colorRT, colorLB, colorRB, alpha, _renderLevel, transformMatrix);
	} else {
		UIDXFoundation::GetSingletonInstance()->Draw2DRectSolid(_start, _end, _z, colorLT, colorRT, colorLB, colorRB, alpha, _renderLevel);
	}
}

UIImage::UIImage() {
	_sourceFlag = 0;
}

UIImage::UIImage(std::wstring imagePath, const UIColor& colorKey, float z, int renderLevel) {
	_sourceFlag = 1;
	_path = imagePath;
	_z = z;
	_colorKey = colorKey;
	_renderLevel = renderLevel;
}

UIImage::UIImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, float z, int renderLevel) {
	_sourceFlag = 2;
	_path = resDLLPath;
	_id = id;
	_z = z;
	_colorKey = colorKey;
	_renderLevel = renderLevel;
}

void UIImage::operator()(const RECT& srcRect, const RECT& dstRect, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		if (_sourceFlag == 1)	{
			UIDXFoundation::GetSingletonInstance()->Draw3DImage(_path, _colorKey, srcRect,
																XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
																XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
																_z, alphy, _renderLevel, transformMatrix);
		} else if (_sourceFlag == 2) {
			UIDXFoundation::GetSingletonInstance()->Draw3DImage(_path, _id, _colorKey, srcRect,
																XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
																XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
																_z, alphy, _renderLevel, transformMatrix);
		}
	} else {
		if (_sourceFlag == 1)	{
			UIDXFoundation::GetSingletonInstance()->Draw2DImage(_path, _colorKey, srcRect,
																XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
																XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
																_z, alphy, _renderLevel);
		} else if (_sourceFlag == 2) {
			UIDXFoundation::GetSingletonInstance()->Draw2DImage(_path, _id, _colorKey, srcRect,
																XMFLOAT2{static_cast<float>(dstRect.left), static_cast<float>(dstRect.top)}, 
																XMFLOAT2{static_cast<float>(dstRect.right), static_cast<float>(dstRect.bottom)}, 
																_z, alphy, _renderLevel);
		}
	}
}

void UIImage::operator()(const RECT& srcRect, LONG dstBeginX, LONG dstBeginY, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {	
	RECT dstRect = {dstBeginX, dstBeginY, 0, 0};
	this->operator()(srcRect, dstRect, alphy, transformMatrix);
}

void UIImage::operator()(const RECT& dstRect, float scale, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	this->operator()(NULL_RECT, scale == 1 ? dstRect : ScaleRect()(dstRect, scale), alphy, transformMatrix);
}


void UIImage::operator()(LONG dstCenterX, LONG dstCenterY, float scale, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	RECT textureRect;
	if (!GetSize(textureRect)) {
		return;
	}

	RECT dstRect = CreateRect()(UIShape2D::CreatePoint()((LONG)(dstCenterX - (textureRect.right * scale / 2)), (LONG)(dstCenterY - (textureRect.bottom * scale / 2))), 
										 CreateSize()((LONG)((textureRect.right - textureRect.left) * scale), (LONG)((textureRect.bottom - textureRect.top) * scale)));
	this->operator()(NULL_RECT, dstRect, alphy, transformMatrix);
}


bool UIImage::GetSize(RECT& textureRect) {
	if (_sourceFlag == 1) {
		return UIDXFoundation::GetSingletonInstance()->Get2DImageSize(_path, _colorKey, textureRect);
	} else if (_sourceFlag == 2) {
		return UIDXFoundation::GetSingletonInstance()->Get2DImageSize(_path, _id, _colorKey, textureRect);
	}
	return false;
}

UISlicedImage::UISlicedImage() {
	_image._sourceFlag = 0;
}

UISlicedImage::UISlicedImage(std::wstring imagePath, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z, int renderLevel) {
	_image = UIImage(imagePath, colorKey, z, renderLevel);

	_topBarHeight = topBarHeight;
	_bottomBarHeight = bottomBarHeight;
	_leftBarWidth = leftBarWidth;
	_rightBarWidth = rightBarWidth;
	_renderLevel = renderLevel;
}

UISlicedImage::UISlicedImage(std::wstring resDLLPath, UINT id, const UIColor& colorKey, int topBarHeight, int bottomBarHeight, int leftBarWidth, int rightBarWidth, float z, int renderLevel) {
	_image = UIImage(resDLLPath, id, colorKey, z, renderLevel);

	_topBarHeight = topBarHeight;
	_bottomBarHeight = bottomBarHeight;
	_leftBarWidth = leftBarWidth;
	_rightBarWidth = rightBarWidth;
	_renderLevel = renderLevel;
}

void UISlicedImage::operator()(LONG dstBeginX, LONG dstBeginY, LONG width, LONG height, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	RECT textureRect;
	if (!_image.GetSize(textureRect)) {
		return;
	}
	UINT imageWidth = GetRectWidth()(textureRect);
	UINT imageHeight = GetRectHeight()(textureRect);

	RECT rc1 = CreateRect()(UIShape2D::CreatePoint()(0, 0), CreateSize()(_leftBarWidth, _topBarHeight));
	_image(rc1, dstBeginX, dstBeginY, alphy, transformMatrix);

	RECT rc3 = CreateRect()(UIShape2D::CreatePoint()(imageWidth - _rightBarWidth , 0), CreateSize()(_rightBarWidth, _topBarHeight));
	_image(rc3, dstBeginX + width - _rightBarWidth, dstBeginY, alphy, transformMatrix);

	RECT rc4 = CreateRect()(UIShape2D::CreatePoint()(0, imageHeight - _bottomBarHeight), CreateSize()(_leftBarWidth, _bottomBarHeight));
	_image(rc4, dstBeginX, dstBeginY + height - _bottomBarHeight, alphy, transformMatrix);

	RECT rc6 = CreateRect()(UIShape2D::CreatePoint()(imageWidth - _rightBarWidth, imageHeight - _bottomBarHeight), CreateSize()(_rightBarWidth, _bottomBarHeight));
	_image(rc6, dstBeginX + width - _rightBarWidth, dstBeginY + height - _bottomBarHeight, alphy, transformMatrix);

	RECT rc2 = CreateRect()(UIShape2D::CreatePoint()(_leftBarWidth, 0) , CreateSize()(imageWidth - _leftBarWidth - _rightBarWidth, _topBarHeight));
	RECT rc2d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + _leftBarWidth, dstBeginY), CreateSize()(width - _leftBarWidth - _rightBarWidth, _topBarHeight));
	_image(rc2, rc2d, alphy, transformMatrix);

	RECT rc5 = CreateRect()(UIShape2D::CreatePoint()(_leftBarWidth, imageHeight - _bottomBarHeight), CreateSize()(imageWidth - _leftBarWidth - _rightBarWidth, _bottomBarHeight));
	RECT rc5d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + _leftBarWidth, dstBeginY + height - _bottomBarHeight), CreateSize()(width - _leftBarWidth - _rightBarWidth, _bottomBarHeight));
	_image(rc5, rc5d, alphy, transformMatrix);

	RECT rc7 = CreateRect()(UIShape2D::CreatePoint()(0, _topBarHeight), CreateSize()(_leftBarWidth, imageHeight - _topBarHeight - _bottomBarHeight));
	RECT rc7d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX, dstBeginY + _topBarHeight), CreateSize()(_leftBarWidth, height - _topBarHeight - _bottomBarHeight));
	_image(rc7, rc7d, alphy, transformMatrix);

	RECT rc8 = CreateRect()(UIShape2D::CreatePoint()(imageWidth - _rightBarWidth, _topBarHeight), CreateSize()(_rightBarWidth, imageHeight - _topBarHeight - _bottomBarHeight));
	RECT rc8d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + width - _rightBarWidth, dstBeginY + _topBarHeight), CreateSize()(_rightBarWidth, height - _topBarHeight - _bottomBarHeight));
	_image(rc8, rc8d, alphy, transformMatrix);
	
	RECT rc9 = CreateRect()(UIShape2D::CreatePoint()(_leftBarWidth, _topBarHeight), CreateSize()(imageWidth - _leftBarWidth - _rightBarWidth, imageHeight - _topBarHeight - _bottomBarHeight));
	RECT rc9d = CreateRect()(UIShape2D::CreatePoint()(dstBeginX + _leftBarWidth, dstBeginY + _topBarHeight), CreateSize()(width - _leftBarWidth - _rightBarWidth, height - _topBarHeight - _bottomBarHeight));
	_image(rc9, rc9d, alphy, transformMatrix);
}

void UISlicedImage::operator()(const RECT& dstRC, UCHAR alphy, const DirectX::XMMATRIX& transformMatrix) {
	this->operator()(dstRC.left, dstRC.top, dstRC.right - dstRC.left, dstRC.bottom - dstRC.top, alphy, transformMatrix);
}

UIFont::UIFont(float z, float fontSize, int renderLevel) {
	_z = z;
	_fontSize = fontSize;
	_renderLevel = renderLevel;
}

void UIFont::operator()(std::wstring text, const POINT& position, const UIColor& color, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextFT(text, XMFLOAT2{ (float)position.x, (float)position.y }, _z - 0.005f, color, _fontSize, _renderLevel, transformMatrix);
	}
	else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextFT(text, XMFLOAT2{ (float)position.x, (float)position.y }, _z, color, _fontSize, _renderLevel);
	}
}

void UIFont::operator()(std::wstring text, const RECT& rc, const UIColor& color, UIFontPos posFlag, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextFT(text, rc, posFlag, _z - 0.005f, color, _fontSize, _renderLevel, transformMatrix);
	}
	else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextFT(text, rc, posFlag, _z, color, _fontSize, _renderLevel);
	}
}

void UIFont::operator()(std::wstring text, const POINT& position, const UIColor& color, float lineSpacing, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextMultiLineFT(text, XMFLOAT2{ (float)position.x, (float)position.y }, _z - 0.005f, color, _fontSize, lineSpacing, _renderLevel, transformMatrix);
	}
	else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextMultiLineFT(text, XMFLOAT2{ (float)position.x, (float)position.y }, _z, color, _fontSize, lineSpacing, _renderLevel);
	}
}

void UIFont::operator()(std::wstring text, const RECT& rc, const UIColor& color, UIFontPos posFlag, float lineSpacing, const DirectX::XMMATRIX& transformMatrix) {
	if (!XMMatrixIsIdentity(transformMatrix)) {
		UIDXFoundation::GetSingletonInstance()->Draw3DTextMultiLineFT(text, rc, posFlag, _z - 0.005f, color, _fontSize, lineSpacing, _renderLevel, transformMatrix);
	}
	else {
		UIDXFoundation::GetSingletonInstance()->Draw2DTextMultiLineFT(text, rc, posFlag, _z, color, _fontSize, lineSpacing, _renderLevel);
	}
}

SIZE UIFont::GetDrawAreaSize(std::wstring text) {
	return UIDXFoundation::GetSingletonInstance()->GetTextSizeFT(text, _fontSize);
}



// UIPoint3D implementation
UIPoint3D::UIPoint3D(float x, float y, float z, int renderLevel) : _point(x, y, z), _renderLevel(renderLevel) {}

void UIPoint3D::operator()(const UIColor& color, UICameraBase3D* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldPoint(_point, color, _renderLevel, pCamera);
}

// UIPoints3D implementation
UIPoints3D::UIPoints3D(const std::vector<UIPointFloat3>& points, int renderLevel) : _renderLevel(renderLevel) {
	_points.reserve(points.size());
	for (const auto& point : points) {
		_points.emplace_back(point._x, point._y, point._z);
	}
}

void UIPoints3D::operator()(const UIColor& color, UICameraBase3D* pCamera) {
	for (const auto& point : _points) {
		UIDXFoundation::GetSingletonInstance()->Draw3DWorldPoint(point, color, _renderLevel, pCamera);
	}
}

// UILine3D implementation
UILine3D::UILine3D(UIPointFloat3 start, UIPointFloat3 end, float width, int renderLevel) 
	: _start(start._x, start._y, start._z), _end(end._x, end._y, end._z), _width(width), _renderLevel(renderLevel) {}

void UILine3D::operator()(const UIColor& colorS, const UIColor& colorE, UICameraBase3D* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldLine(_start, _end, colorS, colorE, _width, _renderLevel, pCamera);
}

void UILine3D::operator()(const UIColor& color, UICameraBase3D* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldLine(_start, _end, color, _width, _renderLevel, pCamera);
}

// UICircle3D implementation
UICircle3D::UICircle3D(UIPointFloat3 center, float radius, int renderLevel) 
	: _center(center._x, center._y, center._z), _radius(radius), _renderLevel(renderLevel) {}

void UICircle3D::operator()(const UIColor& color, UICameraBase3D* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldCircle(_center, _radius, color, 255, _renderLevel, pCamera);
}

// UITriangle3D implementation
UITriangle3D::UITriangle3D(UIPointFloat3 p1, UIPointFloat3 p2, UIPointFloat3 p3, int renderLevel) 
	: _p1(p1._x, p1._y, p1._z), _p2(p2._x, p2._y, p2._z), _p3(p3._x, p3._y, p3._z), _renderLevel(renderLevel) {}

void UITriangle3D::operator()(const UIColor& color1, const UIColor& color2, const UIColor& color3, UCHAR alpha, UICameraBase3D* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldTriangle(_p1, _p2, _p3, color1, color2, color3, alpha, _renderLevel, pCamera);
}

void UITriangle3D::operator()(const UIColor& color, UCHAR alpha, UICameraBase3D* pCamera) {
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldTriangle(_p1, _p2, _p3, color, alpha, _renderLevel, pCamera);
}

// UIFont3D implementation
UIFont3D::UIFont3D(float fontSize, int renderLevel) : _fontSize(fontSize), _renderLevel(renderLevel) {}

void UIFont3D::operator()(std::wstring text, const UIPointFloat3& position, const UIColor& color, UICameraBase3D* pCamera) {
	// Use UIDXFoundation's 3D world text rendering directly
	UIDXFoundation::GetSingletonInstance()->Draw3DWorldTextFT(text, {position._x, position._y, position._z}, color, _fontSize, _renderLevel, pCamera);
}
