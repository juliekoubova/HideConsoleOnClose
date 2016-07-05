#include "stdafx.h"
#include "HideConsoleOnClose.h"

BOOL WINAPI GetSysNativeRundllPath(LPWSTR Buffer, DWORD BufferCch)
{
	UINT Length = GetSystemWindowsDirectoryW(Buffer, BufferCch);

	if (!Length)
		return FALSE;

	if (Length > BufferCch)
		return FALSE;

	HRESULT hr = StringCchCatW(
		Buffer, 
		BufferCch, 
		L"SysNative\\rundll32.exe"
	);

	return SUCCEEDED(hr);
}

BOOL WINAPI GetRundllCommandLine(DWORD ThreadId, LPWSTR Buffer, DWORD BufferCch)
{
	return FALSE;
}

BOOL WINAPI LaunchSysNativeApplet(DWORD ThreadId)
{
	WCHAR Path[MAX_PATH];
	WCHAR CommandLine[512];

	if (!GetSysNativeRundllPath(Path, ARRAYSIZE(Path)))
		return FALSE;

	if (!GetRundllCommandLine(ThreadId, CommandLine, ARRAYSIZE(CommandLine)))
		return FALSE;

	return FALSE;

}