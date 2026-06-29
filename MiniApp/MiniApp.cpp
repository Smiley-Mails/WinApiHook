#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#pragma once
#include <Windows.h>
#include <winuser.h>
#include <string.h>
#include <sal.h>
#include <cstdlib>
#include "resource.h"
#include "cpuid.h"
#include <dwmapi.h>
#include <tchar.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
//#pragma comment(lib, "kernel32.lib")

// Флаг может отсутствовать в старых версиях SDK, поэтому страхуемся:
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int g_SecretValue = 1337;
HINSTANCE hInst = NULL;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);			//Прототип WndProc

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int CmdShow)
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_STANDARD_CLASSES; // Включает базовые элементы, включая кнопки
	InitCommonControlsEx(&icex);

	hInst = hInstance;
	HWND hWnd;
	MSG msg;
	WNDCLASSW wc; // Изменено на WNDCLASSW
	memset(&wc, 0, sizeof(WNDCLASSW));							// Зарезервировали место под наш класс окна
	//FillMemory(&wc, sizeof(WNDCLASS), 0);
	//ZeroMemory(&wc, sizeof(wc));
	wc.style = 0;
	wc.lpfnWndProc = WndProc;									//Задаем процесс окна
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON));	//LoadIcon(NULL, IDI_INFORMATION); // Изменено на LoadIconW и MAKEINTRESOURCEW
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW); // Изменено на LoadCursorW
	//wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);		// Белый фон формы
	wc.hbrBackground = CreateSolidBrush(RGB(32, 32, 32));
	wc.lpszClassName = L"Windows Form";							// Задали класс окна // Изменено на Unicode-строку (L)

	if (RegisterClassW(&wc) == 0)								// Зарегистрировали класс окна // Изменено на RegisterClassW
		return 1;

	// Задаем размер окна
	int size_x = 300;
	int size_y = 250;

	// Создаем окно // Изменено на CreateWindowExW / CreateWindowW и Unicode-строки
	hWnd = CreateWindowW(wc.lpszClassName, L"Windows Form", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		(GetSystemMetrics(SM_CXSCREEN) - size_x) / 2, (GetSystemMetrics(SM_CYSCREEN) - size_y) / 2,
		size_x, size_y, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL)
	{
		MessageBoxW(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION); // Изменено на MessageBoxW
		exit(EXIT_FAILURE);
	}

	// Включаем темную тему для заголовка окна
	BOOL value = TRUE;
	DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

	ShowWindow(hWnd, CmdShow);	// Показываем окно
	UpdateWindow(hWnd);	// Обновляем окно

	while (GetMessageW(&msg, NULL, 0, 0)) // Изменено на GetMessageW
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg); // Изменено на DispatchMessageW
	}
	return (int)msg.wParam;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		// Получаем дескриптор стандартного шрифта системы (Segoe UI / Tahoma)
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

		// Создаем кнопку // Изменено на CreateWindowW, класс "button" теперь L"button"
		HWND Push_Button = CreateWindowW(L"button", L"Press me",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, // <-- Изменили стиль BS_CENTER
			100, 125, 90, 30, hWnd, (HMENU)1, hInst, 0);

		if (Push_Button == NULL)
		{
			MessageBoxW(NULL, L"Button Creation Failed!", L"Error!", MB_ICONEXCLAMATION); // Изменено на MessageBoxW
			return -1; // Для WM_CREATE принято возвращать -1 в случае ошибки создания
		}

		// Устанавливаем шрифт для первой кнопки
		SendMessage(Push_Button, WM_SETFONT, (WPARAM)hFont, TRUE);

		// Создаем кнопку // Изменено на CreateWindowW, класс "button" теперь L"button"
		HWND Exit_Button = CreateWindowW(L"button", L"Close",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, // <-- Изменили стиль BS_CENTER
			100, 160, 90, 30, hWnd, (HMENU)2, hInst, 0);

		if (Exit_Button == NULL)
		{
			MessageBoxW(NULL, L"Button Creation Failed!", L"Error!", MB_ICONEXCLAMATION); // Изменено на MessageBoxW
			return -1;
		}

		// Устанавливаем шрифт для второй кнопки
		SendMessage(Exit_Button, WM_SETFONT, (WPARAM)hFont, TRUE);

		return 0; // Строго возвращаем 0, чтобы подтвердить успешное создание окна!
	}

	case WM_PAINT:
	{
		PAINTSTRUCT ps = { 0 };
		HDC hdc = BeginPaint(hWnd, &ps);

		// Включаем прозрачный фон для текста, 
		// чтобы буквы ложились прямо на темный фон окна
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(240, 240, 240)); // Светло-серый/белый text

		HFONT hFont = CreateFontW(15, 6, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, L"Consolas"); // Изменено на CreateFontW и L"Consolas"

		SelectObject(hdc, hFont);

		int iTextLen = 0;
		WCHAR szText[1024] = { 0 }; // ТCHAR/WCHAR массив для Unicode

		// Используем swprintf_s вместо sprintf_s и форматную строку L"..."
		iTextLen = swprintf_s(szText, L"Screen resolution: %d x %d dpi\n", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		TextOutW(hdc, 35, 12, szText, iTextLen); // Изменено на TextOutW

		DeleteObject(hFont);
		hFont = CreateFontW(15, 6, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, L"Consolas"); // Изменено на CreateFontW и L"Consolas"
		SelectObject(hdc, hFont);

		// Конвертируем ANSI строку из GetCpuVendorString() в Unicode (std::wstring)
		std::string ansiVendor = GetCpuVendorString();
		std::wstring wstrVendor(ansiVendor.begin(), ansiVendor.end());

		iTextLen = swprintf_s(szText, L"%s", wstrVendor.c_str()); // Изменено на swprintf_s
		TextOutW(hdc, 30, 40, szText, iTextLen); // Изменено на TextOutW

		iTextLen = swprintf_s(szText, L"Variable is: %d\n", g_SecretValue); // Изменено на swprintf_s
		TextOutW(hdc, 85, 65, szText, iTextLen); // Изменено на TextOutW
		EndPaint(hWnd, &ps);

		if (hFont)
		{
#ifdef _DEBUG
			OutputDebugStringW(L"hFont Object Deleted \n"); // Изменено на OutputDebugStringW
#endif
			DeleteObject(hFont);
		}
		return 0;
	}

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;

		// Проверяем, что это одна из наших кнопок (ID 1 или 2)
		if (pDIS->CtlID == 1 || pDIS->CtlID == 2)
		{
			HDC hdc = pDIS->hDC;
			RECT rect = pDIS->rcItem;

			// --- ТОТ САМЫЙ ТРЮК: ОЧИСТКА УГЛОВ ---
			// Создаем кисть цвета фона НАШЕГО ОКНА (темно-серую)
			HBRUSH hParentBgBrush = CreateSolidBrush(RGB(32, 32, 32));
			// Заливаем весь прямоугольник кнопки цветом окна.
			// Это мгновенно сотрет любые стандартные белые углы, которые пытается подсунуть Windows
			FillRect(hdc, &rect, hParentBgBrush);
			DeleteObject(hParentBgBrush);
			// -------------------------------------

			// 1. Определяем цвета в зависимости от состояния кнопки
			COLORREF bgColor;
			COLORREF borderColor = RGB(85, 85, 85);  // Цвет рамки
			COLORREF textColor = RGB(240, 240, 240); // Всегда светлый текст

			if (pDIS->itemState & ODS_SELECTED)
			{
				// Кнопка зажата мышкой (делаем чуть темнее или светлее для отклика)
				bgColor = RGB(60, 60, 60);
			}
			else if (pDIS->itemState & ODS_FOCUS) // ну или можно проверять наведение, если усложнять код
			{
				// Кнопка в фокусе (выбрана стрелочками/табом)
				bgColor = RGB(50, 50, 50);
			}
			else
			{
				// Обычное состояние (чуть светлее фона окна, чтобы выделялась)
				bgColor = RGB(45, 45, 45);
			}

			// 2. Создаем инструменты рисования
			HBRUSH hBrush = CreateSolidBrush(bgColor);      // Кисть для внутренности
			HPEN hPen = CreatePen(PS_SOLID, 1, borderColor); // Перо для контура рамки

			// 3. Выбираем их в контекст устройства (HDC) и сохраняем старые, чтобы вернуть назад
			HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
			HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

			// 4. Рисуем скруглённый прямоугольник
			// Последние два параметра (6, 6) — это ширина и высота эллипса скругления углов.
			// Чем больше числа, тем сильнее закругление. 6-8 — идеальный вариант для интерфейсов.
			RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);

			// 5. Возвращаем старые инструменты на место и удаляем созданные (чистим память)
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldPen);
			DeleteObject(hBrush);
			DeleteObject(hPen);

			// 6. Пишем текст
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, textColor);

			WCHAR szButtonText[64]; // Массив WCHAR для Unicode текста кнопки
			GetWindowTextW(pDIS->hwndItem, szButtonText, sizeof(szButtonText) / sizeof(WCHAR)); // Изменено на GetWindowTextW и корректный подсчет размера буфера в символах
			DrawTextW(hdc, szButtonText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); // Изменено на DrawTextW

			return TRUE;
		}
		break;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case 1:
			MessageBoxW(hWnd, L"Button pressed", L"All right!", MB_OK); // Изменено на MessageBoxW
			// SetWindowText(hButton, wParam == 1 ? TEXT("Clicked!") : TEXT("Press me"));
			// SetWindowText(hButton, "Clicked!");
			break;
		case 2:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProcW(hWnd, msg, wParam, lParam); // Изменено на DefWindowProcW
		}
		return 0;
	}

	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		return 0;
	}

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	}
	// ВСЕ остальные сообщения Windows текут сюда!
	return DefWindowProcW(hWnd, msg, wParam, lParam); // Изменено на DefWindowProcW
}