#include "stdafx.h"
#include "networking.h"
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

namespace networking
{
	void startup()
	{
		WSADATA wsa_data{};
		WSAStartup(MAKEWORD(2, 2), &wsa_data);
	}

	void shutdown()
	{
		WSACleanup();
	}
}
