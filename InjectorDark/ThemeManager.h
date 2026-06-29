// ThemeManager.h : Управление светлой и темной темами оформления
#pragma once
#include <Windows.h>

// Объявляем функции для использования в WinMain
bool IsSystemDarkModeEnabled();
void ApplyTheme(HWND hWnd, bool darkMode);

// Функции для перехвата сообщений отрисовки контролов
LRESULT Handle_WM_CTLCOLOR(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool isDarkMode);
void SubclassGroupBox(HWND hGroupBox);
void SubclassButton(HWND hButton);

// Освобождение ресурсов (кистей) при выходе из программы
void CleanUpTheme();