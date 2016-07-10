#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"

LRESULT WINAPI MessageWndProc(
	HWND   Window,
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam
)
{
	if (Message == WM_HIDE_CONSOLE)
	{
		HWND ConsoleWindow = wParam;

		HideConsoleTrace(
			L"WmHideConsole: ConsoleWindow=%1!p!",
			ConsoleWindow
		);

		return EnableForWindow(ConsoleWindow);
	}

	return DefWindowProcW(Window, Message, wParam, lParam);
}

INT WINAPI wWinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPWSTR    CommandLine,
	INT       CommandShow
)
{
	HideConsoleTrace(
		L"wWinMain: Instance=%1!p!",
		Instance
	);

	WNDCLASSEXW WindowClass;
	WindowClass.cbSize = sizeof(WindowClass);
	WindowClass.lpszClassName = L"HideConsoleOnCloseWow64Helper";
	WindowClass.lpfnWndProc = MessageWndProc;

	WindowClass.style = CS_CLASSDC;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hbrBackground = NULL;
	WindowClass.hCursor = NULL;
	WindowClass.hInstance = Instance;
	WindowClass.hIcon = NULL;
	WindowClass.hIconSm = NULL;
	WindowClass.lpszMenuName = NULL;

	ATOM WindowClassAtom = RegisterClassExW(&WindowClass);

	if (!WindowClassAtom)
	{
		HideConsoleTraceLastError(L"wWinMain: RegisterClassExW");
		return 1;
	}

	HWND MessageWindow = CreateWindowExW(
		0,
		WindowClassAtom,
		NULL,
		WS_OVERLAPPED,
		0, 0, 0, 0,
		HWND_MESSAGE,
		NULL,
		Instance,
		NULL
	);

	if (!MessageWindow)
	{
		HideConsoleTraceLastError(L"wWinMain: CreateWindowExW");
		return 1;
}

	MSG  Message;
	BOOL Result;

	while ((Result = GetMessageW(&Message, NULL, 0, 0)))
	{
		if (Result == -1)
		{
			HideConsoleTraceLastError("wWinMain: GetMessageW");
			return 1;
		}

		DispatchMessageW(&Message);
	}

	return 0;
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
