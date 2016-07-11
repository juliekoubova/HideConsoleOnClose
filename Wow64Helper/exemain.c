#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"

HANDLE g_ReceivingMessagesEventHandle = NULL;

LRESULT WINAPI MessageWndProc(
	HWND   Window,
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam
)
{
	if (Message == WM_HIDE_CONSOLE)
	{
		HWND ConsoleWindow = (HWND)wParam;

		HideConsoleTrace(
			L"WM_HIDE_CONSOLE: ConsoleWindow=%1!p!",
			ConsoleWindow
		);

		return EnableForWindow(ConsoleWindow);
	}

	if (Message == WM_CREATE)
	{
		if (!SetEvent(g_ReceivingMessagesEventHandle))
		{
			HideConsoleTraceLastError(L"WM_CREATE: SetEvent");
			return -1;
		}

		return 0;
	}

	if (Message == WM_DESTROY)
	{
		if (ResetEvent(g_ReceivingMessagesEventHandle))
		{
			HideConsoleTrace(L"WM_DESTROY: Calling PostQuitMessage(0)");
			PostQuitMessage(0);
		}
		else
		{
			HideConsoleTraceLastError(L"ResetEvent");
			HideConsoleTrace(L"WM_DESTROY: Calling PostQuitMessage(1)");
			PostQuitMessage(1);
		}

		return 0;
	}

	return DefWindowProcW(Window, Message, wParam, lParam);
}

HWND WINAPI CreateMessageWindow(HINSTANCE Instance)
{
	WNDCLASSW WindowClass;
	WindowClass.lpfnWndProc = MessageWndProc;
	WindowClass.lpszClassName = WOW64HELPER_WINDOW_CLASS;
	WindowClass.style = CS_CLASSDC;

	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hbrBackground = NULL;
	WindowClass.hCursor = NULL;
	WindowClass.hInstance = Instance;
	WindowClass.hIcon = NULL;
	WindowClass.lpszMenuName = NULL;

	ATOM WindowClassAtom = RegisterClassW(&WindowClass);

	if (!WindowClassAtom)
	{
		HideConsoleTraceLastError(L"RegisterClassW");
		return NULL;
	}

	HWND Window = CreateWindowExW(
		/* dwExStyle           */ 0,
		/* lpClassName         */ (LPCWSTR)WindowClassAtom,
		/* lpWindowName        */ NULL,
		/* dwStyle             */ WS_OVERLAPPED,
		/* X, Y, Width, Height */ 0, 0, 0, 0,
		/* hWndParent          */ HWND_MESSAGE,
		/* hMenu               */ NULL,
		/* hInstance           */ Instance,
		/* lpParam             */ NULL
	);

	if (!Window)
	{
		HideConsoleTraceLastError(L"CreateWindowExW");
	}

	return Window;
}

INT WINAPI wWinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPWSTR    CommandLine,
	INT       CommandShow
)
{
	HideConsoleTrace(L"Instance=%1!p!", Instance);

	HideConsoleTrace(L"CreateMutexW Name='" WOW64HELPER_MUTEX L"'");

	HANDLE MutexHandle = CreateMutexW(
		/* lpMutexAttributes */ NULL,
		/* bInitialOwner     */ FALSE,
		/* lpName            */ WOW64HELPER_MUTEX
	);

	if (!MutexHandle)
	{
		HideConsoleTraceLastError(L"CreateMutexW");
		return 1;
	}

	DWORD WaitResult = WaitForSingleObject(MutexHandle, 0);

	if (WaitResult == WAIT_FAILED)
	{
		HideConsoleTraceLastError(L"WaitForSingleObject");
		return 1;
	}

	if (WaitResult == WAIT_TIMEOUT)
	{
		HideConsoleTrace(
			L"WaitForSingleObject: WAIT_TIMEOUT, Wow64Helper is already "
			L"running"
		);

		return 42;
	}

	if (WaitResult == WAIT_ABANDONED)
	{
		HideConsoleTrace(
			L"WaitForSingleObject: WAIT_ABANDONED, previous Wow64Helper "
			L"must've just crashed, continuing"
		);
	}

	HideConsoleTrace(L"CreateEventW Name='" WOW64HELPER_READY_EVENT L"'");

	g_ReceivingMessagesEventHandle = CreateEventW(
		/* lpEventAttributes */ NULL,
		/* bManualReset      */ TRUE,
		/* bInitialState     */ FALSE,
		/* lpName            */ WOW64HELPER_READY_EVENT
	);

	if (!g_ReceivingMessagesEventHandle)
	{
		HideConsoleTraceLastError(L"CreateEventW");
		return 1;
	}

	// Reset the event in case other Wow64Helper instance crashed with the
	// event signaled and some other process has kept it open.

	if (!ResetEvent(g_ReceivingMessagesEventHandle))
	{
		HideConsoleTraceLastError(L"ResetEvent");
		return 1;
	}

	if (!CreateMessageWindow(Instance))
	{
		return 1;
	}

	MSG  Message;
	BOOL Result;

	while ((Result = GetMessageW(&Message, NULL, 0, 0)))
	{
		if (Result == -1)
		{
			HideConsoleTraceLastError(L"GetMessageW");
			return 1;
		}

		DispatchMessageW(&Message);
	}

	return (INT)Message.wParam;
}

INT WINAPI wWinMainCRTStartup(VOID)
{
	INT ExitCode = wWinMain(
		GetModuleHandleW(NULL),
		NULL,
		NULL,
		0
	);

	ExitProcess(ExitCode);
	return ExitCode;
}
