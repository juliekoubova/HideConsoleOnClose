#pragma once
#include "stdafx.h"

#ifdef HIDE_CONSOLE_DLL
#  define HIDE_CONSOLE_API
#else
#  define HIDE_CONSOLE_API __declspec(dllimport)
#endif

#define WM_HIDE_CONSOLE (WM_APP+0)

typedef struct tagHIDE_CONSOLE
{
	HHOOK  CbtHook;
	HHOOK  GetMessageHook;
	HHOOK  WndProcHook;
	HHOOK  WndProcRetHook;
	HANDLE WaitHandle;
	HANDLE ThreadHandle;
}
HIDE_CONSOLE, *PHIDE_CONSOLE;

BOOL WINAPI CleanupHideConsole(PHIDE_CONSOLE HideConsole);

PHIDE_CONSOLE WINAPI SetupHideConsole(HWND ConsoleWindow);

HIDE_CONSOLE_API
BOOL WINAPI EnableForWindow(HWND ConsoleWindow);