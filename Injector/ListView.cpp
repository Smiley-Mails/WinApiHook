// Create the Process List

#include "framework.h"

static TCHAR szBuff[MAX_PATH];
// Объявляем хэндл списка изображений для иконок процессов
static HIMAGELIST hImageList = nullptr;

HWND CreateListView(HWND hwndParent, HINSTANCE hInst)
{
	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	RECT rcClient;                       // The parent window's client area.

	GetClientRect(hwndParent, &rcClient);

	// Create the list-view window in report view with label editing enabled.
	HWND hWndListView = CreateWindow(WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
		6, 0,
		(rcClient.right - rcClient.left) - 13,
		((rcClient.bottom - rcClient.top) * 10) / 17, // (rcClient.bottom - rcClient.top) / 1.7,
		hwndParent,
		NULL,
		hInst,
		NULL);

	// The LVS_EX_xxx styles are extended listview styles, which...
	ListView_SetExtendedListViewStyleEx(hWndListView, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	return (hWndListView);
}

BOOL InitListViewColumns(HWND hWndListView, HINSTANCE hInst)
{
	TCHAR szText[256];
	LVCOLUMN lvc;
	int iCol;

	RECT rcClient;
	GetClientRect(hWndListView, &rcClient);

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	for (iCol = 0; iCol < C_COLUMNS; iCol++)
	{
		lvc.iSubItem = iCol;
		lvc.pszText = szText;

		if (iCol < 1)
			lvc.cx = (((rcClient.right - rcClient.left) / 3) * 2);
		else
			lvc.cx = ((rcClient.right - rcClient.left) / 7);

		if (iCol < 1)
			lvc.fmt = LVCFMT_LEFT;
		else
			lvc.fmt = LVCFMT_CENTER;

		LoadString(hInst, IDS_FIRSTCOLUMN + iCol, szText, std::size(szText));

		if (ListView_InsertColumn(hWndListView, iCol, &lvc) == -1)
			return FALSE;
	}

	return TRUE;
}

BOOL InsertListViewItems(HWND hWndListView, const std::vector<Process_Struct>& strList)
{
	LVITEM lvI;

	// Инициализируем общие параметры
	lvI.pszText = LPSTR_TEXTCALLBACK;
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvI.stateMask = 0;
	lvI.iSubItem = 0;
	lvI.state = 0;

	for (size_t index = 0; index < strList.size(); index++)
	{
		lvI.iItem = static_cast<int>(index);

		// Задаем индекс изображения. Индекс иконки совпадает с индексом процесса в векторе
		lvI.iImage = (strList[index].hIcon != nullptr) ? static_cast<int>(index) : -1;

		if (ListView_InsertItem(hWndListView, &lvI) == -1)
			return FALSE;
	}

	return TRUE;
}

BOOL GetProcessList(HWND hWndListView, HWND hListBox, std::vector<Process_Struct>* pstrList)
{
	if (!pstrList) return FALSE;

	// Уничтожаем старый список изображений, если он существовал
	if (hImageList)
	{
		ImageList_Destroy(hImageList);
		hImageList = nullptr;
	}

	ClearProcessList(*pstrList);

	PROCESSENTRY32 peProcessEntry;
	HANDLE CONST hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (INVALID_HANDLE_VALUE == hSnapshot) {
		return FALSE;
	}

	peProcessEntry.dwSize = sizeof(PROCESSENTRY32);

	// 1. Сначала просто собираем все процессы в вектор
	if (Process32First(hSnapshot, &peProcessEntry))
	{
		do
		{
			Process_Struct item{};

			item.PID = peProcessEntry.th32ProcessID;
			lstrcpyW(item.szName, peProcessEntry.szExeFile);
			item.Arch = getProcArch(peProcessEntry.th32ProcessID);

			bool path_valid = getProcFullPathW(item.szPath, static_cast<DWORD>(std::size(item.szPath)), item.PID);

			if (path_valid)
			{
				// Пока не извлекаем иконку, сделаем это после сортировки вектора
				item.hIcon = nullptr;
			}
			else
			{
				item.hIcon = nullptr;
				item.szPath[0] = L'\0';
			}

			pstrList->push_back(item);

		} while (Process32Next(hSnapshot, &peProcessEntry));
	}

	CloseHandle(hSnapshot);

	// 2. СОРТИРОВКА ВЕКТОРА (от A до Z, без учета регистра)
	std::sort(pstrList->begin(), pstrList->end(), [](const Process_Struct& a, const Process_Struct& b) {
		return _wcsicmp(a.szName, b.szName) < 0;
		});

	// 3. Теперь, когда вектор отсортирован, создаем Image List и извлекаем иконки в правильном порядке
	hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, static_cast<int>(pstrList->size()), 10);

	for (auto& item : *pstrList)
	{
		// Извлекаем иконку только если путь к файлу был успешно найден ранее
		if (item.szPath[0] != L'\0')
		{
			HRESULT hr = SHDefExtractIconW(item.szPath, 0, NULL, nullptr, &item.hIcon, 16);
			if (FAILED(hr))
			{
				item.hIcon = nullptr;
			}
		}

		// Добавляем иконку (или стандартную заглушку) в Image List
		if (item.hIcon != nullptr && hImageList != nullptr)
		{
			ImageList_AddIcon(hImageList, item.hIcon);
		}
		else if (hImageList != nullptr)
		{
			HICON hDefaultIcon = LoadIcon(nullptr, IDI_APPLICATION);
			ImageList_AddIcon(hImageList, hDefaultIcon);
			item.hIcon = hDefaultIcon;
		}
	}

	// Привязываем заполненный Image List к List View
	if (hWndListView && hImageList)
	{
		ListView_SetImageList(hWndListView, hImageList, LVSIL_SMALL);
	}

	InfoStrigOut(hListBox, L"Process List Updated and Sorted.");

	return TRUE;
}

