// ThemeManager.cpp : Реализация управления темами оформления
#include "ThemeManager.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <tchar.h>
#include <CommCtrl.h> // Обязательно для работы с заголовками ListView

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

typedef HRESULT(WINAPI* PFNSETWINDOWTHEMENONCLIENTATTRIBUTES)(HWND hwnd, DWORD dwMask, DWORD dwAttributes);

// Статические ресурсы модуля
static HBRUSH hDarkBackgroundBrush = nullptr;
static HBRUSH hHeaderBackgroundBrush = nullptr;
static COLORREF clrDarkText = RGB(240, 240, 240);
static COLORREF clrDarkBk = RGB(30, 30, 30);
static COLORREF clrHeaderBk = RGB(45, 45, 45); // Чуть более светлый серый для шапки
static bool g_bDarkMode = false;

void EnableDarkScrollbar(HWND hWnd, bool darkMode) {
    HMODULE hUxTheme = GetModuleHandleW(L"uxtheme.dll");
    if (hUxTheme) {
        PFNSETWINDOWTHEMENONCLIENTATTRIBUTES pSetWindowThemeNonClientAttributes =
            (PFNSETWINDOWTHEMENONCLIENTATTRIBUTES)GetProcAddress(hUxTheme, "SetWindowThemeNonClientAttributes");

        if (pSetWindowThemeNonClientAttributes) {
            // Включаем темные неклиентские атрибуты (включая скроллбар)
            pSetWindowThemeNonClientAttributes(hWnd, darkMode ? 1 : 0, 1);
        }
    }
}

bool IsSystemDarkModeEnabled() {
    HKEY hKey;
    LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey);
    if (res == ERROR_SUCCESS) {
        DWORD value = 1;
        DWORD size = sizeof(value);
        RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size);
        RegCloseKey(hKey);
        return value == 0;
    }
    return false;
}

