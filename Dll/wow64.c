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
		L"ModuleHandle=%1!p! LastWriteTime=%2!p!",
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
		HideConsoleTraceLastError(L"GetModuleFileNameW");
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
		HideConsoleTraceLastError(L"CreateFileW");
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
		HideConsoleTraceLastError(L"GetTempPathW");
		return FALSE;
	}

	if (TempPathLength >= BufferCch)
	{
		HideConsoleTrace(
			L"TempPathLength greater than provided buffer size"
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
		HideConsoleTraceErrorCode(L"StringCchCatW", hr);
		return FALSE;
	}

	if (CreateDirectoryW(Buffer, NULL))
	{
		HideConsoleTrace(L"Created directory '%1'", Buffer);
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		HideConsoleTrace(L"Directory '%1' already exists", Buffer);
	}
	else
	{
		HideConsoleTraceLastError(L"CreateDirectoryW");
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
		HideConsoleTraceLastError(L"FindResourceW");
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
		HideConsoleTraceLastError(L"LoadResource");
		goto Cleanup;
	}

	Bytes = LockResource(Resource);

	if (!Bytes)
	{
		HideConsoleTraceLastError(L"LockResource");
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
		HideConsoleTrace(L"BytesWritten: %1!u!", BytesWritten);
	}
	else
	{
		HideConsoleTraceLastError(L"WriteFile");
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
		L"ResourceId=%1!u! FileName='%2'",
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
		HideConsoleTraceErrorCode(L"StringCchCatW", hr);
		return FALSE;
	}

	HANDLE FileWriteHandle = INVALID_HANDLE_VALUE;

	HideConsoleTrace(
		L"Attempting to CreateFileW Path='%1' for reading and writing",
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
		HideConsoleTraceErrorCode(L"CreateFileW", LastError);
		return FALSE;
	}

	if (LastError == ERROR_ALREADY_EXISTS)
	{
		HideConsoleTrace(L"File '%1' already exists", FilePath);

		FILETIME FileLastWriteTime;
		if (!GetFileTime(FileWriteHandle, NULL, NULL, &FileLastWriteTime))
		{
			HideConsoleTraceLastError(L"GetFileTime");
			CloseHandle(FileWriteHandle);
			return FALSE;
		}

		FILETIME ModuleLastWriteTime;
		if (!GetModuleLastWriteTime(g_ModuleHandle, &ModuleLastWriteTime))
		{
			CloseHandle(FileWriteHandle);
			return FALSE;
		}

		HideConsoleTraceFileTime(L"FileLastWriteTime", &FileLastWriteTime);
		HideConsoleTraceFileTime(L"ModuleLastWriteTime", &ModuleLastWriteTime);

		LONG Result = CompareFileTime(
			&ModuleLastWriteTime,
			&FileLastWriteTime
		);

		if (Result < 1)
		{
			HideConsoleTrace(
				L"Module is older than the existing helper file, "
				L"will use the existing file."
			);

			return TRUE;
		}
		else
		{
			HideConsoleTrace(
				L"Module is newer than the existing helper file, "
				L"will overwrite it."
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
		L"ConsoleWindow=%1!p!",
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
		HideConsoleTraceLastError( L"DWordToHex" );
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
		L"CreateProcessW Path='%1' Arguments='%2'",
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
		HideConsoleTraceLastError(L"CreateProcessW");
	}

	return Success;
}