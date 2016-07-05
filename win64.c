#include "stdafx.h"
#include "HideConsoleOnClose.h"

VOID WINAPI AppletW(
	HWND      hWnd,
	HINSTANCE hInstance,
	LPCWSTR   lpszCmdLine,
	INT32     nCmdShow
)
{
	DWORD ThreadId = StrToIntW(lpszCmdLine);

	if (!ThreadId)
		return;

	PHIDE_CONSOLE HideConsole = SetupHideConsole(ThreadId);

	if (!HideConsole)
		return;

	WaitForSingleObject(HideConsole->ThreadHandle, INFINITE);

	CleanupHideConsole(HideConsole);
}

