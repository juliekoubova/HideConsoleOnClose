#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/hexstr.h"
#include "../Shared/trace.h"
#include "dll.h"
#include "resource.h"

#define X64_DLL_NAME   L"HideConsoleOnClose64.dll"
#define X64_EXE_NAME   L"HideConsoleOnCloseWow64Helper.exe"
#define VERSIONED_NAME L"HideConsoleOnClose-1.0"

BOOL WINAPI GetModuleLastWriteTime(
	HMODULE ModuleHandle,
	PFILETIME LastWriteTime
)
{
	HideConsoleTrace(
		L"GetModuleLastWriteTime: ModuleHandle=%1!p! LastWriteTime=%2!p!",
		ModuleHandle,
		LastWriteTime
	);

	if (!ModuleHandle)
	{
		return FALSE;
	}

	WCHAR FileName[MAX_PATH];
	DWORD FileNameCch = GetModuleFileNameW(
		ModuleHandle,
		FileName,
		ARRAYSIZE(FileName)
	);

	if (!FileNameCch)
	{
		HideConsoleTraceLastError(
			L"GetModuleLastWriteTime: GetModuleFileNameW"
		);

		return FALSE;
	}

	HANDLE FileHandle = CreateFileW(
		FileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		HideConsoleTraceLastError(
			L"GetModuleLastWriteTime: CreateFileW"
		);

		return FALSE;
	}

	BOOL Success = GetFileTime(
		FileHandle,
		NULL,
		NULL,
		LastWriteTime
	);

	CloseHandle(FileHandle);

	return Success;
}
BOOL WINAPI EnsureHelperDirectory(LPWSTR Buffer, DWORD BufferCch)
{
	DWORD TempPathLength = GetTempPathW(BufferCch, Buffer);

	if (!TempPathLength)
	{
		HideConsoleTraceLastError(
			L"EnsureHelperDirectory: GetTempPathW"
		);

		return FALSE;
	}

	if (TempPathLength >= BufferCch)
	{
		HideConsoleTrace(
			L"EnsureHelperDirectory: TempPathLength greater than provided "
			L"buffer size"
		);

		return FALSE;
	}

	HRESULT hr = StringCchCatW(
		Buffer,
		BufferCch,
		VERSIONED_NAME L"\\"
	);

	if (FAILED(hr))
	{
		HideConsoleTraceErrorCode(
			L"EnsureHelperDirectory: StringCchCatW",
			hr
		);

		return FALSE;
	}

	if (CreateDirectoryW(Buffer, NULL))
	{
		HideConsoleTrace(
			L"EnsureHelperDirectory: Created directory '%1'",
			Buffer
		);
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		HideConsoleTrace(
			L"EnsureHelperDirectory: directory '%1' already exists",
			Buffer
		);
	}
	else
	{
		HideConsoleTraceLastError(
			L"EnsureHelperDirectory: CreateDirectoryW"
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

	DWORD BytesWritten;
	Success = WriteFile(
		FileHandle,
		Bytes,
		BytesCount,
		&BytesWritten,
		NULL
	);

	if (Success)
	{
		HideConsoleTrace(
			L"WriteRCDataToFile: BytesWritten: %1!u!",
			BytesWritten
		);
	}
	else
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

BOOL WINAPI EnsureHelperFile(
	WORD    ResourceId,
	LPCWSTR FileName,
	LPWSTR  FilePath,
	DWORD   FilePathCch
)
{
	HideConsoleTrace(
		L"EnsureHelperFile: ResourceId=%1!u! FileName='%2'",
		ResourceId,
		FileName
	);

	if (!EnsureHelperDirectory(FilePath, FilePathCch))
		return FALSE;

	HRESULT hr = StringCchCatW(
		FilePath,
		FilePathCch,
		FileName
	);

	if (FAILED(hr))
	{
		HideConsoleTraceErrorCode(
			L"EnsureHelperFile: StringCchCatW",
			hr
		);

		return FALSE;
	}

	HANDLE FileWriteHandle = INVALID_HANDLE_VALUE;

	HideConsoleTrace(
		L"EnsureHelperFile: Attempting to CreateFileW Path='%1' "
		L"for reading and writing",
		FilePath
	);

	FileWriteHandle = CreateFileW(
		FilePath,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD LastError = GetLastError();

	if (FileWriteHandle == INVALID_HANDLE_VALUE)
	{
		HideConsoleTraceErrorCode(
			L"EnsureHelperFile: CreateFileW",
			LastError
		);

		return FALSE;
	}

	if (LastError == ERROR_ALREADY_EXISTS)
	{
		HideConsoleTrace(
			L"EnsureHelperFile: File '%1' already exists",
			FilePath
		);

		FILETIME FileLastWriteTime;
		if (!GetFileTime(FileWriteHandle, NULL, NULL, &FileLastWriteTime))
		{
			HideConsoleTraceLastError(
				L"EnsureHelperFile: GetFileTime"
			);

			CloseHandle(FileWriteHandle);
			return FALSE;
		}

		FILETIME ModuleLastWriteTime;
		if (!GetModuleLastWriteTime(g_ModuleHandle, &ModuleLastWriteTime))
		{
			CloseHandle(FileWriteHandle);
			return FALSE;
		}

		HideConsoleTraceFileTime(
			L"EnsureHelperFile: FileLastWriteTime",
			&FileLastWriteTime
		);

		HideConsoleTraceFileTime(
			L"EnsureHelperFile: ModuleLastWriteTime",
			&ModuleLastWriteTime
		);

		LONG Result = CompareFileTime(
			&ModuleLastWriteTime,
			&FileLastWriteTime
		);

		if (Result < 1)
		{
			HideConsoleTrace(
				L"EnsureHelperFile: Module is older than the existing "
				L"helper file, will use the existing file."
			);

			return TRUE;
		}
		else
		{
			HideConsoleTrace(
				L"EnsureHelperFile: Module is newer than the existing "
				L"helper file, will overwrite it."
			);
		}
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

	Success = EnsureHelperFile(
		IDR_X64_DLL,
		X64_DLL_NAME,
		FilePath,
		ARRAYSIZE(FilePath)
	);

	if (!Success)
		return FALSE;

	Success = EnsureHelperFile(
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