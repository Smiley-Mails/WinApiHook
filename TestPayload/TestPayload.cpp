#include "framework.h"

// Оригинальная процедура окна TestApp
WNDPROC g_pOldWndProc = nullptr;
// Переменная для анимации цвета
BYTE g_ColorStep = 0;

// Кастомная процедура окна, которая будет выполняться внутри TestApp.
// Используем ANSI-версию (A), так как TestApp зарегистрирован как мультибайтовое окно.
LRESULT CALLBACK NewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Перехватываем сообщение отрисовки
    if (msg == WM_PAINT)
    {
        // Сначала даем оригинальному окну полностью нарисовать свой интерфейс
        LRESULT result = CallWindowProcA(g_pOldWndProc, hWnd, msg, wParam, lParam);

        // Теперь рисуем поверх!
        HDC hdc = GetDC(hWnd);
        if (hdc)
        {
            // Настраиваем прозрачный фон для текста
            SetBkMode(hdc, TRANSPARENT);

            // Создаем красивый меняющийся цвет (RGB-эффект)
            g_ColorStep += 5;
            COLORREF rainbowColor = RGB(g_ColorStep, 255 - g_ColorStep, 128 + g_ColorStep / 2);
            SetTextColor(hdc, rainbowColor);

            // Создаем жирный и четкий шрифт
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, VARIABLE_PITCH, "Consolas");
            HGDIOBJ hOldFont = SelectObject(hdc, hFont);

            // Выводим текст поверх интерфейса TestApp.
            const char szHack[] = "[ DLL INJECTED: MEMORY HIJACKED ]";
            TextOutA(hdc, 10, 95, szHack, static_cast<int>(strlen(szHack)));

            // Рисуем анимированную рамку вокруг клиентской зоны TestApp
            RECT rect;
            GetClientRect(hWnd, &rect);
            HPEN hPen = CreatePen(PS_SOLID, 3, rainbowColor);
            HGDIOBJ hOldPen = SelectObject(hdc, hPen);
            HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

            Rectangle(hdc, 5, 5, rect.right - 5, rect.bottom - 5);

            // Освобождаем ресурсы GDI, чтобы не было утечек памяти
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            ReleaseDC(hWnd, hdc);
        }
        return result;
    }

    // Запускаем таймер обновления при инициализации хука
    if (msg == WM_USER + 1337)
    {
        SetTimer(hWnd, 1337, 30, NULL);
    }

    if (msg == WM_TIMER && wParam == 1337)
    {
        // Принудительно заставляем окно перерисовываться для работы анимации цвета
        InvalidateRect(hWnd, NULL, FALSE);
    }

    // Все остальные сообщения без изменений передаем оригинальной процедуре TestApp
    return CallWindowProcA(g_pOldWndProc, hWnd, msg, wParam, lParam);
}

// Функция сигнатурного поиска (мини аналог Cheat Engine) прямо внутри процесса.
// Ищет адрес, где лежит наше целое число (1337).
void ModifySecretValue()
{
    // Получаем базовый адрес текущего запущенного модуля (TestApp.exe)
    unsigned char* pBase = (unsigned char*)GetModuleHandleA(NULL);
    if (!pBase) return;

    // Читаем заголовки PE-файла, чтобы узнать размер исполняемого образа в памяти
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)pBase;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(pBase + dosHeader->e_lfanew);
    DWORD sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

    int targetValue = 1337;     // Что мы ищем
    int newValue = 99999;       // На что мы хотим это изменить

    // Проходимся циклом по всей памяти образа TestApp.exe
    // Шаг равен 4 байтам, так как int занимает 4 байта и выровнен в памяти
    for (DWORD i = 0; i < sizeOfImage - sizeof(int); i += 4)
    {
        int* pCurrent = (int*)(pBase + i);

        // Защита от падения: проверяем, доступен ли этот адрес для чтения/записи
        // (в основном глобальные переменные лежат в секции .data, которая открыта для R/W)
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(pCurrent, &mbi, sizeof(mbi)) != 0)
        {
            if ((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_EXECUTE_READWRITE))
            {
                // Если нашли наше число 1337
                if (*pCurrent == targetValue)
                {
                    // Перезаписываем его напрямую!
                    *pCurrent = newValue;

#ifdef _DEBUG
                    OutputDebugStringA("Success: g_SecretValue found and modified to 99999!\n");
#endif

                    break; // Прерываем поиск после первого совпадения
                }
            }
        }
    }
}

// Поток, который запускается в памяти TestApp
DWORD WINAPI HookThread(LPVOID lpParam)
{
    // Небольшая пауза для стабильности
    Sleep(200);

    // Сначала производим хак памяти и меняем 1337 на 99999
    ModifySecretValue();

    // Находим окно текущего процесса, которое принадлежит нашему потоку
    HWND hTargetWnd = GetTopWindow(NULL);
    while (hTargetWnd)
    {
        DWORD pid = 0;
        GetWindowThreadProcessId(hTargetWnd, &pid);
        if (pid == GetCurrentProcessId())
        {
            if (IsWindowVisible(hTargetWnd))
                break;
        }
        hTargetWnd = GetNextWindow(hTargetWnd, GW_HWNDNEXT);
    }

    if (hTargetWnd)
    {
        // Делаем субклассирование. Используем ANSI-модификатор (GWLP_WNDPROC + SetWindowLongPtrA)
        // Это критически важно, так как TestApp собран без Unicode, и W-версия функции вызвала бы сбой обработки сообщений.
        g_pOldWndProc = (WNDPROC)SetWindowLongPtrA(hTargetWnd, GWLP_WNDPROC, (LONG_PTR)NewWndProc);

        // Отправляем кастомное сообщение, чтобы запустить таймер внутри NewWndProc
        PostMessageA(hTargetWnd, WM_USER + 1337, 0, 0);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, HookThread, NULL, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
        // При выгрузке DLL (если потребуется) возвращаем старый WndProc на место
        HWND hTargetWnd = GetTopWindow(NULL);
        while (hTargetWnd)
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hTargetWnd, &pid);
            if (pid == GetCurrentProcessId() && g_pOldWndProc)
            {
                SetWindowLongPtrA(hTargetWnd, GWLP_WNDPROC, (LONG_PTR)g_pOldWndProc);
                KillTimer(hTargetWnd, 1337);
                break;
            }
            hTargetWnd = GetNextWindow(hTargetWnd, GW_HWNDNEXT);
        }
        break;
    }
    return TRUE;
}