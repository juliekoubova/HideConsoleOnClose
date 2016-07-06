#include "../Shared/stdafx.h"
#include <TlHelp32.h>

typedef struct tagFINDING_CONHOST
{
	HWND  ConsoleWindow;
	DWORD CurrentThreadId;
	DWORD Result;
}
FINDING_CONHOST;

BOOL CALLBACK FindThreadId(HWND hWnd, LPARAM lParam)
{
	FINDING_CONHOST *State = (FINDING_CONHOST*)lParam;

	if (hWnd == State->ConsoleWindow)
	{
		State->Result = State->CurrentThreadId;
		return FALSE;
	}

	return TRUE;
}

DWORD WINAPI FindConhostUIThreadId(HWND ConsoleWindow)
{
	HANDLE Snapshot = CreateToolhelp32Snapshot(
		TH32CS_SNAPTHREAD,
		0
	);

	if (Snapshot == INVALID_HANDLE_VALUE)
		return 0;

	FINDING_CONHOST State;
	State.ConsoleWindow = ConsoleWindow;
	State.Result = 0;

	THREADENTRY32 ThreadEntry;
	ThreadEntry.dwSize = sizeof(ThreadEntry);

	if (!Thread32First(Snapshot, &ThreadEntry))
		goto Cleanup;

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

			if (State.Result)
				break;
		}

	} while (Thread32Next(Snapshot, &ThreadEntry));

Cleanup:

	CloseHandle(Snapshot);
	return State.Result;
}