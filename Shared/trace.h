#pragma once
#include "stdafx.h"

#define HIDE_CONSOLE_TRACE_PREFIX L"HideConsoleOnClose: "

VOID WINAPIV ImplHideConsoleTrace(LPCWSTR Format, ...);
VOID WINAPI  ImplHideConsoleTraceErrorCode(LPCWSTR Message, DWORD ErrorCode);
VOID WINAPI  ImplHideConsoleTraceFileTime(LPCWSTR Prefix, PFILETIME FileTime);
VOID WINAPI  ImplHideConsoleTraceLastError(LPCWSTR Message);

#define HideConsoleAssert(condition)                                          \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE && !(condition))                               \
		{                                                                     \
			ImplHideConsoleTrace(                                             \
				HIDE_CONSOLE_TRACE_PREFIX                                     \
				__FUNCTIONW__                                                 \
				L": Assertion failed: "                                       \
				_CRT_WIDE(_CRT_STRINGIZE(condition))                          \
				L"\r\n"                                                       \
			);                                                                \
		                                                                      \
			DbgRaiseAssertionFailure();                                       \
		}                                                                     \
	} while(0) 

#define HideConsoleTrace(msg, ...)                                            \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE)                                               \
		{                                                                     \
			ImplHideConsoleTrace(                                             \
				HIDE_CONSOLE_TRACE_PREFIX                                     \
				__FUNCTIONW__                                                 \
				L": "                                                         \
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
			ImplHideConsoleTraceErrorCode(                                    \
				HIDE_CONSOLE_TRACE_PREFIX                                     \
				__FUNCTIONW__                                                 \
				L": "                                                         \
				msg,                                                          \
				err                                                           \
			);                                                                \
		}                                                                     \
	} while(0) 

#define HideConsoleTraceFileTime(msg, ft)                                     \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE)                                               \
		{                                                                     \
			ImplHideConsoleTraceFileTime(                                     \
				HIDE_CONSOLE_TRACE_PREFIX                                     \
				__FUNCTIONW__                                                 \
				L": "                                                         \
				msg,                                                          \
				ft                                                            \
			);                                                                \
		}                                                                     \
	} while(0) 

#define HideConsoleTraceLastError(msg)                                        \
	do                                                                        \
	{                                                                         \
		if (HIDE_CONSOLE_TRACE)                                               \
		{                                                                     \
			ImplHideConsoleTraceLastError(                                    \
				HIDE_CONSOLE_TRACE_PREFIX                                     \
				__FUNCTIONW__                                                 \
				L": "                                                         \
				msg                                                          \
			);                                                                \
		}                                                                     \
	} while(0) 
