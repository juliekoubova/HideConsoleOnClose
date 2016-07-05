#include "stdafx.h"
#include "HideConsoleOnClose.h"

HMODULE g_ModuleHandle = NULL;

BOOL WINAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID Reserved)
{
	if (Reason == DLL_PROCESS_ATTACH) 
	{
		g_ModuleHandle = hModule;
	}

	return TRUE;
}
