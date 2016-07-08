#include "../Shared/stdafx.h"
#include "dll.h"

HMODULE g_ModuleHandle = NULL;
DWORD   g_TlsIndex     = TLS_OUT_OF_INDEXES;

BOOL WINAPI _DllMainCRTStartup(HMODULE hModule, DWORD Reason, LPVOID IsStatic)
{
	if (Reason == DLL_PROCESS_ATTACH) 
	{
		g_ModuleHandle = hModule;
		g_TlsIndex = TlsAlloc();

		if (g_TlsIndex == TLS_OUT_OF_INDEXES)
		{
			return FALSE;
		}
	}
	else if (Reason == DLL_PROCESS_DETACH && !IsStatic)
	{
		TlsFree(g_TlsIndex);
	}

	return TRUE;
}