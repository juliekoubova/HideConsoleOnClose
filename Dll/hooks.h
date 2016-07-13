#pragma once
#include "../Shared/stdafx.h"
#include "../Shared/api.h"

BOOL WINAPI UnhookHideConsole(PHIDE_CONSOLE_HOOKS Hook, PBOOL WasLastHook);

BOOL WINAPI SetHideConsoleHooks(PHIDE_CONSOLE_HOOKS Hooks, DWORD ThreadId);
