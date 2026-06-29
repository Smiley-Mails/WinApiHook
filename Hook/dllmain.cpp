#include <Windows.h>
#include <detours.h>
/*
#ifdef _WIN64
  #include "../Detours/X64/include/detours.h"
  #pragma comment(lib, "./Detours/X64/lib.X64/detours.lib")
#else
  #include "../Detours/X86/include/detours.h"
  #pragma comment(lib, "./Detours/X86/lib.X86/detours.lib")
#endif
*/

// Address of the real WriteFile API
BOOL(WINAPI* TrueMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT) = MessageBoxA;

// Our intercept function
BOOL WINAPI HookedMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    return TrueMessageBoxA(hWnd, "Dll injected in process! \n\n\t Process Hacked!", "Hacked!", NULL);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueMessageBoxA, HookedMessageBoxA);
        DetourTransactionCommit();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueMessageBoxA, HookedMessageBoxA);
        DetourTransactionCommit();
    }
    return TRUE;
}
