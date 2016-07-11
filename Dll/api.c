#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"

BOOL WINAPI SendWow64HelperMessage(HWND ConsoleWindow);

VOID CALLBACK CleanupCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context)
{
	HideConsoleTrace(
		L"Instance=%1!p! Context=%2!p!",
		Instance,
		Context
	);

	PHIDE_CONSOLE HideConsole = Context;

	if (HideConsole->OurModuleHandle)
	{
		HideConsoleTrace(
			L"FreeLibraryWhenCallbackReturns OurModuleHandle=%1!p!",
			HideConsole->OurModuleHandle
		);

		FreeLibraryWhenCallbackReturns(
			Instance,
			HideConsole->OurModuleHandle
		);
	}

	if (HideConsole->ConhostWaitHandle)
	{
		HideConsoleTrace(L"UnregisterWaitEx (blocking)");

		BOOL WaitUnregistered = UnregisterWaitEx(
			HideConsole->ConhostWaitHandle,
			INVALID_HANDLE_VALUE // block until the wait is unregistered
		);

		if (!WaitUnregistered)
		{
			HideConsoleTraceLastError(L"UnregisterWaitEx");
		}
	}

	CleanupHideConsole(HideConsole, NULL);
}

VOID CALLBACK OnThreadExited(PVOID Parameter, BOOLEAN TimerOrWaitFired)
{
	HideConsoleTrace(
		L"Parameter=%1!p! TimerOrWaitFired=%2!u!",
		Parameter,
		TimerOrWaitFired
	);

	// Need to delegate the cleanup to another thread pool work item, because
	// 1) UnregisterWaitEx waits for the wait callback to complete, and 
	// 2) we need TP_CALLBACK_INSTANCE to FreeLibraryWhenCallbackReturns.

	BOOL Success = TrySubmitThreadpoolCallback(
		CleanupCallback,
		Parameter,
		NULL
	);

	if (!Success)
	{
		HideConsoleTraceLastError(
			L"TrySubmitThreadpoolCallback"
		);
	}
}

BOOL WINAPI EnableForWindow(HWND hWnd)
{
	HideConsoleTrace(L"hWnd=0x%1!p!", hWnd);

	if (!hWnd)
	{
		SetLastError(ERROR_SUCCESS);
		return FALSE;
	}

	if (!HIDE_CONSOLE_WIN64)
	{
		BOOL IsWow64;

		if (!IsWow64Process(GetCurrentProcess(), &IsWow64))
		{
			HideConsoleTraceLastError(L"IsWow64Process");
			return FALSE;
		}

		if (IsWow64)
		{
			HideConsoleTrace(L"Running in WOW64, delegating to Wow64Helper");
			return SendWow64HelperMessage(hWnd);
		}
	}

	PHIDE_CONSOLE HideConsole = SetupHideConsole(hWnd);

	if (!HideConsole)
	{
		return FALSE;
	}

	BOOL AddedModuleRef = GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCWSTR)EnableForWindow,
		&HideConsole->OurModuleHandle
	);

	if (!AddedModuleRef)
	{
		HideConsoleTraceLastError(
			L"GetModuleHandleExW"
		);

		goto Cleanup;
	}

	BOOL RegisteredWait = RegisterWaitForSingleObject(
		&HideConsole->ConhostWaitHandle,
		HideConsole->ConhostThreadHandle,
		OnThreadExited,
		HideConsole,
		INFINITE,
		WT_EXECUTEONLYONCE
	);

	if (!RegisteredWait)
	{
		HideConsoleTraceLastError(
			L"RegisterWaitForSingleObject"
		);

		goto Cleanup;
	}

	return TRUE;

Cleanup:

	if (HideConsole)
	{
		DWORD LastError = GetLastError();

		if (HideConsole->OurModuleHandle)
		{
			HideConsoleTrace(L"Freeing our additional module handle");

			if (!FreeLibrary(HideConsole->OurModuleHandle))
			{
				HideConsoleTraceLastError(L"FreeLibrary");
			}
		}

		CleanupHideConsole(HideConsole, NULL);

		SetLastError(LastError);
	}

	return FALSE;
}