//Outputput Info and Debug String

#include "InfoStringOut.h"

void InfoStrigOut(HWND handle, LPCWSTR OutputString)
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);

	static TCHAR strBuff[MAX_PATH];
	wsprintf(strBuff, L"%02d:%02d:%02d -> %s\r\n", lt.wHour, lt.wMinute, lt.wSecond, OutputString);
	SendMessage(handle, LB_ADDSTRING, 0, (LPARAM)strBuff);

	// Автоскролл списка логов вниз:
	LRESULT count = SendMessage(handle, LB_GETCOUNT, 0, 0);
	if (count != LB_ERR) {
		SendMessage(handle, LB_SETTOPINDEX, count - 1, 0);
	}

#ifdef _DEBUG
	wsprintf(strBuff, L"%02d:%02d:%02d Debug out: \"%s\"\r\n", lt.wHour, lt.wMinute, lt.wSecond, OutputString);
	OutputDebugString(strBuff);
#endif
}