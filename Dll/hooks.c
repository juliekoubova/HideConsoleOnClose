#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"
#include "dll.h"
#include "hooks.h"

static LONG g_HookCount = 0;

static BOOL WINAPI IsConsoleWindow(HWND hWnd)
{
	static const WCHAR ConsoleWindowClass[] = L"ConsoleWindowClass";

	HideConsoleTrace(L"hWnd=%1!p!", hWnd);

	WCHAR ClassName[20];
	INT32 ClassNameCch = GetClassNameW(hWnd, ClassName, ARRAYSIZE(ClassName));

	if (ClassNameCch == 0)
	{
		HideConsoleTraceLastError(L"GetClassNameW");
		return FALSE;
	}

	if (ClassNameCch != ARRAYSIZE(ConsoleWindowClass) - 1)
	{
		HideConsoleTrace(
			L"hWnd=%1!p! ClassName=%2 Result=FALSE",
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
		L"hWnd=%1!p! ClassName=%2 Result=%3",
		hWnd,
		ClassName,
		(CompareResult == CSTR_EQUAL) ? L"TRUE" : L"FALSE"
	);

	return (CompareResult == CSTR_EQUAL);
}

static LRESULT CALLBACK HookCbt(INT32 Code, WPARAM wParam, LPARAM lParam)
{
	if ((Code != HCBT_SYSCOMMAND) || (wParam != SC_CLOSE))
	{
		goto NextHook;
	}

	HideConsoleTrace(L"HCBT_SYSCOMMAND SC_CLOSE");

	HWND ConsoleWindowBeingClosed = TlsGetValue(
		g_TlsIndex
	);

	HideConsoleTrace(
		L"ConsoleWindowBeingClosed=%1!p!",
		ConsoleWindowBeingClosed
	);

	if (!ConsoleWindowBeingClosed)
	{
		goto NextHook;
	}

	ShowWindow(ConsoleWindowBeingClosed, SW_HIDE);

	HideConsoleTrace(L"Result=1");
	return 1; // prevent action

NextHook:

	return CallNextHookEx(NULL, Code, wParam, lParam);
}

static LRESULT CALLBACK HookWndProc(INT32 Code, WPARAM wParam, LPARAM lParam)
{
	if (Code != HC_ACTION)
	{
		goto NextHook;
	}

	LPCWPSTRUCT Msg = (LPCWPSTRUCT)lParam;

	if ((Msg->message != WM_SYSCOMMAND) || (Msg->wParam != SC_CLOSE))
	{
		goto NextHook;
	}

	HideConsoleTrace(
		L"WM_SYSCOMMAND SC_CLOSE hWnd=%1!p!",
		Msg->hwnd
	);

	if (!IsConsoleWindow(Msg->hwnd))
	{
		goto NextHook;
	}

	if (!TlsSetValue(g_TlsIndex, Msg->hwnd))
	{
		HideConsoleTraceLastError(L"TlsSetValue");
		goto NextHook;
	}

NextHook:

	return CallNextHookEx(NULL, Code, wParam, lParam);
}

static LRESULT CALLBACK HookWndProcRet(INT32 Code, WPARAM wParam, LPARAM lParam)
{
	if (Code != HC_ACTION)
	{
		goto NextHook;
	}

	LPCWPRETSTRUCT Msg = (LPCWPRETSTRUCT)lParam;

	if ((Msg->message != WM_SYSCOMMAND) || (Msg->wParam != SC_CLOSE))
	{
		goto NextHook;
	}

	HideConsoleTrace(
		L"WM_SYSCOMMAND SC_CLOSE hWnd=%1!p!",
		Msg->hwnd
	);

	HWND ConsoleWindowBeingClosed = TlsGetValue(g_TlsIndex);

	HideConsoleTrace(
		L"ConsoleWindowBeingClosed=%1!p!",
		ConsoleWindowBeingClosed
	);

	if (ConsoleWindowBeingClosed != Msg->hwnd)
	{
		goto NextHook;
	}

	if (!TlsSetValue(g_TlsIndex, NULL))
	{
		HideConsoleTraceLastError(L"TlsSetValue");
		goto NextHook;
	}

NextHook:

	return CallNextHookEx(NULL, Code, wParam, lParam);
}

static LRESULT CALLBACK HookGetMessage(INT32 Code, WPARAM wParam, LPARAM lParam)
{
	if (Code != HC_ACTION)
	{
		goto NextHook;
	}

	LPMSG Msg = (LPMSG)lParam;

	if ((Msg->message != WM_SYSCOMMAND) || (Msg->wParam != SC_CLOSE))
	{
		goto NextHook;
	}

	HideConsoleTrace(
		L"WM_SYSCOMMAND SC_CLOSE hWnd=%1!p!",
		Msg->hwnd
	);

	if (!IsConsoleWindow(Msg->hwnd))
	{
		goto NextHook;
	}

	Msg->message = WM_NULL;
	ShowWindow(Msg->hwnd, SW_HIDE);

	HideConsoleTrace(
		L"WM_SYSCOMMAND changed into WM_NULL"
	);

NextHook:

	return CallNextHookEx(NULL, Code, wParam, lParam);
}

BOOL WINAPI UnhookHideConsole(PHIDE_CONSOLE_HOOKS Hooks, PBOOL WasLastHook)
{
	HideConsoleTrace(
		L"Hooks=%1!p! WasLastHook=%2!p!",
		Hooks,
		WasLastHook
	);

	if (!Hooks)
		return FALSE;

	LONG HookCount = InterlockedDecrement(&g_HookCount);

	HideConsoleTrace(L"HookCount=%1!i!", HookCount);

	if (WasLastHook)
	{
		*WasLastHook = (HookCount == 0) ? TRUE : FALSE;
	}

	BOOL Result = TRUE;

	if (Hooks->GetMessageHook)
	{
		if (!UnhookWindowsHookEx(Hooks->GetMessageHook) &&
			(GetLastError() != ERROR_SUCCESS))
		{
			HideConsoleTraceLastError(
				L"UnhookWindowsHookEx(GetMessageHook)"
			);

			Result = FALSE;
		}
	}

	if (Hooks->WndProcRetHook)
	{
		if (!UnhookWindowsHookEx(Hooks->WndProcRetHook) &&
			(GetLastError() != ERROR_SUCCESS))
		{
			HideConsoleTraceLastError(
				L"UnhookWindowsHookEx(WndProcRetHook)"
			);

			Result = FALSE;
		}
	}

	if (Hooks->WndProcHook)
	{
		if (!UnhookWindowsHookEx(Hooks->WndProcHook) &&
			(GetLastError() != ERROR_SUCCESS))
		{
			HideConsoleTraceLastError(
				L"UnhookWindowsHookEx(WndProcHook)"
			);

			Result = FALSE;
		}
	}

	if (Hooks->CbtHook)
	{
		if (!UnhookWindowsHookEx(Hooks->CbtHook) &&
			(GetLastError() != ERROR_SUCCESS))
		{
			HideConsoleTraceLastError(
				L"UnhookWindowsHookEx(CbtHook)"
			);

			Result = FALSE;
		}
	}

	return Result;
}

BOOL WINAPI SetHideConsoleHooks(PHIDE_CONSOLE_HOOKS Hooks, DWORD ThreadId)
{
	HideConsoleTrace(L"Hooks=%1!p! ThreadId=%2!u!", Hooks, ThreadId);

	HideConsoleAssert(Hooks != NULL);
	HideConsoleAssert(ThreadId != 0);

	if (!Hooks || !ThreadId)
	{
		return FALSE;
	}

	LONG HookCount = InterlockedIncrement(&g_HookCount);

	HideConsoleTrace(L"HookCount=%1!i!", HookCount);

	Hooks->CbtHook = SetWindowsHookExW(
		WH_CBT,
		HookCbt,
		g_ModuleHandle,
		ThreadId
	);

	if (!Hooks->CbtHook)
	{
		HideConsoleTraceLastError(L"SetWindowsHookExW(WH_CBT)");
		goto Cleanup;
	}

	Hooks->WndProcHook = SetWindowsHookExW(
		WH_CALLWNDPROC,
		HookWndProc,
		g_ModuleHandle,
		ThreadId
	);

	if (!Hooks->WndProcHook)
	{
		HideConsoleTraceLastError(L"SetWindowsHookExW(WH_CALLWNDPROC)");
		goto Cleanup;
	}

	Hooks->WndProcRetHook = SetWindowsHookExW(
		WH_CALLWNDPROCRET,
		HookWndProcRet,
		g_ModuleHandle,
		ThreadId
	);

	if (!Hooks->WndProcRetHook)
	{
		HideConsoleTraceLastError(L"SetWindowsHookExW(WH_CALLWNDPROCRET)");
		goto Cleanup;
	}

	Hooks->GetMessageHook = SetWindowsHookExW(
		WH_GETMESSAGE,
		HookGetMessage,
		g_ModuleHandle,
		ThreadId
	);

	if (!Hooks->GetMessageHook)
	{
		HideConsoleTraceLastError(L"SetWindowsHookExW(WH_GETMESSAGE)");
		goto Cleanup;
	}

	return TRUE;

Cleanup:

	UnhookHideConsole(Hooks, NULL);
	return FALSE;
}

LONG WINAPI GetHookCount(VOID)
{
	return g_HookCount;
}
