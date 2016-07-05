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

BOOL 
WINAPI 
CleanupHideConsole(PHIDE_CONSOLE HideConsole);

PHIDE_CONSOLE
WINAPI
SetupHideConsole(DWORD ThreadId);

#ifndef _WIN64

BOOL
WINAPI
LaunchSysNativeApplet(DWORD ThreadId);

#endif