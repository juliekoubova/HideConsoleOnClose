#pragma once
#include "stdafx.h"

#ifdef _DEBUG
#  define HIDE_CONSOLE_TRACE 1
#else
#  define HIDE_CONSOLE_TRACE 0
#endif

#define HIDE_CONSOLE_TRACE_PREFIX L"HideConsoleOnClose: "

VOID WINAPIV HideConsoleTraceImpl(LPCWSTR Format, ...);
VOID WINAPI  HideConsoleTraceLastErrorImpl(LPCWSTR Message);

#define HideConsoleTrace(msg, ...)                  \
	do                                              \
	{                                               \
		if (HIDE_CONSOLE_TRACE)                     \
		{                                           \
			HideConsoleTraceImpl(                   \
				HIDE_CONSOLE_TRACE_PREFIX           \
				msg                                 \
				L"\r\n",                            \
				__VA_ARGS__                         \
			);                                      \
		}                                           \
	} while(0) 

#define HideConsoleTraceLastError(msg)              \
	do                                              \
	{                                               \
		if (HIDE_CONSOLE_TRACE)                     \
		{                                           \
			HideConsoleTraceLastErrorImpl(msg);     \
		}                                           \
	} while(0) 
