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

typedef struct tagHIDE_CONSOLE_HOOKS
{
	HHOOK   CbtHook;
	HHOOK   GetMessageHook;
	HHOOK   WndProcHook;
	HHOOK   WndProcRetHook;
}
HIDE_CONSOLE_HOOKS, *PHIDE_CONSOLE_HOOKS;

typedef struct tagHIDE_CONSOLE_WAIT
{
	HANDLE ObjectHandle;
	HANDLE WaitHandle;
}
HIDE_CONSOLE_WAIT, *PHIDE_CONSOLE_WAIT;

typedef struct tagHIDE_CONSOLE
{
	HMODULE            Module;
	HIDE_CONSOLE_HOOKS Hooks;

	DWORD              WaitsCount;
	HIDE_CONSOLE_WAIT  Waits[0];
}
HIDE_CONSOLE, *PHIDE_CONSOLE;

HIDE_CONSOLE_API
BOOL WINAPI EnableForWindow(HWND ConsoleWindow);

HIDE_CONSOLE_API
BOOL WINAPI EnableForWindowWithOwner(HWND ConsoleWindow, DWORD OwnerThreadId);

HIDE_CONSOLE_API
BOOL WINAPI CloseWindowOnLastUnhook(HWND WindowToBeClosed);

HIDE_CONSOLE_API
LONG WINAPI GetHookCount(VOID);
