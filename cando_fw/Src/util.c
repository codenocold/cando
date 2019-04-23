#include "util.h"

void hex32(char *out, uint32_t val)
{
	char *p = out + 8;

	*p-- = 0;

	while (p >= out) {
		uint8_t nybble = val & 0x0F;

		if (nybble < 10)
			*p = '0' + nybble;
		else
			*p = 'A' + nybble - 10;

		val >>= 4;
		p--;
	}
}
