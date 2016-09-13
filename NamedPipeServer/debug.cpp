#include "stdafx.h"
#include "debug.h"

#define MAXMSG 4096
#define PREFIX "[NS] "

void dprintf(const char* fmt, ...)
{
	va_list args;
	char buf[MAXMSG +sizeof(PREFIX)] = PREFIX;
	char *p = &buf[sizeof(PREFIX) - 1];

	va_start(args, fmt);
	vsnprintf(p, MAXMSG, fmt, args);
	va_end(args);

	fprintf(stderr, "%s", buf);
	OutputDebugStringA(buf);
}

void HexPrint(LPCSTR aPrefix, LPVOID aBuffer, DWORD aBufferSize)
{
	DWORD bufSize = (DWORD)(aBufferSize * 2) + 1;
	std::unique_ptr<CHAR[]> buf(new CHAR[bufSize]);
	buf[0] = '\0';

	DWORD offset = 0;
	for (auto i = 0u; i < aBufferSize; ++i) {
		offset += snprintf(buf.get() + offset, bufSize - offset, "%02x", ((uint8_t*)aBuffer)[i]);
	}

	dprintf("%s %s\n", aPrefix, buf.get());
}
