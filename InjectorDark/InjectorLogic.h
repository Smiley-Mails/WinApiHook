#pragma once
#include <Windows.h>
#include "ListView.h" // Нужен для структуры Process_Struct

// Функция возвращает true при успешной инжекции и false при ошибке.
// Принимает: хэндл ListBox для логов, структуру целевого процесса и полный путь к DLL.
bool InjectDll(HWND hListBox, const Process_Struct& targetProc, LPCWSTR szDllPath);