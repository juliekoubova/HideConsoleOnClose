#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"

BOOL
WINAPI
LaunchWow64Helper(HWND ConsoleWindow);

VOID CALLBACK CleanupCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context)
{
	HideConsoleTrace(L"CleanupCallback");

	PHIDE_CONSOLE HideConsole = Context;

	if (HideConsole->OurModuleHandle)
	{
		HideConsoleTrace(
			L"CleanupCallback: FreeLibraryWhenCallbackReturns "
			L"OurModuleHandle=%1!p!",
			HideConsole->OurModuleHandle
		);

		FreeLibraryWhenCallbackReturns(
			Instance,
			HideConsole->OurModuleHandle
		);
	}

	if (HideConsole->ConhostWaitHandle)
	{
		HideConsoleTrace(
			L"CleanupCallback: UnregisterWaitEx (blocking)"
		);

		BOOL WaitUnregistered = UnregisterWaitEx(
			HideConsole->ConhostWaitHandle,
			INVALID_HANDLE_VALUE // block until the wait is unregistered
		);

		if (!WaitUnregistered)
		{
			HideConsoleTraceLastError(
				L"CleanupCallback: UnregisterWaitEx"
			);
		}
	}

	CleanupHideConsole(HideConsole, NULL);
}

VOID CALLBACK OnThreadExited(PVOID Parameter, BOOLEAN TimerOrWaitFired)
{
	HideConsoleTrace(L"OnThreadExited");

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
			L"OnThreadExited: TrySubmitThreadpoolCallback"
		);
	}
}

BOOL WINAPI EnableForWindow(HWND hWnd)
{
	HideConsoleTrace(L"EnableForWindow: hWnd=0x%1!p!", hWnd);

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
			HideConsoleTraceLastError(L"EnableForWindow: IsWow64Process");
			return FALSE;
		}

		if (IsWow64)
		{
			HideConsoleTrace(L"EnableForWindow: Launching Wow64Helper");
			return LaunchWow64Helper(hWnd);
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
			L"EnableForWindow: GetModuleHandleExW"
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
			L"EnableForWindow: RegisterWaitForSingleObject"
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
			HideConsoleTrace(
				L"EnableForWindow: Freeing our additional module handle"
			);

			if (!FreeLibrary(HideConsole->OurModuleHandle))
			{
				HideConsoleTraceLastError(L"EnableForWindow: FreeLibrary");
			}
		}

		CleanupHideConsole(HideConsole, NULL);

		SetLastError(LastError);
	}

	return FALSE;
}