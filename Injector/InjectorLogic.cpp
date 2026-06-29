#include "framework.h"
#include "InjectorLogic.h"

// Функция для включения привилегии SeDebugPrivilege.
// Позволяет инжектору получать доступ к ЛЮБЫМ процессам в системе.
bool EnableDebugPrivilege()
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		CloseHandle(hToken);
		return false;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	BOOL bRet = AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	CloseHandle(hToken);

	return (bRet && GetLastError() == ERROR_SUCCESS);
}

bool InjectDll(HWND hListBox, const Process_Struct& targetProc, LPCWSTR szDllPath)
{
	// Автоматически пытаемся поднять права инжектора до уровня отладки
	if (EnableDebugPrivilege()) {
		InfoStrigOut(hListBox, L"System: SeDebugPrivilege enabled successfully.");
	}
	else {
		InfoStrigOut(hListBox, L"Warning: Failed to enable SeDebugPrivilege. \nRun as Admin if access is denied.");
	}

	// 1. Проверяем входные данные на всякий случай
	if (!szDllPath || szDllPath[0] == L'\0')
	{
		InfoStrigOut(hListBox, L"Error: Path to DLL is empty.");
		return false;
	}

	// 2. Проверка разрядности (архитектуры)
	// Компилятор определяет разрядность самого инжектора (_WIN64),
	// и мы сравниваем её с разрядностью выбранного процесса.
#ifdef _WIN64
	if (targetProc.Arch == ARCH::X86)
	{
		InfoStrigOut(hListBox, L"Error: Cannot inject 64-bit DLL into a 32-bit (x86) process.");
		return false;
	}
#else
	if (targetProc.Arch == ARCH::X64)
	{
		InfoStrigOut(hListBox, L"Error: Cannot inject 32-bit DLL into a 64-bit (x64) process.");
		return false;
	}
#endif

	WCHAR szLog[MAX_PATH + 100];
	wsprintf(szLog, L"Attempting to inject into %s (PID: %d)...", targetProc.szName, targetProc.PID);
	InfoStrigOut(hListBox, szLog);

	// 3. Открываем дескриптор целевого процесса с нужными правами доступа
	HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
		FALSE, targetProc.PID);
	if (!hProcess)
	{
		wsprintf(szLog, L"Error: OpenProcess failed. Error code: %d", GetLastError());
		InfoStrigOut(hListBox, szLog);
		return false;
	}

	// 4. Выделяем виртуальную память внутри целевого процесса под строку с путем к DLL
	size_t pathSize = (lstrlenW(szDllPath) + 1) * sizeof(WCHAR);
	LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pRemoteBuf)
	{
		wsprintf(szLog, L"Error: VirtualAllocEx failed. Error code: %d", GetLastError());
		InfoStrigOut(hListBox, szLog);
		CloseHandle(hProcess);
		return false;
	}

	// 5. Записываем путь к нашей DLL в выделенный буфер удаленного процесса
	SIZE_T bytesWritten = 0;
	if (!WriteProcessMemory(hProcess, pRemoteBuf, szDllPath, pathSize, &bytesWritten))
	{
		wsprintf(szLog, L"Error: WriteProcessMemory failed. Error code: %d", GetLastError());
		InfoStrigOut(hListBox, szLog);
		VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	// 6. Получаем адрес функции LoadLibraryW из библиотеки kernel32.dll
	PTHREAD_START_ROUTINE pLoadLibraryW = (PTHREAD_START_ROUTINE)GetProcAddress(
		GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");

	if (!pLoadLibraryW)
	{
		InfoStrigOut(hListBox, L"Error: Could not find LoadLibraryW address.");
		VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	// 7. Создаем удаленный поток (Remote Thread) в процессе жертвы
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibraryW, pRemoteBuf, 0, NULL);
	if (!hThread)
	{
		wsprintf(szLog, L"Error: CreateRemoteThread failed. Error code: %d", GetLastError());
		InfoStrigOut(hListBox, szLog);
		VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	// Инжекция запущена успешно! 
	wsprintf(szLog, L"Success! DLL successfully injected into %s.", targetProc.szName);
	InfoStrigOut(hListBox, szLog);

	// Ожидаем завершения потока (вызова LoadLibraryW), чтобы безопасно освободить память буфера
	WaitForSingleObject(hThread, 2000);

	// Чистим за собой ресурсы
	VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
	CloseHandle(hThread);
	CloseHandle(hProcess);

	return true;
}