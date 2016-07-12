#include "../Shared/stdafx.h"
#include "../Shared/api.h"
#include "../Shared/trace.h"
#include "dll.h"
#include "resource.h"

#define X64_DLL_NAME   L"HideConsoleOnClose64.dll"
#define X64_EXE_NAME   L"HideConsoleOnCloseWow64Helper.exe"

#ifndef _WIN64

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
		/* lpFileName            */ FileName,
		/* dwDesiredAccess       */ GENERIC_READ,
		/* dwShareMode           */ FILE_SHARE_READ,
		/* lpSecurityAttributes  */ NULL,
		/* dwCreationDisposition */ OPEN_EXISTING,
		/* dwFlagsAndAttributes  */ FILE_ATTRIBUTE_NORMAL,
		/* hTemplateFile         */ NULL
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

	if (!CloseHandle(FileHandle))
	{
		HideConsoleTraceLastError(L"CloseHandle");
		return FALSE;
	}

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
		HideConsoleTrace(L"TempPathLength greater than provided buffer size");
		return FALSE;
	}

	HRESULT hr = StringCchCatW(
		Buffer,
		BufferCch,
		HIDE_CONSOLE_NAME L"\\"
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
	HideConsoleTrace(
		L"FileHandle=%1!p! ResourceId=%2!u!",
		FileHandle,
		ResourceId
	);

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

	if (!BytesCount)
	{
		HideConsoleTraceLastError(L"SizeofResource");
		goto Cleanup;
	}

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

	HideConsoleTrace(
		L"CreateFileW Path='%1' DesiredAccess=GENERIC_READ|GENERIC_WRITE",
		FilePath
	);

	HANDLE FileWriteHandle = CreateFileW(
		/* lpFileName            */ FilePath,
		/* dwDesiredAccess       */ GENERIC_READ | GENERIC_WRITE,
		/* dwShareMode           */ 0,
		/* lpSecurityAttributes  */ NULL,
		/* dwCreationDisposition */ OPEN_ALWAYS,
		/* dwFlagsAndAttributes  */ FILE_ATTRIBUTE_NORMAL,
		/* hTemplateFile         */ NULL
	);

	DWORD LastError = GetLastError();

	if (FileWriteHandle == INVALID_HANDLE_VALUE)
	{
		HideConsoleTraceErrorCode(L"CreateFileW", LastError);

		if (LastError == ERROR_SHARING_VIOLATION)
		{
			HideConsoleTrace(L"Wow64Helper probably running, continuing");
			return TRUE;
		}

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

			if (!CloseHandle(FileWriteHandle))
			{
				HideConsoleTraceLastError(L"CloseHandle");
				return FALSE;
			}

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

	if (!CloseHandle(FileWriteHandle))
	{
		HideConsoleTraceLastError(L"CloseHandle");
		return FALSE;
	}

	return Success;
}

BOOL WINAPI CreateHelperProcess(VOID)
{
	BOOL  Success;
	WCHAR FilePath[MAX_PATH];

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

	STARTUPINFOW StartupInfo;
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpReserved = NULL;
	StartupInfo.lpDesktop = NULL;
	StartupInfo.lpTitle = NULL;
	StartupInfo.dwFlags = 0;
	StartupInfo.cbReserved2 = 0;
	StartupInfo.lpReserved2 = NULL;

	PROCESS_INFORMATION ProcessInfo;

	HideConsoleTrace(L"CreateProcessW ApplicationName='%1'", FilePath);

	Success = CreateProcessW(
		/* lpApplicationName   */ FilePath,
		/* lpCommandLine       */ NULL,
		/* lpProcessAttributes */ NULL,
		/* lpThreadAttributes  */ NULL,
		/* bInheritHandles     */ FALSE,
		/* dwCreationFlags     */ 0,
		/* lpEnvironment       */ NULL,
		/* lpCurrentDirectory  */ NULL,
		/* lpStartupInfo       */ &StartupInfo,
		/* lpProcessInfo       */ &ProcessInfo
	);

	if (!Success)
	{
		HideConsoleTraceLastError(L"CreateProcessW");
		return FALSE;
	}

	CloseHandle(ProcessInfo.hThread);
	CloseHandle(ProcessInfo.hProcess);

	return Success;
}

BOOL WINAPI WaitForHelperReady(VOID)
{
	HANDLE ReadyEvent = CreateEventW(
		/* lpEventAttributes */ NULL,
		/* bManualReset      */ TRUE,
		/* bInitialState     */ FALSE,
		/* lpName            */ WOW64HELPER_READY_EVENT
	);

	if (!ReadyEvent)
	{
		HideConsoleTraceLastError(L"CreateEventW");
		return FALSE;
	}

	DWORD WaitResult = WaitForSingleObject(
		ReadyEvent,
		WOW64HELPER_READY_TIMEOUT
	);

	if (WaitResult == WAIT_FAILED)
	{
		HideConsoleTraceLastError(L"WaitForSingleObject");

		if (!CloseHandle(ReadyEvent))
		{
			HideConsoleTraceLastError(L"CloseHandle");
		}

		return FALSE;
	}

	if (WaitResult == WAIT_TIMEOUT)
	{
		HideConsoleTrace(L"WAIT_TIMEOUT");

		if (!CloseHandle(ReadyEvent))
		{
			HideConsoleTraceLastError(L"CloseHandle");
		}

		return FALSE;
	}

	if (!CloseHandle(ReadyEvent))
	{
		HideConsoleTraceLastError(L"CloseHandle");
		return FALSE;
	}

	return TRUE;
}

BOOL WINAPI SendWow64HelperMessage(HWND ConsoleWindow)
{
	HideConsoleTrace(L"ConsoleWindow=%1!p!", ConsoleWindow);

	if (!CreateHelperProcess())
	{
		return FALSE;
	}

	if (!WaitForHelperReady())
	{
		return FALSE;
	}

	HWND MessageWindow = FindWindowExW(
		HWND_MESSAGE,
		NULL,
		WOW64HELPER_WINDOW_CLASS,
		NULL
	);

	if (!MessageWindow)
	{
		HideConsoleTraceLastError(L"FindWindowExW");
		return FALSE;
	}

	DWORD_PTR HelperResult = 0;

	LRESULT Success = SendMessageTimeoutW(
		MessageWindow,
		WM_HIDE_CONSOLE,
		(WPARAM)ConsoleWindow,
		(LPARAM)0,
		SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
		WOW64HELPER_SMTO_TIMEOUT,
		&HelperResult
	);

	if (!Success)
	{
		HideConsoleTraceLastError(L"SendMessageTimeoutW");
		return FALSE;
	}

	HideConsoleTrace(
		L"WM_HIDE_CONSOLE Result=%1!p!",
		HelperResult
	);

	return (BOOL)HelperResult;
}

#endif ! _WIN64