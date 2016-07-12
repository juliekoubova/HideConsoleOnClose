#pragma once

#ifdef _DEBUG
#  define HIDE_CONSOLE_TRACE 1
#else
#  define HIDE_CONSOLE_TRACE 0
#endif

// We need at least Windows 7, that's where conhost.exe was introduced.
// In previous versions, console windows were owned by the special csrss.exe
// process, which isn't possible to hook anyway.

#define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h>

#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCOMM
#define NOCTLMGR
#define NODEFERWINDOWPOS
#define NODRAWTEXT
#define NOGDI
#define NOGDICAPMASKS
#define NOHELP
#define NOICONS
#define NOKANJI
#define NOKERNEL
#define NOKEYSTATES
#define NOMCX
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOPROFILER
#define NORASTEROPS
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOVIRTUALKEYCODES
#define NOWINOFFSETS
#define WIN32_LEAN_AND_MEAN

#undef NOMB
#undef NOMSG
#undef NONLS
#undef NOSHOWWINDOW
#undef NOSYSCOMMANDS
#undef NOUSER
#undef NOWH
#undef NOWINMESSAGES
#undef NOWINSTYLES
#undef OEMRESOURCE

#include <Windows.h>