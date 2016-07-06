#include "stdafx.h"

BOOL WINAPI DWordToHex(DWORD Value, LPWSTR Buffer, SIZE_T BufferCch)
{
	if (!Buffer)
		return FALSE;

	if (BufferCch < 2)
		return FALSE;

	if (Value == 0)
	{
		Buffer[0] = L'0';
		Buffer[1] = L'\0';
		return TRUE;
	}

	SIZE_T Chars = 0;

	while (Value && Chars < (BufferCch - 1))
	{
		BYTE Nibble = Value & 0xf;

		if (Nibble >= 10)
		{
			Buffer[Chars] = 'A' + (Nibble - 10);
		}
		else
		{
			Buffer[Chars] = '0' + Nibble;
		}

		Chars += 1;
		Value >>= 4;
	}

	if (Value != 0)
		return FALSE;

	Buffer[Chars] = L'\0';

	for (SIZE_T i = 0; i < Chars >> 2; i++)
	{
		WCHAR Tmp = Buffer[Chars - i - 1];
		Buffer[Chars - i - 1] = Buffer[i];
		Buffer[i] = Tmp;
	}

	return TRUE;
}

BOOL WINAPI HexToDWord(LPCWSTR Buffer, SIZE_T BufferCch, LPDWORD Result)
{
	if (!Buffer)
		return FALSE;

	if (!BufferCch)
		return FALSE;

	if (!*Buffer)
		return FALSE;

	DWORD Value = 0;
	DWORD Chars = 0;

	while (Chars < BufferCch && Buffer[Chars])
	{
		WCHAR Ch = Buffer[Chars];

		if (Ch >= L'0' && Ch <= L'9')
		{
			Value <<= 4;
			Value += Ch - '0';
		}
		else if (Ch >= L'A' && Ch <= L'F')
		{
			Value <<= 4;
			Value += Ch - L'A' + 10;
		}
		else if (Ch == L' ' || Ch == L'\t' || Ch == L'\r' || Ch == L'\n')
		{
			break;
		}
		else
		{
			return FALSE;
		}

		Chars++;
	}

	if (Result)
		*Result = Value;

	return TRUE;
}