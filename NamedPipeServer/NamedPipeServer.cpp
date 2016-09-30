// NamedPipeServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "server.h"
#include "test.h"

int main()
{
	WSADATA data;
	if (WSAStartup(0x0202, &data) != 0) {
		return -1;
	}

	// RunTests();
	CreateServer();

	WSACleanup();
    return 0;
}
