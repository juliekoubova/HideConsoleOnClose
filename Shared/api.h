#pragma once
#include "stdafx.h"

typedef struct tagHIDE_CONSOLE
{
	HHOOK  Cbt;
	HHOOK  WndProc;
	HHOOK  WndProcRet;
	HANDLE WaitHandle;
	HANDLE ThreadHandle;
}
HIDE_CONSOLE, *PHIDE_CONSOLE;

extern HINSTANCE g_ModuleHandle;

#ifdef HIDE_CONSOLE_DLL
#  define HIDE_CONSOLE_API
#else
#  define HIDE_CONSOLE_API __declspec(dllimport)
#endif

HIDE_CONSOLE_API
BOOL 
WINAPI 
CleanupHideConsole(PHIDE_CONSOLE HideConsole);

HIDE_CONSOLE_API
PHIDE_CONSOLE
WINAPI
SetupHideConsole(DWORD ThreadId);

#ifndef _WIN64

BOOL
WINAPI
LaunchWow64Applet(DWORD ThreadId);

#endif