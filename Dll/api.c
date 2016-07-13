#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"
#include "hooks.h"

static HWND g_WindowToBeClosed = NULL;

DWORD WINAPI FindConhostUIThreadId(HWND ConsoleWindow);
BOOL  WINAPI SendWow64HelperMessage(HWND ConsoleWindow);

static PHIDE_CONSOLE WINAPI AllocHideConsole(DWORD WaitsCount)
{
	HideConsoleTrace(L"WaitsCount=%1!u!", WaitsCount);

	HANDLE ProcessHeap = GetProcessHeap();

	if (!ProcessHeap)
	{
		HideConsoleTraceLastError(L"GetProcessHeap");
		return NULL;
	}

	PHIDE_CONSOLE Result = HeapAlloc(
		ProcessHeap,
		HEAP_ZERO_MEMORY,
		sizeof(HIDE_CONSOLE) +
		sizeof(HIDE_CONSOLE_WAIT) * (WaitsCount - 1)
	);

	if (!Result)
	{
		HideConsoleTraceLastError(L"HeapAlloc");
		return NULL;
	}

	Result->WaitsCount = WaitsCount;
	return Result;
}

static BOOL WINAPI FreeHideConsole(PHIDE_CONSOLE HideConsole)
{
	HideConsoleTrace(L"HideConsole=%1!p!", HideConsole);
	HideConsoleAssert(HideConsole != NULL);

	if (!HideConsole)
	{
		return FALSE;
	}

	HANDLE ProcessHeap = GetProcessHeap();

	if (!ProcessHeap)
	{
		HideConsoleTraceLastError(L"GetProcessHeap");
		return FALSE;
	}

	if (!HeapFree(ProcessHeap, 0, HideConsole))
	{
		HideConsoleTraceLastError(L"HeapFree");
		return FALSE;
	}

	return TRUE;
}

static BOOL WINAPI UnregisterObjectWaits(PHIDE_CONSOLE HideConsole)
{
	HideConsoleTrace(L"HideConsole=%1!p!", HideConsole);
	HideConsoleAssert(HideConsole != NULL);

	if (!HideConsole)
	{
		return FALSE;
	}

	HideConsoleTrace(
		L"HideConsole->WaitsCount=%1!u!",
		HideConsole->WaitsCount
	);

	BOOL Result = TRUE;

	for (DWORD Index = 0; Index < HideConsole->WaitsCount; Index++)
	{
		// Atomically replace the wait handle with NULL because another wait
		// might complete too and we don't want to unregister the same wait
		// twice.

		HANDLE WaitHandle = InterlockedExchangePointer(
			&HideConsole->Waits[Index].Wait,
			NULL
		);

		// No need to replace the object handle. If we got the wait handle, we
		// are both unregistering the wait and closing the object handle.

		HANDLE ObjectHandle = HideConsole->Waits[Index].Object;

		HideConsoleTrace(
			L"[%1!u!]: WaitHandle=%2!p! ObjectHandle=%3!p!",
			Index,
			WaitHandle,
			ObjectHandle
		);

		if (WaitHandle)
		{
			// INVALID_HANDLE_VALUE means block until the wait is unregistered

			BOOL WaitUnregistered = UnregisterWaitEx(
				WaitHandle,
				INVALID_HANDLE_VALUE 
			);

			if (!WaitUnregistered)
			{
				HideConsoleTraceLastError(L"UnregisterWaitEx");
				Result = FALSE;
			}

			if (!CloseHandle(ObjectHandle))
			{
				HideConsoleTraceLastError(L"CloseHandle");
				Result = FALSE;
			}
		}
	}

	return Result;
}

//
// Unregisters the waits, unhooks the hooks, and frees
// the bookkeeping structure.
//
// Must not be called from the registered wait callback because 
// UnregisterWaitEx waits for the callback to complete.
//
static VOID CALLBACK CleanupCallback(PTP_CALLBACK_INSTANCE Callback, PVOID Context)
{
	HideConsoleTrace(
		L"Callback=%1!p! Context=%2!p!",
		Callback,
		Context
	);

	PHIDE_CONSOLE HideConsole = Context;

	if (HideConsole->Module)
	{
		HideConsoleTrace(
			L"FreeLibraryWhenCallbackReturns OurModuleHandle=%1!p!",
			HideConsole->Module
		);

		FreeLibraryWhenCallbackReturns(
			Callback,
			HideConsole->Module
		);
	}

	UnregisterObjectWaits(HideConsole);

	BOOL WasLastHook = FALSE;
	UnhookHideConsole(&HideConsole->Hooks, &WasLastHook);

	HWND WindowToBeClosed = g_WindowToBeClosed;

	HideConsoleTrace(
		L"WasLastHook=%1 WindowToBeClosed=%2!p!",
		WasLastHook ? L"TRUE" : L"FALSE",
		WindowToBeClosed
	);

	if (WindowToBeClosed && WasLastHook)
	{
		HideConsoleTrace(
			L"PostMessageW WM_CLOSE hWnd=%1!p!",
			WindowToBeClosed
		);

		if (!PostMessageW(WindowToBeClosed, WM_CLOSE, 0, 0))
		{
			HideConsoleTraceLastError(L"PostMessageW");
		}
	}
}

