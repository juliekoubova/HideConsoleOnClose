#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"

#ifndef _WIN64

BOOL
WINAPI
LaunchWow64Helper(HWND ConsoleWindow);

#endif

DWORD WINAPI CleanupWorkItem(LPVOID Parameter)
{
	HideConsoleTrace(L"CleanupWorkItem");
	CleanupHideConsole(Parameter);

	return 0;
}

VOID CALLBACK OnThreadExited(PVOID Parameter, BOOLEAN TimerOrWaitFired)
{
	HideConsoleTrace(L"OnThreadExited");

	if (!QueueUserWorkItem(CleanupWorkItem, Parameter, WT_EXECUTEDEFAULT))
	{
		HideConsoleTraceLastError(L"OnThreadExited: QueueUserWorkItem");
	}
}

BOOL WINAPI EnableForWindow(HWND hWnd)
{
	HideConsoleTrace(L"EnableForWindow: hWnd=0x%1!p!", hWnd);

	if (!hWnd)
		return FALSE;

#ifndef _WIN64

	BOOL IsWow64;

	if (!IsWow64Process(GetCurrentProcess(), &IsWow64))
	{
		HideConsoleTraceLastError(L"EnableForWindow: IsWow64Process");
		return FALSE;
	}

	if (IsWow64)
	{
		HideConsoleTrace(L"EnableForWindow: Launching Wow64Helper");
		return LaunchWow64Helper(ConsoleWindow);
	}

#endif // ! _WIN64

	PHIDE_CONSOLE HideConsole = SetupHideConsole(hWnd);

	if (!HideConsole)
	{
		return FALSE;
	}

	// Pin our library in the calling process; we need to keep the OnThreadExited
	// callback around even if our caller decides to unload us (e.g. AppDomain
	// unload in CLR), and we can't FreeLibrary() safely from that callback
	// anyway.

	// Other option would be to use a thread and wait for the ConHost to exit,
	// but that would be pretty wasteful, given that our caller may realistically 
	// want to hide 20-30 of console windows.

	HMODULE PinnedModuleHandle;

	GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
		(LPCWSTR)SetupHideConsole,
		&PinnedModuleHandle
	);

	if (!PinnedModuleHandle)
	{
		HideConsoleTraceLastError(
			L"EnableForWindow: GetModuleHandleExW"
		);

		goto Cleanup;
	}

	BOOL RegisteredWait = RegisterWaitForSingleObject(
		&HideConsole->WaitHandle,
		HideConsole->ThreadHandle,
		OnThreadExited,
		HideConsole,
		INFINITE,
		WT_EXECUTEONLYONCE
	);

	if (!RegisteredWait)
	{
		HideConsoleTraceLastError(
			L"EnableForWindow: RegisterWaitForSingleObject"
		);

		goto Cleanup;
	}

	return TRUE;

Cleanup:

	if (HideConsole)
	{
		CleanupHideConsole(HideConsole);
	}

	return FALSE;
}