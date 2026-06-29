#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <Windows.h>
#include <winuser.h>
#include "resource.h"
#include "cpuid.h"
//#include <stdio.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int g_SecretValue = 1337;
HINSTANCE hInst = NULL;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	//Прототип WndProc

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int CmdShow)
{
	hInst = hInstance;
	HWND hWnd;
	MSG msg;
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));							// Зарезервировали место под наш класс окна
	//FillMemory(&wc, sizeof(WNDCLASS), 0);
	//ZeroMemory(&wc, sizeof(wc));
	wc.style = 0;
	wc.lpfnWndProc = WndProc;									//Задаем процесс окна
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));	//LoadIcon(NULL, IDI_INFORMATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);		// Белый фон формы
	wc.lpszClassName = "Windows Form";							// Задали класс окна

	if (RegisterClass(&wc) == 0)								// Зарегистрировали класс окна
		exit(EXIT_FAILURE);

	// Создаем окно
	hWnd = CreateWindow(wc.lpszClassName, "Windows Form", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		(GetSystemMetrics(SM_CXSCREEN) - 300) / 2, (GetSystemMetrics(SM_CYSCREEN) - 250) / 2, 
		300, 250, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION);
		exit(EXIT_FAILURE);
	}
	// Делаем окно "Всегда поверх остальных" (Topmost)
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	ShowWindow(hWnd, CmdShow);	// Показываем окно
	UpdateWindow(hWnd);	// Обновляем окно

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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

			// Создаем кнопку
			HWND Push_Button = CreateWindow("button", "Press me",
				WS_CHILD | WS_VISIBLE | BS_CENTER,
				100, 125, 90, 30, hWnd, (HMENU)1001, hInst, 0);

			if (Push_Button == NULL)
			{
				MessageBox(NULL, "Button Creation Failed!", "Error!", MB_ICONEXCLAMATION);
				return -1;
			}

			// Устанавливаем шрифт для первой кнопки
			SendMessage(Push_Button, WM_SETFONT, (WPARAM)hFont, TRUE);

			// Создаем кнопку
			HWND Exit_Button = CreateWindow("button", "Close", 
				WS_CHILD | WS_VISIBLE | BS_CENTER,
				100, 160, 90, 30, hWnd, (HMENU)1002, hInst, 0);

			if (Exit_Button == NULL)
			{
				MessageBox(NULL, "Button Creation Failed!", "Error!", MB_ICONEXCLAMATION);
				return -1;
			}

			// Устанавливаем шрифт для второй кнопки
			SendMessage(Exit_Button, WM_SETFONT, (WPARAM)hFont, TRUE);

			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps = { 0 };
			HDC hdc = BeginPaint(hWnd, &ps);
			
			HFONT hFont = CreateFont(15, 6, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
				CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, "Consolas");

			SelectObject(hdc, hFont);

			int iTextLen = 0;
			TCHAR szText[1024] = { 0 };
			iTextLen = sprintf_s(szText, "Screen resolution: %d x %d dpi\n", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
			TextOut(hdc, 40, 12, szText, iTextLen);

			DeleteObject(hFont);
			hFont = CreateFont(15, 6, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, "Consolas");
			SelectObject(hdc, hFont);

			iTextLen = sprintf_s(szText, GetCpuVendorString().c_str());
			TextOut(hdc, 35, 40, szText, iTextLen);
			iTextLen = sprintf_s(szText, "Variable is: %d\n", g_SecretValue);
			TextOut(hdc, 85, 65, szText, iTextLen);
			EndPaint(hWnd, &ps);
			if (hFont)
			{
				#ifdef _DEBUG
					OutputDebugString("hFont Object Deleted \n");
				#endif
				DeleteObject(hFont);
			}
			return 0;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case 1001:
				MessageBox(hWnd, "Button pressed", "All right!", MB_OK);
				// SetWindowText(hButton, wParam == 1 ? TEXT("Clicked!") : TEXT("Press me"));
				// SetWindowText(hButton, "Clicked!");
				break;
			case 1002:
				PostQuitMessage(0);
				break;

			default:
				return DefWindowProc(hWnd, msg, wParam, lParam);
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
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
