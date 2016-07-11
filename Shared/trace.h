#pragma once
#include "stdafx.h"

#define HIDE_CONSOLE_TRACE_PREFIX L"HideConsoleOnClose: "

VOID WINAPIV ImplHideConsoleTrace(LPCWSTR Format, ...);
VOID WINAPI  ImplHideConsoleTraceErrorCode(LPCWSTR Message, DWORD ErrorCode);
VOID WINAPI  ImplHideConsoleTraceFileTime(LPCWSTR Prefix, PFILETIME FileTime);
VOID WINAPI  ImplHideConsoleTraceLastError(LPCWSTR Message);

#define HideConsoleTrace(msg, ...)                                            \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE)                                               \
		{                                                                     \
			ImplHideConsoleTrace(                                             \
				HIDE_CONSOLE_TRACE_PREFIX                                     \
				msg                                                           \
				L"\r\n",                                                      \
				__VA_ARGS__                                                   \
			);                                                                \
		}                                                                     \
	} while(0) 

#define HideConsoleTraceErrorCode(msg, err)                                   \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE)                                               \
		{                                                                     \
			ImplHideConsoleTraceErrorCode(msg, err);                          \
		}                                                                     \
	} while(0) 

#define HideConsoleTraceFileTime(msg, ft)                                     \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE)                                               \
		{                                                                     \
			ImplHideConsoleTraceFileTime(msg, ft);                            \
		}                                                                     \
	} while(0) 

#define HideConsoleTraceLastError(msg)                                        \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE)                                               \
		{                                                                     \
			ImplHideConsoleTraceLastError(msg);                               \
		}                                                                     \
	} while(0) 
