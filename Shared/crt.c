#include <stddef.h>

void* __cdecl memset(void* dst, int val, size_t size)
{
	unsigned char *ptr = dst;

	while (size > 0)
	{
		*ptr = (unsigned char) val;
		ptr++;
		size--;
	}

	return dst;
}
