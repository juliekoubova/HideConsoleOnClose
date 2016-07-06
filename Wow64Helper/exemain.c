#include "../Shared/stdafx.h"
#include "../Shared/hexstr.h"
#include "../Shared/api.h"

INT WINAPI wWinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPWSTR    CommandLine,
	INT       CommandShow
)
{
#ifdef _DEBUG
	MessageBoxW(NULL, CommandLine, L"HideConsoleOnClose", 0);
#endif

	if (*CommandLine == 0)
		return 1;
	
	DWORD ConsoleWindow = 0;
	
	if (!HexToDWord(CommandLine, &ConsoleWindow))
		return 1;

	if (!ConsoleWindow)
		return 1;

	PHIDE_CONSOLE HideConsole = SetupHideConsole(
		(HWND)(DWORD_PTR)ConsoleWindow
	);

	if (!HideConsole)
		return 1;

	WaitForSingleObject(HideConsole->ThreadHandle, INFINITE);

	CleanupHideConsole(HideConsole);

	return 0;
}

INT WINAPI wWinMainCRTStartup(VOID)
{
	LPWSTR CommandLine = GetCommandLineW();

	INT ExitCode = wWinMain(
		NULL,
		NULL,
		GetCommandLineW(),
		0
	);

	ExitProcess(ExitCode);
	return ExitCode;
}