LRESULT CALLBACK GroupBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
    case WM_PAINT:
        if (g_bDarkMode) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // 1. Сначала просим Windows нарисовать стандартную рамку GroupBox
            DefSubclassProc(hWnd, uMsg, (WPARAM)hdc, lParam);

            // 2. Поверх рисуем наш светлый текст
            wchar_t szText[256];
            GetWindowTextW(hWnd, szText, 256);

            if (wcslen(szText) > 0) {
                HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                SetTextColor(hdc, clrDarkText); // Светлый текст
                SetBkColor(hdc, clrDarkBk);     // Темный фон (затрет кусок рамки под текстом)
                SetBkMode(hdc, OPAQUE);

                RECT rect;
                GetClientRect(hWnd, &rect);
                rect.left += 9; // Смещение текста вправо, как у стандартного GroupBox
                rect.top += 0;

                DrawTextW(hdc, szText, -1, &rect, DT_LEFT | DT_SINGLELINE);
                SelectObject(hdc, hOldFont);
            }

            EndPaint(hWnd, &ps);
            return 0;
        }
        break;
    case WM_DESTROY:
        RemoveWindowSubclass(hWnd, GroupBoxSubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Новая процедура субклассирования для ОБЫЧНЫХ КНОПОК
LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
    case WM_PAINT:
        if (g_bDarkMode) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);

            // Цвета для кнопки в темной теме
            static HBRUSH hBtnReleaseBrush = CreateSolidBrush(RGB(50, 50, 50)); // Обычное состояние (темно-серый)
            static HBRUSH hBtnPressBrush = CreateSolidBrush(RGB(70, 70, 70));   // При нажатии

            // Проверяем состояние кнопки (нажата или нет)
            LRESULT state = SendMessage(hWnd, BM_GETSTATE, 0, 0);
            bool isPressed = (state & BST_PUSHED) != 0;

            // 1. Рисуем тело кнопки
            FillRect(hdc, &rect, isPressed ? hBtnPressBrush : hBtnReleaseBrush);

            // 2. Рисуем тонкую рамку вокруг кнопки
            HBRUSH hBorderBrush = CreateSolidBrush(RGB(100, 100, 100));
            FrameRect(hdc, &rect, hBorderBrush);
            DeleteObject(hBorderBrush);

            // 3. Рисуем текст кнопки
            wchar_t szText[256];
            GetWindowTextW(hWnd, szText, 256);

            if (wcslen(szText) > 0) {
                HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                SetTextColor(hdc, clrDarkText); // Светлый текст!
                SetBkMode(hdc, TRANSPARENT);

                DrawTextW(hdc, szText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, hOldFont);
            }

            EndPaint(hWnd, &ps);
            return 0;
        }
        break;

        // Перерисовываем кнопку при наведении мыши и кликах, чтобы не было визуальных багов
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_DESTROY:
        RemoveWindowSubclass(hWnd, ButtonSubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void SubclassGroupBox(HWND hGroupBox) {
    if (hGroupBox) {
        SetWindowSubclass(hGroupBox, GroupBoxSubclassProc, 0, 0);
    }
}

void SubclassButton(HWND hButton) {
    if (hButton) {
        SetWindowSubclass(hButton, ButtonSubclassProc, 0, 0);
    }
}

void ApplyTheme(HWND hWnd, bool darkMode) {
    g_bDarkMode = darkMode;

    // 1. Красим заголовок главного окна
    BOOL allowDarkMode = darkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, 20, &allowDarkMode, sizeof(allowDarkMode));

    // 2. Инициализируем кисти
    if (!hDarkBackgroundBrush) hDarkBackgroundBrush = CreateSolidBrush(clrDarkBk);
    if (!hHeaderBackgroundBrush) hHeaderBackgroundBrush = CreateSolidBrush(clrHeaderBk);

    HBRUSH hLightBackgroundBrush = (HBRUSH)(COLOR_MENU + 1);
    SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)(darkMode ? hDarkBackgroundBrush : hLightBackgroundBrush));

    // 3. Настраиваем дочерний ListView
    HWND hChildListView = FindWindowEx(hWnd, NULL, WC_LISTVIEW, NULL);
    if (hChildListView) {
        // Переключаем тему оформления Windows Explorer
        SetWindowTheme(hChildListView, darkMode ? L"DarkMode_Explorer" : L"Explorer", nullptr);

        if (darkMode) {
            // Включаем двойную буферизацию для ListView, чтобы убрать мерцание белого фона при прокрутке
            SendMessage(hChildListView, LVM_SETEXTENDEDLISTVIEWSTYLE,
                LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES,
                LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            // Явно задаем цвета фона для строк и пустой области списка
            ListView_SetBkColor(hChildListView, clrDarkBk);
            ListView_SetTextBkColor(hChildListView, clrDarkBk);
            ListView_SetTextColor(hChildListView, clrDarkText);
        }
        else {
            // Возвращаем дефолтные цвета для светлой темы
            SendMessage(hChildListView, LVM_SETEXTENDEDLISTVIEWSTYLE,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES,
                LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            ListView_SetBkColor(hChildListView, CLR_NONE);
            ListView_SetTextBkColor(hChildListView, CLR_NONE);
            ListView_SetTextColor(hChildListView, CLR_DEFAULT);
        }

        // Принудительно обновляем заголовок колонок (Header control) внутри ListView
        HWND hHeader = ListView_GetHeader(hChildListView);
        if (hHeader) {
            SetWindowTheme(hHeader, darkMode ? /*L"DarkMode_ItemsView"*/ L"DarkMode_Explorer" : L"Explorer", nullptr);
        }

        // Для заголовков (Header) внутри ListView
        // Принудительно обновляем заголовок колонок (Header control) внутри ListView
        //HWND hHeader = ListView_GetHeader(hChildListView);
        //if (hHeader) {
        //    if (darkMode) {
        //        // Отключаем стандартные визуальные стили Windows, 
        //        // чтобы заставить работать наш CustomDraw в WndProc
        //        SetWindowTheme(hHeader, L"", L"");
        //    }
        //    else {
        //        // Для светлой темы возвращаем стандартный стиль Explorer
        //        SetWindowTheme(hHeader, L"Explorer", nullptr);
        //    }
        //}

        //HWND hHeader = ListView_GetHeader(hChildListView);
        //if (hHeader) {
        //    // Для любого режима (и темного, и светлого) оставляем стандартную светлую шапку Explorer
        //    SetWindowTheme(hHeader, L"Explorer", nullptr);
        //}

    }

    // Настройка ListBox (Логи)
    HWND hChildListBox = FindWindowEx(hWnd, NULL, L"LISTBOX", NULL);
    if (hChildListBox) {
        // Чтобы скроллбар ListBox стал темным, используем тему "Explorer" или "DarkMode_Explorer"
        SetWindowTheme(hChildListBox, darkMode ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        EnableDarkScrollbar(hChildListBox, darkMode);
    }

    RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
}

LRESULT Handle_WM_CTLCOLOR(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool isDarkMode) {
    if (isDarkMode) {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, clrDarkText);
        SetBkColor(hdc, clrDarkBk);
        return (LRESULT)hDarkBackgroundBrush;
    }
    return 0;
}

void CleanUpTheme() {
    if (hDarkBackgroundBrush) {
        DeleteObject(hDarkBackgroundBrush);
        hDarkBackgroundBrush = nullptr;
    }
    if (hHeaderBackgroundBrush) {
        DeleteObject(hHeaderBackgroundBrush);
        hHeaderBackgroundBrush = nullptr;
    }
}