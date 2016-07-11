#include "stdafx.h"
#include "trace.h"

#if NDEBUG

VOID WINAPIV ImplHideConsoleTrace(LPCWSTR Format, ...) {}
VOID WINAPI  ImplHideConsoleTraceErrorCode(LPCWSTR Message, DWORD ErrorCode) {}
VOID WINAPI  ImplHideConsoleTraceFileTime(LPCWSTR Prefix, PFILETIME FileTime) {}
VOID WINAPI  ImplHideConsoleTraceLastError(LPCWSTR Message) {}

#else

VOID WINAPIV ImplHideConsoleTrace(LPCWSTR Format, ...)
{
	DWORD LastError = GetLastError();

	va_list args;
	va_start(args, Format);

	WCHAR Buffer[1024];
	DWORD Cch = FormatMessageW(
		FORMAT_MESSAGE_FROM_STRING,
		Format,
		0,
		MAKELANGID(LANG_NEUTRAL, LANG_NEUTRAL),
		Buffer,
		ARRAYSIZE(Buffer),
		&args
	);

	va_end(args);

	if (Cch)
	{
		OutputDebugStringW(Buffer);
	}
	else
	{
		OutputDebugStringW(
			HIDE_CONSOLE_TRACE_PREFIX
			L"HideConsoleTrace: FormatMessageW failed"
			L"\r\n"
		);
	}

	SetLastError(LastError);
}

VOID WINAPI ImplHideConsoleTraceErrorCode(LPCWSTR Message, DWORD ErrorCode)
{
	DWORD LastError = GetLastError();

	WCHAR Buffer[1024];

	DWORD Cch = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL,
		ErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		Buffer,
		ARRAYSIZE(Buffer),
		NULL
	);

	if (Cch)
	{
		ImplHideConsoleTrace(L"%1: %2\r\n", Message, Buffer);
	}
	else
	{
		OutputDebugStringW(
			HIDE_CONSOLE_TRACE_PREFIX
			L"HideConsoleTraceErrorCode: FormatMessageW failed"
			L"\r\n"
		);
	}

	SetLastError(LastError);
}

VOID WINAPI ImplHideConsoleTraceFileTime(LPCWSTR Message, PFILETIME FileTime)
{
	FILETIME LocalFileTime;

	if (!FileTimeToLocalFileTime(FileTime, &LocalFileTime))
	{
		HideConsoleTraceLastError(L"FileTimeToLocalFileTime");
		return;
	}

	SYSTEMTIME SystemTime;

	if (!FileTimeToSystemTime(&LocalFileTime, &SystemTime))
	{
		HideConsoleTraceLastError(L"FileTimeToSystemTime");
		return;
	}

	WCHAR DateString[100];

	DWORD DateStringCch = GetDateFormatEx(
		LOCALE_NAME_INVARIANT,
		DATE_SHORTDATE,
		&SystemTime,
		NULL,
		DateString,
		ARRAYSIZE(DateString),
		NULL
	);

	if (!DateStringCch)
	{
		HideConsoleTraceLastError(L"GetDateFormatEx");
		return;
	}

	WCHAR TimeString[100];

	DWORD TimeStringCch = GetTimeFormatEx(
		LOCALE_NAME_INVARIANT,
		TIME_FORCE24HOURFORMAT,
		&SystemTime,
		NULL,
		TimeString,
		ARRAYSIZE(TimeString)
	);

	if (!TimeStringCch)
	{
		HideConsoleTraceLastError(L"GetTimeFormatEx");
		return;
	}

	ImplHideConsoleTrace(
		L"%1=%2 %3\r\n",
		Message,
		DateString,
		TimeString
	);
}

VOID WINAPI ImplHideConsoleTraceLastError(LPCWSTR Message)
{
	DWORD LastError = GetLastError();

	ImplHideConsoleTraceErrorCode(Message, LastError);

	SetLastError(LastError);
}

#endif
