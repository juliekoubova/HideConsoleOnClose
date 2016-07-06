#pragma once
#include "stdafx.h"

BOOL WINAPI DWordToHex(DWORD Value, LPWSTR Buffer, SIZE_T BufferCch);

BOOL WINAPI HexToDWord(LPCWSTR Buffer, PDWORD Result);
