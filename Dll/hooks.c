#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"
#include "dll.h"

#define ConsoleWindowClass L"ConsoleWindowClass"

DWORD WINAPI FindConhostUIThreadId(HWND ConsoleWindow);

BOOL WINAPI IsConsoleWindow(HWND hWnd)
{
	HideConsoleTrace(
		L"IsConsoleWindow: hWnd=%1!p!",
		hWnd
	);

	WCHAR ClassName[20];
	INT32 ClassNameCch = GetClassNameW(hWnd, ClassName, ARRAYSIZE(ClassName));

	if (ClassNameCch == 0)
	{
		HideConsoleTraceLastError(L"IsConsoleWindow: GetClassNameW");
		return FALSE;
	}

	if (ClassNameCch != ARRAYSIZE(ConsoleWindowClass) - 1)
	{
		HideConsoleTrace(
			L"IsConsoleWindow: hWnd=%1!p! ClassName=%2 Result=FALSE",
			hWnd,
			ClassName
		);

		return FALSE;
	}

	INT32 CompareResult = CompareStringW(
		LOCALE_INVARIANT,
		0,
		ClassName,
		ClassNameCch,
		ConsoleWindowClass,
		ARRAYSIZE(ConsoleWindowClass) - 1
	);

	HideConsoleTrace(
		L"IsConsoleWindow: hWnd=%1!p! ClassName=%2 Result=%3!s!",
		hWnd,
		ClassName,
		(CompareResult == CSTR_EQUAL) ? L"TRUE" : L"FALSE"
	);

	return (CompareResult == CSTR_EQUAL);
}

LRESULT CALLBACK HookCbt(INT32 nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode == HCBT_SYSCOMMAND) && (wParam == SC_CLOSE))
	{
		HideConsoleTrace(L"HookCbt: HCBT_SYSCOMMAND SC_CLOSE");

		HWND ConsoleWindowBeingClosed = TlsGetValue(
			g_TlsIndex
		);

		HideConsoleTrace(
			L"HookCbt: ConsoleWindowBeingClosed=%1!p!",
			ConsoleWindowBeingClosed
		);

		if (!ConsoleWindowBeingClosed)
		{
			goto NextHook;
		}

		if (!ShowWindow(ConsoleWindowBeingClosed, SW_HIDE))
		{
			HideConsoleTraceLastError(L"HookCbt: ShowWindow");
		}

		HideConsoleTrace(L"HookCbt: Result=1");
		return 1; // prevent action
	}

NextHook:

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK HookWndProc(INT32 nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		LPCWPSTRUCT Msg = (LPCWPSTRUCT)lParam;

		if ((Msg->message == WM_SYSCOMMAND) && (Msg->wParam == SC_CLOSE))
		{
			HideConsoleTrace(
				L"HookWndProc: WM_SYSCOMMAND SC_CLOSE hWnd=%1!p!",
				Msg->hwnd
			);

			if (!IsConsoleWindow(Msg->hwnd))
			{
				goto NextHook;
			}

			if (!TlsSetValue(g_TlsIndex, Msg->hwnd))
			{
				HideConsoleTraceLastError(L"HookWndProc: TlsSetValue");
				goto NextHook;
			}
		}
	}

NextHook:

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK HookWndProcRet(INT32 nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		LPCWPRETSTRUCT Msg = (LPCWPRETSTRUCT)lParam;

		if ((Msg->message == WM_SYSCOMMAND) && (Msg->wParam == SC_CLOSE))
		{
			HideConsoleTrace(
				L"HookWndProcRet: WM_SYSCOMMAND SC_CLOSE hWnd=%1!p!",
				Msg->hwnd
			);

			HWND ConsoleWindowBeingClosed = TlsGetValue(g_TlsIndex);

			HideConsoleTrace(
				L"HookWndProcRet: ConsoleWindowBeingClosed=%1!p!",
				ConsoleWindowBeingClosed
			);

			if (ConsoleWindowBeingClosed != Msg->hwnd)
			{
				goto NextHook;
			}

			if (!TlsSetValue(g_TlsIndex, NULL))
			{
				HideConsoleTraceLastError(L"HookWndProcRet: TlsSetValue");
				goto NextHook;
			}
		}
	}

NextHook:

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL WINAPI CleanupHideConsole(PHIDE_CONSOLE HideConsole)
{
	HideConsoleTrace(
		L"CleanupHideConsole: HideConsole=%1!p!",
		HideConsole
	);

	if (!HideConsole)
		return FALSE;

	if (HideConsole->WaitHandle)
	{
		HideConsoleTrace(
			L"CleanupHideConsole: UnregisterWaitEx (blocking)"
		);

		UnregisterWaitEx(
			HideConsole->WaitHandle,
			INVALID_HANDLE_VALUE // block until the wait is unregistered
		);
	}

	if (HideConsole->ThreadHandle)
		CloseHandle(HideConsole->ThreadHandle);

	if (HideConsole->WndProcRet)
		UnhookWindowsHookEx(HideConsole->WndProcRet);

	if (HideConsole->WndProc)
		UnhookWindowsHookEx(HideConsole->WndProc);

	if (HideConsole->Cbt)
		UnhookWindowsHookEx(HideConsole->Cbt);

	return HeapFree(GetProcessHeap(), 0, HideConsole);
}

PHIDE_CONSOLE WINAPI SetupHideConsole(HWND hWnd)
{
	HideConsoleTrace(
		L"SetupHideConsole: hWnd=%1!p!",
		hWnd
	);

	if (!hWnd)
	{
		return NULL;
	}

	DWORD ThreadId = FindConhostUIThreadId(hWnd);

	if (!ThreadId)
	{
		HideConsoleTrace(
			L"SetupHideConsole: Thread for hWnd 0x%1!p! could not be found",
			hWnd
		);

		return NULL;
	}

	PHIDE_CONSOLE Result = HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(HIDE_CONSOLE)
	);
	
	if (!Result)
	{
		HideConsoleTraceLastError(L"SetupHideConsole: HeapAlloc");
		return NULL;
	}

	Result->ThreadHandle = OpenThread(SYNCHRONIZE, FALSE, ThreadId);

	if (!Result->ThreadHandle)
	{
		HideConsoleTraceLastError(L"SetupHideConsole: OpenThread");
		goto Cleanup;
	}

	Result->Cbt = SetWindowsHookExW(
		WH_CBT,
		HookCbt,
		g_ModuleHandle,
		ThreadId
	);

	if (!Result->Cbt)
	{
		HideConsoleTraceLastError(
			L"SetupHideConsole: SetWindowsHookExW(WH_CBT)"
		);

		goto Cleanup;
	}

	Result->WndProc = SetWindowsHookExW(
		WH_CALLWNDPROC,
		HookWndProc,
		g_ModuleHandle,
		ThreadId
	);

	if (!Result->WndProc)
	{
		HideConsoleTraceLastError(
			L"SetupHideConsole: SetWindowsHookExW(WH_CALLWNDPROC)"
		);

		goto Cleanup;
	}

	Result->WndProcRet = SetWindowsHookExW(
		WH_CALLWNDPROCRET,
		HookWndProcRet,
		g_ModuleHandle,
		ThreadId
	);

	if (!Result->WndProcRet)
	{
		HideConsoleTraceLastError(
			L"SetupHideConsole: SetWindowsHookExW(WH_CALLWNDPROCRET)"
		);

		goto Cleanup;
	}

	return Result;

Cleanup:
	
	CleanupHideConsole(Result);
	return NULL;
}
