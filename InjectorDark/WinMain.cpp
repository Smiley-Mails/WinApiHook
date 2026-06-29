#include "framework.h"

#define MAX_LOADSTRING 100

// Scale a design that assumes 96-DPI pixels
double scaleX;
double scaleY;

// Define Macros 
#define SCALEX(argX) ((int) ((argX) * scaleX))
#define SCALEY(argY) ((int) ((argY) * scaleY))

// Define Init Function
void InitScaling() {
	HDC screen = GetDC(0);
	scaleX = GetDeviceCaps(screen, LOGPIXELSX) / 96.0;
	scaleY = GetDeviceCaps(screen, LOGPIXELSY) / 96.0;
	ReleaseDC(0, screen);
}

HINSTANCE hInst;
HWND hWnd;
HWND hList;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
WCHAR szString[MAX_LOADSTRING];
static HWND hListBox;

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);



// Create the Process List
static std::vector<Process_Struct> ProcessListStruct;

// Буфер для хранения полного пути к выбранной DLL-библиотеке
static WCHAR szDllPath[MAX_PATH] = L"";

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, 
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_INJECTOR, szWindowClass, MAX_LOADSTRING);

    // Register Window Class
	WNDCLASSEXW wcex;
    
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INJECTOR));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_MENU + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (RegisterClassExW(&wcex) == 0)
        return 1;

    // Create Window
	hInst = hInstance;

    // DPI Scale
    InitScaling();

	/*HWND*/ hWnd = CreateWindowW(szWindowClass, szTitle, WIN_MAIN_STYLES,
		(GetSystemMetrics(SM_CXSCREEN) - SCALEX(366)) / 2, (GetSystemMetrics(SM_CYSCREEN) - SCALEY(468)) / 2,
        SCALEX(366), SCALEY(468), nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
		return 1;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    // Messages loop
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //static HWND hList;
	// Буфер состояния темы для обработчиков отрисовки
	static bool bIsDarkMode = false;

    switch (msg)
    {
        case WM_CREATE:
        {
			// Create System Font
			HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

			// Create the ListView control
            hList = CreateListView(hWnd, hInst);

			if (!hList)
			{
				OutputDebugString(L"Failed to create ListView control !\n");
                exit(EXIT_FAILURE);
			}

			// Create the ListBox control
			//static HWND hListBox = CreateWindow(L"listbox", NULL,
			//	WS_CHILD | WS_VISIBLE | LBS_STANDARD,
			//	SCALEX(5), SCALEY(255), SCALEX(340), SCALEY(120), hWnd, NULL, hInst, 0);

			/*static HWND*/ hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("listbox"), NULL, 
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL, 
				SCALEX(5), SCALEY(252), SCALEX(340), SCALEY(120), hWnd, NULL, NULL, NULL);

			SendMessage(hListBox, WM_SETFONT, WPARAM(hFont), TRUE);
			/*SendMessage(hListBox, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);*/

			if (!hListBox)
			{
				OutputDebugString(L"Failed to create ListBox control !\n");
				exit(EXIT_FAILURE);
			}

			//SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Первый");
			//SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Второй");
			InfoStrigOut(hListBox, L"Simple DLL Injector by Smiley.");
			InfoStrigOut(hListBox, L"Compiled 12.06.2026.");
			InfoStrigOut(hListBox, L"Ready...");
			// Mode redraw List On
			SendMessage(hListBox, WM_SETREDRAW, TRUE, 0L);
			// Redraw List
			InvalidateRect(hListBox, NULL, TRUE);

			// Create the GroupBox control
			HWND hGr = CreateWindowEx(WS_EX_TRANSPARENT, L"BUTTON", L"Controll", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
				SCALEX(5), SCALEY(360), SCALEX(340), SCALEY(65), hWnd, NULL, hInst, NULL);

			// Вешаем темную тему на GroupBox:
			SubclassGroupBox(hGr);

			if (!hGr)
			{
				OutputDebugString(L"Failed to create GroupBox control !\n");
				exit(EXIT_FAILURE);
			}
			SendMessage(hGr, WM_SETFONT, WPARAM(hFont), TRUE);

			// Create the Update Button
			HWND Update_Button = CreateWindow(L"button", L"Update",
				WS_CHILD | WS_VISIBLE | BS_CENTER,
                SCALEX(30), SCALEY(380), SCALEX(90), SCALEY(30), hWnd, (HMENU)1001, hInst, 0);

			// Вешаем темную тему на кнопки:
			SubclassButton(Update_Button);

			if (!Update_Button)
			{
				MessageBox(NULL, L"Button Creation Failed!", L"Error!", MB_ICONEXCLAMATION);
				exit(EXIT_FAILURE);
			}
			SendMessage(Update_Button, WM_SETFONT, WPARAM(hFont), TRUE);

			HWND Sel_Dll_Button = CreateWindow(L"button", L"Shoose DLL",
				WS_CHILD | WS_VISIBLE | BS_CENTER,
				SCALEX(130), SCALEY(380), SCALEX(90), SCALEY(30), hWnd, (HMENU)1002, hInst, 0);

			// Вешаем темную тему на кнопки:
			SubclassButton(Sel_Dll_Button);

			if (!Sel_Dll_Button)
			{
				MessageBox(NULL, L"Button Creation Failed!", L"Error!", MB_ICONEXCLAMATION);
				exit(EXIT_FAILURE);
			}
			SendMessage(Sel_Dll_Button, WM_SETFONT, WPARAM(hFont), TRUE);

			HWND Inject_Button = CreateWindow(L"button", L"Inject",
				WS_CHILD | WS_VISIBLE | BS_CENTER,
				SCALEX(230), SCALEY(380), SCALEX(90), SCALEY(30), hWnd, (HMENU)1003, hInst, 0);

			// Вешаем темную тему на кнопки:
			SubclassButton(Inject_Button);

			if (!Inject_Button)
			{
				MessageBox(NULL, L"Button Creation Failed!", L"Error!", MB_ICONEXCLAMATION);
				exit(EXIT_FAILURE);
			}
			SendMessage(Inject_Button, WM_SETFONT, WPARAM(hFont), TRUE);

			// Передаем и hList, и hListBox в функцию
			GetProcessList(hList, hListBox, &ProcessListStruct);
			InitListViewColumns(hList, hInst);
			// Передаем сам вектор для правильного сопоставления индексов изображений
			InsertListViewItems(hList, ProcessListStruct);

			// Определяем и применяем тему одной строчкой кода!
			bIsDarkMode = IsSystemDarkModeEnabled();
			ApplyTheme(hWnd, bIsDarkMode);

			// ShowWindow(hList, SW_SHOWDEFAULT);
			UpdateWindow(hWnd);
        }
        break;

		// Отслеживаем смену темы пользователем в системе «на лету»
        case WM_SETTINGCHANGE:
		{
			if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)
			{
				bIsDarkMode = IsSystemDarkModeEnabled();
				ApplyTheme(hWnd, bIsDarkMode);
			}
		}
		break;

		// Перенаправляем сообщения покраски в наш ThemeManager
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSTATIC:
		{
			LRESULT lResult = Handle_WM_CTLCOLOR(hWnd, msg, wParam, lParam, bIsDarkMode);
			if (lResult != 0) return lResult; // Если перекрасили в темный — возвращаем кисть

			return DefWindowProc(hWnd, msg, wParam, lParam); // Для светлой темы отдаем стандартной процедуре
		}

		case WM_NOTIFY:
		{
			// Просто передаем обработку стандартной логике заполнения строк списка
			if (HandleWM_NOTIFY(hListBox, lParam, ProcessListStruct))
			{
				return TRUE;
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		break;

		case WM_COMMAND:
		{
			// Используем LOWORD, чтобы отсечь коды уведомлений кнопок
			switch (LOWORD(wParam))
			{
			case IDB_UPDATE:
			{
				GetProcessList(hList, hListBox, &ProcessListStruct);
				SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);
				InsertListViewItems(hList, ProcessListStruct);
				UpdateWindow(hList);
			}
			break;

			case IDB_SELDLL:
			{
				OPENFILENAMEW ofn;       // Структура стандартного диалогового окна
				WCHAR szFile[MAX_PATH] = L""; // Локальный временный буфер для окна

				// Полностью зануляем структуру перед использованием
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hWnd; // Родительское окно
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);

				// Фильтр файлов: показываем только .dll и "Все файлы"
				ofn.lpstrFilter = L"Dynamic Link Libraries (*.dll)\0*.dll\0All Files (*.*)\0*.*\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFileTitle = NULL;
				ofn.nMaxFileTitle = 0;
				ofn.lpstrInitialDir = NULL;

				// Флаги: файл должен существовать, путь должен быть валидным
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

				// Вызываем диалоговое окно
				if (GetOpenFileNameW(&ofn) == TRUE)
				{
					// Копируем выбранный путь в наш глобальный буфер
					lstrcpyW(szDllPath, szFile);

					// Извлекаем только имя файла из полного пути для красивого вывода в лог
					LPWSTR pszFileName = wcsrchr(szDllPath, L'\\');
					if (pszFileName) {
						pszFileName++; // Смещаемся за символ слэша
					}
					else {
						pszFileName = szDllPath;
					}

					// Формируем красивое сообщение для вашего ListBox логов
					WCHAR szLogMsg[MAX_PATH + 50];
					wsprintf(szLogMsg, L"Selected DLL: %s", pszFileName);

					InfoStrigOut(hListBox, szLogMsg);
				}
				else
				{
					// Сюда мы попадаем, если пользователь просто закрыл окно на "Отмена"
					InfoStrigOut(hListBox, L"DLL selection canceled.");
				}
			}
			break;

			case IDB_INJECT:
			{
				// 1. Проверяем, выбрана ли библиотека
				if (szDllPath[0] == L'\0')
				{
					InfoStrigOut(hListBox, L"Error: No DLL selected! Please select a DLL first.");
					break;
				}

				// 2. Получаем индекс выделенной строки из List View
				int iSelected = GetSelectedListViewItem(hList);
				if (iSelected == -1)
				{
					InfoStrigOut(hListBox, L"Error: No process selected! Please select a target process from the list.");
					break;
				}

				// 3. Вызываем нашу функцию инжекции из отдельного модуля
				InjectDll(hListBox, ProcessListStruct[iSelected], szDllPath);
			}
			break;

			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;

			default:
				return DefWindowProc(hWnd, msg, wParam, lParam);
			}
		}
		break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                // TODO: Add any drawing code that uses hdc here...
				
                EndPaint(hWnd, &ps);
            }
            break;

        case WM_DESTROY:
			CleanUpTheme(); // Освобождаем кисти из модуля тем
			ClearProcessList(ProcessListStruct); // Освобождаем иконки перед выходом
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
