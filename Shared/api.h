#pragma once
#include "stdafx.h"

#ifdef HIDE_CONSOLE_DLL
#  define HIDE_CONSOLE_API
#else
#  define HIDE_CONSOLE_API __declspec(dllimport)
#endif

#define WM_HIDE_CONSOLE (WM_APP+0)

#define HIDE_CONSOLE_NAME L"HideConsoleOnClose-1.0"

#define WOW64HELPER_MUTEX \
	L"Local\\" HIDE_CONSOLE_NAME L"-Wow64HelperMutex"

#define WOW64HELPER_READY_EVENT \
	L"Local\\" HIDE_CONSOLE_NAME L"-Wow64HelperReady"

#define WOW64HELPER_READY_TIMEOUT 2500

#define WOW64HELPER_SMTO_TIMEOUT 500

#define WOW64HELPER_WINDOW_CLASS \
	HIDE_CONSOLE_NAME L"-Wow64Helper"

typedef struct tagHIDE_CONSOLE
{
	HHOOK   CbtHook;
	HHOOK   GetMessageHook;
	HHOOK   WndProcHook;
	HHOOK   WndProcRetHook;

	HANDLE  ConhostThreadHandle;
	HANDLE  ConhostWaitHandle;

	HMODULE OurModuleHandle;
}
HIDE_CONSOLE, *PHIDE_CONSOLE;

BOOL WINAPI CleanupHideConsole(PHIDE_CONSOLE HideConsole, PBOOL WasLastHook);

PHIDE_CONSOLE WINAPI SetupHideConsole(HWND ConsoleWindow);

HIDE_CONSOLE_API
BOOL WINAPI EnableForWindow(HWND ConsoleWindow);

HIDE_CONSOLE_API
LONG WINAPI GetHookCount(VOID);

HIDE_CONSOLE_API
BOOL WINAPI CloseWindowOnLastUnhook(HWND WindowToBeClosed);