static VOID CALLBACK OnWaitCompleted(PVOID Parameter, BOOLEAN TimerOrWaitFired)
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
		HideConsoleTraceLastError(L"TrySubmitThreadpoolCallback");
	}
}

static BOOL WINAPI RegisterThreadWait(
	PHIDE_CONSOLE HideConsole,
	DWORD Index,
	DWORD ThreadId
)
{
	HideConsoleTrace(
		L"HideConsole=%1!p! Index=%2!u! ThreadId=%3!u!",
		HideConsole,
		Index,
		ThreadId
	);

	HideConsoleAssert(HideConsole != NULL);
	HideConsoleAssert(ThreadId != 0);
	HideConsoleAssert(Index < HideConsole->WaitsCount);

	HideConsole->Waits[Index].Object = OpenThread(
		SYNCHRONIZE,
		FALSE,
		ThreadId
	);

	if (!HideConsole->Waits[Index].Object)
	{
		HideConsoleTraceLastError(L"OpenThread");
		return FALSE;
	}

	BOOL RegisteredWait = RegisterWaitForSingleObject(
		&HideConsole->Waits[Index].Wait,
		HideConsole->Waits[Index].Object,
		OnWaitCompleted,
		HideConsole,
		INFINITE,
		WT_EXECUTEONLYONCE
	);

	if (!RegisteredWait)
	{
		HideConsoleTraceLastError(L"RegisterWaitForSingleObject");

		if (!CloseHandle(HideConsole->Waits[Index].Object))
		{
			HideConsoleTraceLastError(L"CloseHandle");
		}

		return FALSE;
	}

	return TRUE;
}

BOOL WINAPI EnableForWindowWithOwner(HWND hWnd, DWORD OwnerThreadId)
{
	HideConsoleTrace(L"hWnd=0x%1!p! OwnerThreadId=%2!u!", hWnd, OwnerThreadId);

	if (!hWnd)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

#ifndef _WIN64

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

#endif

	DWORD ThreadId = FindConhostUIThreadId(hWnd);

	if (!ThreadId)
	{
		HideConsoleTrace(
			L"Conhost UI thread for hWnd 0x%1!p! could not be found",
			hWnd
		);

		return FALSE;
	}

	DWORD WaitsCount = (OwnerThreadId == 0) ? 1 : 2;

	PHIDE_CONSOLE HideConsole = AllocHideConsole(WaitsCount);

	if (!HideConsole)
	{
		return FALSE;
	}

	BOOL Success = SetHideConsoleHooks(
		&HideConsole->Hooks,
		ThreadId
	);

	if (!Success)
	{
		goto Cleanup;
	}

	Success = GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(LPCWSTR)EnableForWindow,
		&HideConsole->Module
	);

	if (!Success)
	{
		HideConsoleTraceLastError(L"GetModuleHandleExW");
		goto Cleanup;
	}

	HideConsoleTrace(L"Register Conhost UI thread wait");

	if (!RegisterThreadWait(HideConsole, 0, ThreadId))
	{
		goto Cleanup;
	}

	if (OwnerThreadId)
	{
		HideConsoleTrace(L"Register owner thread wait");

		if (!RegisterThreadWait(HideConsole, 1, OwnerThreadId))
		{
			goto Cleanup;
		}
	}

	return TRUE;

Cleanup:

	if (HideConsole)
	{
		DWORD LastError = GetLastError();

		UnhookHideConsole(&HideConsole->Hooks, NULL);

		UnregisterObjectWaits(HideConsole);

		if (HideConsole->Module)
		{
			HideConsoleTrace(L"Freeing our additional module handle");

			if (!FreeLibrary(HideConsole->Module))
			{
				HideConsoleTraceLastError(L"FreeLibrary");
			}
		}

		FreeHideConsole(HideConsole);

		SetLastError(LastError);
	}

	return FALSE;
}

BOOL WINAPI EnableForWindow(HWND hWnd)
{
	HideConsoleTrace(L"hWnd=%1!p!", hWnd);

	return EnableForWindowWithOwner(
		hWnd,
		GetCurrentThreadId()
	);
}

BOOL WINAPI CloseWindowOnLastUnhook(HWND WindowToBeClosed)
{
	HideConsoleTrace(L"WindowToBeClosed=%1!p!", WindowToBeClosed);

	if (!WindowToBeClosed)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	HWND Current = InterlockedCompareExchangePointer(
		&g_WindowToBeClosed,
		WindowToBeClosed,
		NULL
	);

	if (Current != NULL)
	{
		HideConsoleTrace(L"WindowToBeClosed already set");
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}

	SetLastError(ERROR_SUCCESS);
	return TRUE;
}
