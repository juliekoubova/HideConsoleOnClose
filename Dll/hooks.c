#include "../Shared/stdafx.h"
#include "../Shared/api.h"

#define ConsoleWindowClass L"ConsoleWindowClass"

HWND g_hConsoleWindowBeingClosed = NULL;

DWORD WINAPI FindConhostUIThreadId(HWND ConsoleWindow);

BOOL WINAPI IsConsoleWindow(HWND hWnd)
{
	WCHAR ClassName[20];
	INT32 ClassNameCch = GetClassNameW(hWnd, ClassName, ARRAYSIZE(ClassName));
	
	if (ClassNameCch > 0)
	{
		INT32 CompareResult = CompareStringW(
			LOCALE_INVARIANT,
			0,
			ClassName,
			ClassNameCch,
			ConsoleWindowClass,
			ARRAYSIZE(ConsoleWindowClass) - 1
		);

		return (CompareResult == CSTR_EQUAL);
	}

	return FALSE;
}

LRESULT CALLBACK HookCbt(INT32 nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode  == HCBT_SYSCOMMAND) && 
		(wParam == SC_CLOSE)        &&
		g_hConsoleWindowBeingClosed)
	{	
		ShowWindow(g_hConsoleWindowBeingClosed, SW_HIDE);
		return 1;
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK HookWndProc(INT32 nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		LPCWPSTRUCT lpCwp = (LPCWPSTRUCT)lParam;

		if ((lpCwp->message == WM_SYSCOMMAND) && 
			(lpCwp->wParam  == SC_CLOSE))
		{
			if (IsConsoleWindow(lpCwp->hwnd))
			{
				g_hConsoleWindowBeingClosed = lpCwp->hwnd;
			}
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK HookWndProcRet(INT32 nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		LPCWPRETSTRUCT lpCwpRet = (LPCWPRETSTRUCT)lParam;

		if ((lpCwpRet->message == WM_SYSCOMMAND) &&
			(lpCwpRet->wParam  == SC_CLOSE) &&
			(lpCwpRet->hwnd    == g_hConsoleWindowBeingClosed))
		{
			g_hConsoleWindowBeingClosed = NULL;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL WINAPI CleanupHideConsole(PHIDE_CONSOLE HideConsole)
{
	if (!HideConsole)
		return FALSE;

	if (HideConsole->WaitHandle)
	{
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

PHIDE_CONSOLE WINAPI SetupHideConsole(HWND ConsoleWindow)
{
	DWORD ThreadId = FindConhostUIThreadId(ConsoleWindow);

	if (!ThreadId)
		return NULL;

	PHIDE_CONSOLE Result = HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(HIDE_CONSOLE)
	);
	
	if (!Result)
		return NULL;

	Result->ThreadHandle = OpenThread(SYNCHRONIZE, FALSE, ThreadId);

	if (!Result->ThreadHandle)
		goto Cleanup;

	Result->Cbt = SetWindowsHookExW(
		WH_CBT,
		HookCbt,
		g_ModuleHandle,
		ThreadId
	);

	if (!Result->Cbt)
		goto Cleanup;

	Result->WndProc = SetWindowsHookExW(
		WH_CALLWNDPROC,
		HookWndProc,
		g_ModuleHandle,
		ThreadId
	);

	if (!Result->WndProc)
		goto Cleanup;

	Result->WndProcRet = SetWindowsHookExW(
		WH_CALLWNDPROCRET,
		HookWndProcRet,
		g_ModuleHandle,
		ThreadId
	);

	if (!Result->WndProcRet)
		goto Cleanup;

	return Result;

Cleanup:
	
	CleanupHideConsole(Result);
	return NULL;
}
