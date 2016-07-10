#include "stdafx.h"
#include "trace.h"

BOOL WINAPI DWordToHex(DWORD Value, LPWSTR Buffer, SIZE_T BufferCch)
{
	if (!Buffer)
	{
		HideConsoleTrace(
			L"DWordToHex: Value=0x%1!x! Buffer=NULL",
			Value
		);

		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	HideConsoleTrace(
		L"DWordToHex: Value=0x%1!x! BufferCch=%2!u!",
		Value,
		BufferCch
	);

	if (BufferCch < 2)
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	if (Value == 0)
	{
		Buffer[0] = L'0';
		Buffer[1] = L'\0';

		HideConsoleTrace(
			L"DWordToHex: Value=0x%1!x! Buffer='%2'",
			Value,
			Buffer
		);

		SetLastError(ERROR_SUCCESS);
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
	{
		HideConsoleTrace(
			L"DWordToHex: Insufficient buffer"
		);

		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	Buffer[Chars] = L'\0';

	for (SIZE_T i = 0; i < Chars >> 2; i++)
	{
		WCHAR Tmp = Buffer[Chars - i - 1];
		Buffer[Chars - i - 1] = Buffer[i];
		Buffer[i] = Tmp;
	}

	HideConsoleTrace(
		L"DWordToHex: Buffer='%1'",
		Buffer
	);

	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

BOOL WINAPI HexToDWord(LPCWSTR Buffer, PDWORD Result)
{
	if (!Buffer)
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	HideConsoleTrace(L"HexToDWord: Buffer='%1'", Buffer);

	if (!*Buffer)
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	DWORD Value = 0;

	while (*Buffer)
	{
		WCHAR Ch = *Buffer;

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
			HideConsoleTrace(L"HexToDWord: Invalid Buffer='%1'", Buffer);

			SetLastError(ERROR_INVALID_DATA);
			return FALSE;
		}

		Buffer++;
	}

	HideConsoleTrace(
		L"HexToDWord: Buffer='%1' Result=0x%2!x!",
		Buffer,
		Value
	);

	if (Result)
		*Result = Value;

	SetLastError(ERROR_SUCCESS);
	return TRUE;
}