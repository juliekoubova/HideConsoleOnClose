#include "stdafx.h"
#include "trace.h"

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
		ImplHideConsoleTrace(
			HIDE_CONSOLE_TRACE_PREFIX L"%1: %2\r\n",
			Message,
			Buffer
		);
	}
	else
	{
		OutputDebugStringW(
			HIDE_CONSOLE_TRACE_PREFIX 
			L"HideConsoleTraceLastError: FormatMessageW failed"
			L"\r\n"
		);
	}

	SetLastError(LastError);
}

VOID WINAPI ImplHideConsoleTraceLastError(LPCWSTR Message)
{
	DWORD LastError = GetLastError();

	ImplHideConsoleTraceErrorCode(Message, LastError);

	SetLastError(LastError);
}
