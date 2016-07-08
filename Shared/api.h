#pragma once
#include "stdafx.h"

#ifdef HIDE_CONSOLE_DLL
#  define HIDE_CONSOLE_API
#else
#  define HIDE_CONSOLE_API __declspec(dllimport)
#endif

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

HIDE_CONSOLE_API
BOOL WINAPI CleanupHideConsole(PHIDE_CONSOLE HideConsole);

HIDE_CONSOLE_API
PHIDE_CONSOLE WINAPI SetupHideConsole(HWND ConsoleWindow);
