#include "../Shared/stdafx.h"
#include "../Shared/hexstr.h"
#include "../Shared/api.h"

INT CALLBACK WinMainCRTStartup(VOID)
{
	LPWSTR lpCmdLine = GetCommandLineW();

	MessageBoxW(NULL, lpCmdLine, L"Hello", 0);

	if (*lpCmdLine == 0)
		ExitProcess(1);
	
	DWORD ThreadId = 0;
	
	if (!HexToDWord(lpCmdLine, MAXSIZE_T, &ThreadId))
		ExitProcess(1);

	if (!ThreadId)
		ExitProcess(1);

	PHIDE_CONSOLE HideConsole = SetupHideConsole(ThreadId);

	if (!HideConsole)
		ExitProcess(1);

	WaitForSingleObject(HideConsole->ThreadHandle, INFINITE);

	CleanupHideConsole(HideConsole);

	ExitProcess(0);
	return 0;
}
