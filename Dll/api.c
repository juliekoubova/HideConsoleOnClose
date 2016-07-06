#include "../Shared/stdafx.h"
#include "../Shared/api.h"

#ifndef _WIN64

BOOL
WINAPI
LaunchWow64Helper(HWND ConsoleWindow);

#endif

DWORD WINAPI CleanupThreadProc(LPVOID lpParameter)
{
	CleanupHideConsole((PHIDE_CONSOLE)lpParameter);
	return 0;
}

VOID CALLBACK OnThreadExited(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	QueueUserWorkItem(CleanupThreadProc, lpParameter, WT_EXECUTEDEFAULT);
}

BOOL WINAPI EnableForWindow(HWND ConsoleWindow)
{
	if (!ConsoleWindow)
		return FALSE;

#ifndef _WIN64

	BOOL IsWow64;

	if (!IsWow64Process(GetCurrentProcess(), &IsWow64))
		return FALSE;

	if (IsWow64)
		return LaunchWow64Helper(ConsoleWindow);

#endif // ! _WIN64

	PHIDE_CONSOLE HideConsole = SetupHideConsole(ConsoleWindow);

	if (!HideConsole)
		return FALSE;

	BOOL RegisteredWait = RegisterWaitForSingleObject(
		&HideConsole->WaitHandle,
		HideConsole->ThreadHandle,
		OnThreadExited,
		HideConsole,
		INFINITE,
		WT_EXECUTEONLYONCE
	);

	if (!RegisteredWait)
		goto Cleanup;

	return TRUE;

Cleanup:

	if (HideConsole)
		CleanupHideConsole(HideConsole);

	return FALSE;
}