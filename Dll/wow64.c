#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/hexstr.h"
#include "../Shared/trace.h"
#include "dll.h"
#include "resource.h"

#define X64_DLL_NAME L"HideConsoleOnClose64.dll"
#define X64_EXE_NAME L"HideConsoleOnCloseWow64Helper.exe"

BOOL WINAPI EnsureTempDirectory(LPWSTR Buffer, DWORD BufferCch)
{
	DWORD TempPathLength = GetTempPathW(BufferCch, Buffer);

	if (!TempPathLength)
	{
		HideConsoleTraceLastError(
			L"EnsureTempDirectory: GetTempPathW"
		);

		return FALSE;
	}

	if (TempPathLength >= BufferCch)
	{
		HideConsoleTrace(
			L"EnsureTempDirectory: TempPathLength greater than provided "
			L"buffer size"
		);

		return FALSE;
	}

	HRESULT hr = StringCchCatW(
		Buffer,
		BufferCch,
		L"HideConsoleOnClose-1.0\\"
	);

	if (FAILED(hr))
	{
		HideConsoleTraceErrorCode(
			L"EnsureTempDirectory: StringCchCatW",
			hr
		);

		return FALSE;
	}

	if (CreateDirectoryW(Buffer, NULL))
	{
		HideConsoleTrace(
			L"EnsureTempDirectory: Created directory '%1'",
			Buffer
		);
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		HideConsoleTrace(
			L"EnsureTempDirectory: directory '%1' already exists",
			Buffer
		);
	}
	else
	{
		HideConsoleTraceLastError(
			L"EnsureTempDirectory: CreateDirectoryW"
		);

		return FALSE;
	}

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
	{
		HideConsoleTraceLastError(
			L"WriteRCDataToFile: FindResourceW"
		);

		return FALSE;
	}

	BytesCount = SizeofResource(
		g_ModuleHandle,
		ResourceHandle
	);

	Resource = LoadResource(
		g_ModuleHandle,
		ResourceHandle
	);

	if (!Resource)
	{
		HideConsoleTraceLastError(
			L"WriteRCDataToFile: LoadResource"
		);

		goto Cleanup;
	}

	Bytes = LockResource(Resource);

	if (!Bytes)
	{
		HideConsoleTraceLastError(
			L"WriteRCDataToFile: LockResource"
		);

		goto Cleanup;
	}

	Success = WriteFile(
		FileHandle,
		Bytes,
		BytesCount,
		NULL,
		NULL
	);

	if (!Success)
	{
		HideConsoleTraceLastError(
			L"WriteRCDataToFile: WriteFile"
		);

		goto Cleanup;
	}

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
	HideConsoleTrace(
		L"CreateTempFile: ResourceId=%1!u! FileName=%2",
		ResourceId,
		FileName
	);

	if (!EnsureTempDirectory(FilePath, FilePathCch))
		return FALSE;

	HRESULT hr = StringCchCatW(
		FilePath,
		FilePathCch,
		FileName
	);

	if (FAILED(hr))
	{
		HideConsoleTraceErrorCode(
			L"CreateTempFile: StringCchCatW",
			hr
		);

		return FALSE;
	}

	HANDLE FileWriteHandle = INVALID_HANDLE_VALUE;
	DWORD  SleepMilliseconds = 100;
	DWORD  IterationsToGo = 20;

	while (IterationsToGo-- && FileWriteHandle == INVALID_HANDLE_VALUE)
	{
		HideConsoleTrace(
			L"CreateTempFile: Attempting to CreateFileW Path='%1' "
			L"for GENERIC_WRITE",
			FilePath
		);

		HideConsoleTrace(
			L"CreateTempFile: IterationsToGo=%1!u!",
			IterationsToGo
		);

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
				HideConsoleTraceErrorCode(
					L"CreateTempFile: CreateFileW: ERROR_SHARING_VIOLATION, "
					L"sleeping for %1!u! ms",
					SleepMilliseconds
				);

				Sleep(SleepMilliseconds);

				if (SleepMilliseconds < 6400)
				{
					SleepMilliseconds = SleepMilliseconds * 2;
				}
			}
			else
			{
				HideConsoleTraceErrorCode(
					L"CreateTempFile: CreateFileW",
					LastError
				);

				return FALSE;
			}
		}
		else if (LastError == ERROR_ALREADY_EXISTS)
		{
			HideConsoleTrace(
				L"CreateTempFile: File '%1' already exists",
				FilePath
			);

			CloseHandle(FileWriteHandle);
			return TRUE;
		}
	}

	if (FileWriteHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL Success = WriteRCDataToFile(
		FileWriteHandle,
		ResourceId
	);

	CloseHandle(FileWriteHandle);
	return Success;
}

BOOL WINAPI LaunchWow64Helper(HWND ConsoleWindow)
{
	HideConsoleTrace(
		L"LaunchWow64Helper: ConsoleWindow=%1!p!",
		ConsoleWindow
	);

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

#pragma warning(push)
#pragma warning(disable:4311)

	// only low 32-bits of HWNDs are used
	DWORD ConsoleWindowDWord = (DWORD)ConsoleWindow;

#pragma warning(pop)

	if (!DWordToHex(ConsoleWindowDWord, CommandLine, ARRAYSIZE(CommandLine)))
	{
		HideConsoleTraceLastError(
			L"LaunchWow64Helper: DWordToHex"
		);

		return FALSE;
	}

	STARTUPINFOW StartupInfo;
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpReserved = NULL;
	StartupInfo.lpDesktop = NULL;
	StartupInfo.lpTitle = NULL;
	StartupInfo.dwFlags = 0;
	StartupInfo.cbReserved2 = 0;
	StartupInfo.lpReserved2 = NULL;

	PROCESS_INFORMATION ProcessInfo;

	HideConsoleTrace(
		L"LaunchWow64Helper: CreateProcessW Path='%1' Arguments='%2'",
		FilePath,
		CommandLine
	);

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
	else
	{
		HideConsoleTraceLastError(
			L"LaunchWow64Helper: CreateProcessW"
		);
	}

	return Success;
}