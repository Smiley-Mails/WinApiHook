#include <windows.h>
#include <stdio.h>
#include "detours.h" // Подключаем Detours из папки вашего проекта

// Подключаем статическую либу Detours (убедитесь, что имя файла совпадает с вашим)
// #pragma comment(lib, "detours.lib")

// 1. Создаем указатель на оригинальную функцию TextOutA.
// Detours использует этот указатель, чтобы сохранить оригинальные байты функции.
static BOOL(WINAPI* Original_TextOutA)(HDC hdc, int x, int y, LPCSTR lpString, int c) = TextOutA;
static BOOL(WINAPI* Original_TextOutW)(HDC hdc, int x, int y, LPCWSTR lpString, int c) = TextOutW;

// 2. Наша кастомная функция, которая будет вызываться ВМЕСТО оригинальной TextOutA
// 2. Хук для ANSI-версии (TextOutA)
BOOL WINAPI Hooked_TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c)
{
    // Проверяем: если TestApp пытается отрисовать строку, связанную с нашей переменной
    if (lpString && strncmp(lpString, "Variable is:", 12) == 0)
    {
        // Мы можем полностью подменить текст и его длину!
        const char* szNewText = "Variable is: DETOURED! (A)";
        int newLen = static_cast<int>(strlen(szNewText));

        // Меняем цвет текста именно для этой строки, чтобы было наглядно
        COLORREF oldColor = SetTextColor(hdc, RGB(255, 0, 0)); // Красный цвет

        // Вызываем оригинальную функцию, но уже с НАШИМИ параметрами текста
        BOOL result = Original_TextOutA(hdc, x, y, szNewText, newLen);

        // Возвращаем оригинальный цвет контексту устройства
        SetTextColor(hdc, oldColor);

        return result;
    }

    // Для всех остальных строк (разрешение экрана, CPU vendor и т.д.) 
    // просто вызываем оригинальный TextOutA без изменений
    return Original_TextOutA(hdc, x, y, lpString, c);
}

// 3. Хук для Unicode-версии (TextOutW)
BOOL WINAPI Hooked_TextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c)
{
    // Проверяем широкую строку на "Variable is:"
    if (lpString && wcsncmp(lpString, L"Variable is:", 12) == 0)
    {
        const wchar_t* szNewText = L"Variable is: DETOURED! (W)";
        int newLen = static_cast<int>(wcslen(szNewText));

        COLORREF oldColor = SetTextColor(hdc, RGB(0, 128, 255)); // Красивый сине-голубой цвет
        BOOL result = Original_TextOutW(hdc, x, y, szNewText, newLen);
        SetTextColor(hdc, oldColor);
        return result;
    }
    return Original_TextOutW(hdc, x, y, lpString, c);
}

// Поток инициализации хуков
DWORD WINAPI DetoursHookThread(LPVOID lpParam)
{
    // Небольшая пауза для стабильности инициализации в целевом процессе
    Sleep(100);

    // Инициализируем транзакцию Detours
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // Ставим хуки сразу на обе функции
    DetourAttach(&(PVOID&)Original_TextOutA, Hooked_TextOutA);
    DetourAttach(&(PVOID&)Original_TextOutW, Hooked_TextOutW);

    // Применяем изменения в памяти процесса
    LONG error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        // Хуки успешно поставлены. Теперь заставим окно TestApp немедленно перерисоваться!
        HWND hTargetWnd = GetTopWindow(NULL);
        while (hTargetWnd)
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hTargetWnd, &pid);
            if (pid == GetCurrentProcessId())
            {
                if (IsWindowVisible(hTargetWnd))
                {
                    // Посылаем сигнал "перерисовать всё окно прямо сейчас"
                    InvalidateRect(hTargetWnd, NULL, TRUE);
                    UpdateWindow(hTargetWnd);
                    break;
                }
            }
            hTargetWnd = GetNextWindow(hTargetWnd, GW_HWNDNEXT);
        }
        #ifdef _DEBUG
                OutputDebugStringA("Detours: TextOutA successfully hooked!\n");
        #endif
    }

    return 0;
}

// Поток деинициализации (снятия хуков)
void UninstallDetoursHook()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // Отсоединяем хук и возвращаем оригинальную TextOutA в первозданный вид
    // Снимаем оба хука при выгрузке
    DetourDetach(&(PVOID&)Original_TextOutA, Hooked_TextOutA);
    DetourDetach(&(PVOID&)Original_TextOutW, Hooked_TextOutW);

    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Запускаем установку хуков в отдельном потоке
        CreateThread(NULL, 0, DetoursHookThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        // Снимаем хуки при выгрузке DLL (для чистоты эксперимента)
        UninstallDetoursHook();
        break;
    }
    return TRUE;
}