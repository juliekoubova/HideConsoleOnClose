#include "../Shared/stdafx.h"
#include "../Shared/trace.h"
#include <TlHelp32.h>

typedef struct tagFINDING_CONHOST
{
	HWND  ConsoleWindow;
	DWORD CurrentThreadId;
	DWORD Result;
}
FINDING_CONHOST, *PFINDING_CONHOST;

static BOOL CALLBACK FindThreadId(HWND hWnd, LPARAM lParam)
{
	PFINDING_CONHOST State = (PFINDING_CONHOST)lParam;

	if (hWnd == State->ConsoleWindow)
	{
		State->Result = State->CurrentThreadId;
		return FALSE;
	}

	return TRUE;
}

//
// I'd love to use GetWindowThreadProcessId but that function reports the
// window as belonging to the console process (e.g. cmd.exe or powershell.exe)
// instead of the real owner (ie. conhost.exe).
//
// Therefore, this hack.
//
DWORD WINAPI FindConhostUIThreadId(HWND hWnd)
{
	HideConsoleTrace(L"hWnd=%1!p!", hWnd);
	HideConsoleAssert(hWnd != NULL);

	if (!hWnd)
	{
		return 0;
	}

	HANDLE Snapshot = CreateToolhelp32Snapshot(
		TH32CS_SNAPTHREAD,
		0
	);

	if (Snapshot == INVALID_HANDLE_VALUE)
	{
		HideConsoleTraceLastError(L"CreateToolhelp32Snapshot");
		return 0;
	}

	FINDING_CONHOST State;
	State.ConsoleWindow = hWnd;
	State.Result = 0;

	THREADENTRY32 ThreadEntry;
	ThreadEntry.dwSize = sizeof(ThreadEntry);

	if (!Thread32First(Snapshot, &ThreadEntry))
	{
		HideConsoleTraceLastError(L"Thread32First");
		goto Cleanup;
	}

#if HIDE_CONSOLE_TRACE
	LARGE_INTEGER Frequency;
	LARGE_INTEGER StartingTime;
	LARGE_INTEGER EndingTime;
	LARGE_INTEGER ElapsedMilliseconds;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&StartingTime);
#endif

	DWORD ThreadsEnumerated = 0;

	do
	{
		State.CurrentThreadId = ThreadEntry.th32ThreadID;

		if (State.CurrentThreadId)
		{
			EnumThreadWindows(
				State.CurrentThreadId,
				FindThreadId,
				(LPARAM)&State
			);

			ThreadsEnumerated++;

			if (State.Result)
				break;
		}

	} while (Thread32Next(Snapshot, &ThreadEntry));

#if HIDE_CONSOLE_TRACE

	QueryPerformanceCounter(&EndingTime);
	ElapsedMilliseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

	if (!ElapsedMilliseconds.HighPart && !Frequency.HighPart)
	{
		ElapsedMilliseconds.LowPart *= 1000;
		ElapsedMilliseconds.LowPart /= Frequency.LowPart;

		HideConsoleTrace(
			L"ThreadsEnumerated=%1!u! Elapsed=%2!u!ms Result=%3!u!",
			ThreadsEnumerated,
			ElapsedMilliseconds.LowPart,
			State.Result
		);
	}
	else
	{
		HideConsoleTrace(
			L"ThreadsEnumerated=%1!u! Result=%2!u!",
			ThreadsEnumerated,
			State.Result
		);
	}
#endif

Cleanup:

	CloseHandle(Snapshot);
	return State.Result;
}