#include <windows.h>
#include <stdio.h>
#include "detours.h"

// #pragma comment(lib, "detours.lib")
// Указатель на оригинальную функцию TextOutA
static BOOL(WINAPI* Original_TextOutA)(HDC hdc, int x, int y, LPCSTR lpString, int c) = TextOutA;

// Флаг, чтобы мы пропатчили память только ОДИН раз при первом вызове отрисовки
static bool g_MemoryPatched = false;

// Наша функция поиска и изменения g_SecretValue (1337) внутри TestApp
void FindAndModifySecret()
{
    // Получаем базовый адрес TestApp.exe
    unsigned char* pBase = (unsigned char*)GetModuleHandleA(NULL);
    if (!pBase) return;

    // Читаем заголовки PE, чтобы узнать точный размер программы в памяти
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)pBase;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(pBase + dosHeader->e_lfanew);
    DWORD sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

    int targetValue = 1337;     // Что ищем
    int newValue = 77777;       // Какое новое значение хотим записать

    // Сканируем память с шагом 4 байта (размер int)
    for (DWORD i = 0; i < sizeOfImage - sizeof(int); i += 4)
    {
        int* pCurrent = (int*)(pBase + i);

        // Проверяем страницу памяти на доступность чтения и записи
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(pCurrent, &mbi, sizeof(mbi)) != 0)
        {
            if ((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_EXECUTE_READWRITE))
            {
                // Если нашли место, где хранится 1337
                if (*pCurrent == targetValue)
                {
                    *pCurrent = newValue; // Меняем значение в памяти процесса напрямую!
                    g_MemoryPatched = true;
                    break;
                }
            }
        }
    }
}

// Наш Detours-хук на функцию TextOutA
BOOL WINAPI Hooked_TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c)
{
    // Как только TestApp пытается нарисовать строку с переменной
    if (lpString && strncmp(lpString, "Variable is:", 12) == 0)
    {
        // Если мы еще не изменили переменную, делаем это прямо сейчас
        if (!g_MemoryPatched)
        {
            FindAndModifySecret();
        }

        // Мы меняем цвет текста на зеленый (RGB), чтобы показать, что сработал хук,
        // но САМУ СТРОКУ НЕ ПОДМЕНЯЕМ! Мы отдаем её оригинальной функции.
        COLORREF oldColor = SetTextColor(hdc, RGB(0, 200, 0)); // Зеленый цвет

        // Вызываем оригинальный TextOutA. Так как память уже изменена,
        // sprintf_s внутри TestApp уже сформировал строку "Variable is: 77777"
        BOOL result = Original_TextOutA(hdc, x, y, lpString, c);

        SetTextColor(hdc, oldColor);
        return result;
    }

    return Original_TextOutA(hdc, x, y, lpString, c);
}

// Поток установки перехвата
DWORD WINAPI DetoursHookThread(LPVOID lpParam)
{
    Sleep(50);

    // 1. ПЕРВОЕ ДЕЙСТВИЕ: Сначала находим и меняем переменную в памяти
    FindAndModifySecret();

    // 2. ВТОРОЕ ДЕЙСТВИЕ: Устанавливаем хук Detours на отрисовку
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)Original_TextOutA, Hooked_TextOutA);
    LONG error = DetourTransactionCommit();

    if (error == NO_ERROR)
    {
        // 3. ТРЕТЬЕ ДЕЙСТВИЕ: Заставляем окно перерисоваться.
        // Теперь TestApp зайдет в WM_PAINT, прочитает УЖЕ измененную переменную 77777,
        // сформирует правильную строку и вызовет TextOutA, который мы окрасим в зеленый!
        HWND hTargetWnd = GetTopWindow(NULL);
        while (hTargetWnd)
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hTargetWnd, &pid);
            if (pid == GetCurrentProcessId() && IsWindowVisible(hTargetWnd))
            {
                InvalidateRect(hTargetWnd, NULL, TRUE);
                UpdateWindow(hTargetWnd);
                break;
            }
            hTargetWnd = GetNextWindow(hTargetWnd, GW_HWNDNEXT);
        }
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, DetoursHookThread, NULL, 0, NULL);
    }
    return TRUE;
}