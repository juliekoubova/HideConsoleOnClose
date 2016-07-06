#include "../Shared/stdafx.h"

HMODULE g_ModuleHandle = NULL;

BOOL WINAPI _DllMainCRTStartup(HMODULE hModule, DWORD Reason, LPVOID Reserved)
{
	if (Reason == DLL_PROCESS_ATTACH) 
	{
		g_ModuleHandle = hModule;
	}

	return TRUE;
}
