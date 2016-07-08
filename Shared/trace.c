#include "stdafx.h"
#include "trace.h"

VOID WINAPIV HideConsoleTraceImpl(LPCWSTR Format, ...)
{
#if HIDE_CONSOLE_TRACE
	va_list args;
	va_start(args, Format);

	WCHAR Buffer[1024];
	DWORD Cch = FormatMessageW(
		FORMAT_MESSAGE_FROM_STRING,
		Format,
		0,
		0,
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
			L"HideConsoleTraceImpl: FormatMessageW failed"
			L"\r\n"
		);
	}
#endif
}

VOID WINAPI HideConsoleTraceLastErrorImpl(LPCWSTR Message)
{
#if HIDE_CONSOLE_TRACE

	DWORD LastError = GetLastError();

	WCHAR Buffer[1024];
	DWORD Cch = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		LastError,
		0,
		Buffer,
		ARRAYSIZE(Buffer),
		NULL
	);

	if (Cch)
	{
		HideConsoleTraceImpl(
			HIDE_CONSOLE_TRACE_PREFIX L"%1!s!: %2!s!",
			Message,
			Buffer
		);
	}
	else
	{
		OutputDebugStringW(
			HIDE_CONSOLE_TRACE_PREFIX 
			L"HideConsoleTraceLastErrorImpl: FormatMessageW failed"
			L"\r\n"
		);
	}

#endif
}