bool getProcFullPathW(wchar_t* szfullPath, DWORD BufferSize, int PID)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, PID);

	if (hProc != NULL)
	{
		BOOL bRet = QueryFullProcessImageNameW(hProc, 0, szfullPath, &BufferSize);
		CloseHandle(hProc);
		return (bRet != FALSE);
	}

	return false;
}

bool IsNativeProcess(const int PID)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, PID);
	if (hProc != NULL)
	{
		BOOL bIsWow64 = FALSE;
		IsWow64Process(hProc, &bIsWow64);
		CloseHandle(hProc);
		return (bIsWow64 == FALSE);
	}
	return true;
}

ARCH getProcArch(const int PID)
{
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		return ARCH::X86;
	}

	if (IsNativeProcess(PID))
	{
		return ARCH::X64;
	}

	return ARCH::X86;
}

void ClearProcessList(std::vector<Process_Struct>& strList)
{
	for (auto& item : strList)
	{
		if (item.hIcon != nullptr)
		{
			DestroyIcon(item.hIcon);
			item.hIcon = nullptr;
		}
	}
	strList.clear();

	if (hImageList)
	{
		ImageList_Destroy(hImageList);
		hImageList = nullptr;
	}
}

BOOL HandleWM_NOTIFY(HWND hListBox, LPARAM lParam, const std::vector<Process_Struct>& strList)
{
	NMLVDISPINFO* plvdi;
	LPNMLISTVIEW pnmv;

	switch (((LPNMHDR)lParam)->code)
	{
	case LVN_GETDISPINFO:
		plvdi = (NMLVDISPINFO*)lParam;

		if (plvdi->item.iItem < 0 || plvdi->item.iItem >= (int)strList.size())
			break;

		switch (plvdi->item.iSubItem)
		{
		case 0:
			lstrcpy(plvdi->item.pszText, strList[plvdi->item.iItem].szName);
			break;
		case 1:
			wsprintf(szBuff, L"%d", strList[plvdi->item.iItem].PID);
			plvdi->item.pszText = szBuff;
			break;
		case 2:
			if (strList[plvdi->item.iItem].Arch == ARCH::X64)
				lstrcpy(plvdi->item.pszText, L"x64");
			else if (strList[plvdi->item.iItem].Arch == ARCH::X86)
				lstrcpy(plvdi->item.pszText, L"x86");
			else
				lstrcpy(plvdi->item.pszText, L"Unknown");
			break;
		}
		return TRUE; // Сообщаем, что обработали заполнение текста

	case LVN_ITEMCHANGED:
		pnmv = (LPNMLISTVIEW)lParam;

		// Проверяем, что изменилось состояние выделения элемента, 
		// и элемент стал ИМЕННО ВЫДЕЛЕННЫМ (а не с него сняли выделение)
		if ((pnmv->uChanged & LVIF_STATE) &&
			(pnmv->uNewState & LVIS_SELECTED) &&
			!(pnmv->uOldState & LVIS_SELECTED))
		{
			int iItem = pnmv->iItem;
			if (iItem >= 0 && iItem < (int)strList.size())
			{
				wchar_t szLogBuff[MAX_PATH + 64];
				// Формируем сообщение: Название процесса и его PID
				wsprintf(szLogBuff, L"Selected target process: %s (PID: %d)",
					strList[iItem].szName, strList[iItem].PID);

				// Выводим в твой ListBox логов внизу приложения
				InfoStrigOut(hListBox, szLogBuff);
			}
		}
		break;
	}

	return FALSE;
}

int GetSelectedListViewItem(HWND hWndListView)
{
	// Ищем элемент со статусом LVIS_SELECTED
	return ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
}