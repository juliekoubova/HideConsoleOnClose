#if !defined(_WIN64)

#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/hexstr.h"
#include "resource.h"

#define X64_DLL_NAME L"HideConsoleOnClose64.dll"
#define X64_EXE_NAME L"HideConsoleOnCloseWow64Helper.exe"

BOOL WINAPI EnsureTempDirectory(LPWSTR Buffer, DWORD BufferCch)
{
	DWORD TempPathLength = GetTempPathW(BufferCch, Buffer);

	if (!TempPathLength)
		return FALSE;

	if (TempPathLength >= BufferCch)
		return FALSE;

	HRESULT hr = StringCchCatW(
		Buffer,
		BufferCch,
		L"HideConsoleOnClose-1.0\\"
	);

	if (FAILED(hr))
		return FALSE;

	if (!CreateDirectoryW(Buffer, NULL))
		if (GetLastError() != ERROR_ALREADY_EXISTS)
			return FALSE;

	return TRUE;
}

BOOL WINAPI WriteRCDataToFile(HANDLE FileHandle, WORD ResourceId)
{
	BOOL    Success = FALSE;
	HRSRC   ResourceHandle = NULL;
	HGLOBAL Resource = NULL;
	DWORD   BytesCount = 0;
	LPVOID  Bytes = NULL;

	ResourceHandle = FindResourceW(
		g_ModuleHandle,
		MAKEINTRESOURCEW(ResourceId),
		RT_RCDATA
	);

	if (!ResourceHandle)
		return FALSE;

	BytesCount = SizeofResource(
		g_ModuleHandle,
		ResourceHandle
	);

	Resource = LoadResource(
		g_ModuleHandle,
		ResourceHandle
	);

	if (!Resource)
		goto Cleanup;

	Bytes = LockResource(Resource);

	if (!Bytes)
		goto Cleanup;

	Success = WriteFile(
		FileHandle,
		Bytes,
		BytesCount,
		NULL,
		NULL
	);

Cleanup:

	if (Bytes)
		UnlockResource(Bytes);

	if (Resource)
		FreeResource(Resource);

	return Success;
}

BOOL WINAPI CreateTempFile(
	WORD    ResourceId,
	LPCWSTR FileName,
	LPWSTR  FilePath,
	DWORD   FilePathCch
)
{
	if (!EnsureTempDirectory(FilePath, FilePathCch))
		return FALSE;

	HRESULT hr = StringCchCatW(
		FilePath,
		FilePathCch,
		FileName
	);

	if (FAILED(hr))
		return FALSE;

	HANDLE FileWriteHandle = INVALID_HANDLE_VALUE;
	DWORD  SleepMilliseconds = 100;
	DWORD  IterationsToGo = 20;

	while (IterationsToGo-- && FileWriteHandle == INVALID_HANDLE_VALUE)
	{
		FileWriteHandle = CreateFileW(
			FilePath,
			GENERIC_WRITE,
			0,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		DWORD LastError = GetLastError();

		if (FileWriteHandle == INVALID_HANDLE_VALUE)
		{
			if (LastError == ERROR_SHARING_VIOLATION)
			{
				Sleep(SleepMilliseconds);

				if (SleepMilliseconds < 6400)
				{
					SleepMilliseconds = SleepMilliseconds * 2;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else if (LastError == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(FileWriteHandle);
			return TRUE;
		}
	} 

	if (FileWriteHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	BOOL Success = WriteRCDataToFile(
		FileWriteHandle,
		ResourceId
	);

	CloseHandle(FileWriteHandle);
	return Success;
}

BOOL WINAPI LaunchWow64Helper(HWND ConsoleWindow)
{
	BOOL  Success;
	WCHAR FilePath[MAX_PATH];
	WCHAR CommandLine[32];

	Success = CreateTempFile(
		IDR_X64_DLL,
		X64_DLL_NAME,
		FilePath,
		ARRAYSIZE(FilePath)
	);

	if (!Success)
		return FALSE;

	Success = CreateTempFile(
		IDR_X64_EXE,
		X64_EXE_NAME,
		FilePath,
		ARRAYSIZE(FilePath)
	);

	if (!Success)
		return FALSE;

	DWORD ConsoleWindowDWord = (DWORD)ConsoleWindow;
	if (!DWordToHex(ConsoleWindowDWord, CommandLine, ARRAYSIZE(CommandLine)))
		return FALSE;

	STARTUPINFOW StartupInfo;
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpReserved = NULL;
	StartupInfo.lpDesktop = NULL;
	StartupInfo.lpTitle = NULL;
	StartupInfo.dwFlags = 0;
	StartupInfo.cbReserved2 = 0;
	StartupInfo.lpReserved2 = NULL;

	PROCESS_INFORMATION ProcessInfo;

	Success = CreateProcessW(
		FilePath,
		CommandLine,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInfo
	);

	if (Success)
	{
		CloseHandle(ProcessInfo.hThread);
		CloseHandle(ProcessInfo.hProcess);
	}

	return Success;
}
#endif