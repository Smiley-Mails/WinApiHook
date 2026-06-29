// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
// Header Files
#include <tlhelp32.h>
#include <vector>
#include <algorithm>
#include <Shlobj_core.h>
#include "resource.h"
#include <CommCtrl.h>
#include <iterator>
#pragma comment(lib, "ComCtl32.Lib")
#include "ListView.h"
#include "ExecutionTimer.h"
#include "InfoStringOut.h"
#include "InjectorLogic.h"

#define WIN_MAIN_STYLES	(WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | \
						 WS_DLGFRAME | WS_SYSMENU & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX)