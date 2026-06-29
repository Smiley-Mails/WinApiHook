// CreateListView: Creates a list-view control functions.
#pragma once
#include <vector>

enum class ARCH : int
{
	NONE,
	X64,
	X86
};

struct Process_Struct
{
	DWORD	PID = 0;
	wchar_t	szName[MAX_PATH];
	wchar_t szPath[MAX_PATH];
	ARCH	Arch = ARCH::NONE;
	/*int     Session;*/
	HICON	hIcon = nullptr;
	//Process_Struct() {};
	//~Process_Struct() {};
};

HWND CreateListView(HWND hwndParent, HINSTANCE hListViewInst);
BOOL InitListViewColumns(HWND hWndListView, HINSTANCE hListViewInst);
BOOL InsertListViewItems(HWND hWndListView, const std::vector<Process_Struct>& strList);
BOOL GetProcessList(HWND hWndListView, HWND hListBox, std::vector<Process_Struct>* pstrList);
bool getProcFullPathW(wchar_t* szfullPath, DWORD BufferSize, int PID);
BOOL HandleWM_NOTIFY(HWND hListBox, LPARAM lParam, const std::vector<Process_Struct>& strList);
bool IsNativeProcess(const int PID);
ARCH getProcArch(const int PID);
void ClearProcessList(std::vector<Process_Struct>& strList);
int GetSelectedListViewItem(HWND hWndListView);