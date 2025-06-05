#include "Common.h"
using namespace std;

#if _WIN32
void StringPasteFromClipboard::operator()(wstring& strBuf, HWND dstHwnd) {
	if (OpenClipboard(dstHwnd) == FALSE) {
		return;
	}

	HANDLE hData = GetClipboardData(CF_UNICODETEXT);

	wchar_t* clipBuf = (wchar_t*)GlobalLock(hData);
	strBuf = clipBuf;

	GlobalUnlock(hData);
	CloseClipboard();
}

void StringCopyToClipboard::operator()(const wstring& strBuf, HWND srcHwnd) {
	if (OpenClipboard(srcHwnd) == FALSE) {
		return;
	}

	HGLOBAL clipHandle;
	wchar_t* clipBuf;
	EmptyClipboard();

	clipHandle = GlobalAlloc(GMEM_DDESHARE, 2 * strBuf.size() + 2);
	clipBuf = (wchar_t*)GlobalLock(clipHandle);

	wmemcpy(clipBuf, strBuf.c_str(), strBuf.size());
	clipBuf[strBuf.size()] = _T('\0');

	GlobalUnlock(clipHandle);
	SetClipboardData(CF_UNICODETEXT, clipHandle);

	CloseClipboard();
}
#endif
