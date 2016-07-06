#if !defined(_WIN64)

#include "stdafx.h"
#include "resource.h"
#include "HideConsoleOnClose.h"

BOOL WINAPI GetTempAppletFileName(LPWSTR Buffer, DWORD BufferCch)
{
	if (BufferCch < MAX_PATH)
		return 0;

	WCHAR TempPath[MAX_PATH];
	DWORD TempPathLength = GetTempPathW(ARRAYSIZE(TempPath), TempPath);

	if (!TempPathLength)
		return 0;

	if (TempPathLength >= ARRAYSIZE(TempPath))
		return 0;

	return GetTempFileNameW(TempPath, L"hco", 0, Buffer);
}

BOOL WINAPI WriteAppletToFile(HANDLE FileHandle)
{
	BOOL    Success = FALSE;
	HRSRC   ResourceHandle = NULL;
	HGLOBAL Resource = NULL;
	DWORD   BytesCount = 0;
	LPVOID  Bytes = NULL;

	ResourceHandle = FindResourceW(
		g_ModuleHandle,
		MAKEINTRESOURCEW(IDR_X64_EXE),
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

HANDLE WINAPI CreateTempAppletFile(LPWSTR FileName, DWORD FileNameCch)
{
	SIZE_T Length = GetTempAppletFileName(
		FileName, 
		FileNameCch
	);

	if (!Length)
		return INVALID_HANDLE_VALUE;

	HANDLE FileWriteHandle = CreateFileW(
		FileName,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL
	);

	if (FileWriteHandle == INVALID_HANDLE_VALUE)
		return INVALID_HANDLE_VALUE;

	if (!WriteAppletToFile(FileWriteHandle))
	{
		CloseHandle(FileWriteHandle);
		return INVALID_HANDLE_VALUE;
	}

	CloseHandle(FileWriteHandle);

	return CreateFileW(
		FileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
		NULL
	);
}

BOOL WINAPI LaunchWow64Applet(DWORD ThreadId)
{
	WCHAR CommandLine[32];
	if (!DWordToHex(ThreadId, CommandLine, ARRAYSIZE(CommandLine)))
		return FALSE;
	
	WCHAR FileName[MAX_PATH];
	HANDLE FileReadHandle = CreateTempAppletFile(
		FileName,
		ARRAYSIZE(FileName)
	);

	if (FileReadHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	STARTUPINFOW StartupInfo = { sizeof(StartupInfo) };
	PROCESS_INFORMATION ProcessInfo;
	
	BOOL Success = CreateProcessW(
		FileName,
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

	CloseHandle(FileReadHandle);

	if (Success) 
	{
		CloseHandle(ProcessInfo.hThread);
		CloseHandle(ProcessInfo.hProcess);
	}

	return Success;
}
#endif