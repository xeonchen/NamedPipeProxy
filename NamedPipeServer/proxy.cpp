#include "stdafx.h"
#include "proxy.h"
#include "debug.h"

#include <mstcpip.h>
#pragma comment(lib, "ws2_32.lib")

Proxy::Proxy()
{
	SecureZeroMemory(&mSendOverlap, sizeof(WSAOVERLAPPED));
	SecureZeroMemory(&mRecvOverlap, sizeof(WSAOVERLAPPED));
	mSendOverlap.hEvent = WSACreateEvent();
	mRecvOverlap.hEvent = WSACreateEvent();
	WSASetEvent(mSendOverlap.hEvent);
	WSASetEvent(mRecvOverlap.hEvent);
}

Proxy::~Proxy()
{
	Reset();
}

HRESULT WaitConnect(SOCKET s)
{
	WSAEVENT e = WSACreateEvent();
	if (e == WSA_INVALID_EVENT) {
		dprintf("WSACreateEvent failed [%d]\n", WSAGetLastError());
		return E_FAIL;
	}

	int err = WSAEventSelect(s, e, FD_WRITE);
	if (err) {
		dprintf("WSAEventSelect failed [%d]\n", WSAGetLastError());
		return E_FAIL;
	}

	DWORD r = WSAWaitForMultipleEvents(1, &e, FALSE, 3000, FALSE);
	switch (r) {
	case WSA_WAIT_EVENT_0:
		break;
	case WSA_WAIT_TIMEOUT:
		dprintf("WSAWaitForMultipleEvents timeout\n");
		return E_FAIL;
	default:
		dprintf("WSAWaitForMultipleEvents error [%d]\n", r);
		return E_FAIL;
	}

	WSANETWORKEVENTS ev;
	if (SOCKET_ERROR == WSAEnumNetworkEvents(s, e, &ev)) {
		dprintf("WSAEnumNetworkEvents failed [%d]\n", WSAGetLastError());
		return E_FAIL;
	}

	if (ev.iErrorCode[FD_WRITE] != 0) {
		dprintf("FD_WRITE failed [%d]\n", ev.iErrorCode[FD_WRITE]);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT Proxy::Connect(LPCSTR aHost, LPCSTR aPort)
{
	HRESULT hr;
	int err;

	SOCKET sock;
	SmartAddrInfo info = nullptr;
	if (FAILED(hr = CreateSocket(aHost, aPort, sock, info))) {
		dprintf("CreateSocket failed [0x%08x]\n", hr);
		return hr;
	}

	linger l;
	l.l_linger = 3;
	l.l_onoff = 1;
	err = setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)(void*)&l, sizeof(l));
	if (err) {
		dprintf("setsockopt failed");
		return E_UNEXPECTED;
	}

	int value = 0, len = sizeof(value);
	err = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)(void*)&value, &len);
	if (err) {
		dprintf("getsockopt failed");
		return E_UNEXPECTED;
	}

	if (value < 65535) {
		value = 65535;
		err = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)(void*)&value, sizeof(value));
		if (err) {
			dprintf("setsockopt failed");
			return E_UNEXPECTED;
		}
	}

	value = 1;
	err = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)(void*)&value, sizeof(value));
	if (err) {
		dprintf("setsockopt failed");
		return E_UNEXPECTED;
	}

	int iResult = WSAConnect(sock, info->ai_addr, (int)info->ai_addrlen, nullptr, nullptr, nullptr, nullptr);
	if (iResult == SOCKET_ERROR) {
		switch (WSAGetLastError())
		{
		case WSAEWOULDBLOCK:
			if (FAILED(hr = WaitConnect(sock))) {
				dprintf("WaitConnect failed [0x%08x]\n", hr);
				return hr;
			}
			break;
		default:
			dprintf("connect error\n");
			closesocket(sock);
			sock = INVALID_SOCKET;
			return E_FAIL;
		}
	}


	if (sock == INVALID_SOCKET) {
		dprintf("Unable to connect to server.\n");
		return E_FAIL;
	}

	//struct tcp_keepalive keepalive_vals = {
	//	(u_long)true,
	//	(u_long)500,
	//	(u_long)1000
	//};
	//DWORD bytesRead;
	//err = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &keepalive_vals,
	//					sizeof(keepalive_vals), nullptr, 0, &bytesRead, nullptr, nullptr);
	//if (err) {
	//	dprintf("WSAIoctl failed");
	//	return E_UNEXPECTED;
	//}

	mSocket = sock;
	return S_OK;
}

HRESULT Proxy::Disconnect()
{
	if (mSocket == INVALID_SOCKET)
		return S_FALSE;

	int iResult = shutdown(mSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR) {
		dprintf("shutdown failed [%d]\n", WSAGetLastError());
	}

	return Reset();
}

HRESULT Proxy::Send(LPVOID aBuffer, DWORD aBufferSize)
{
	if (mSocket == INVALID_SOCKET)
		return E_FAIL;

	memcpy(mSendBuf, aBuffer, aBufferSize);
	mBytesToSend = aBufferSize;
	WSABUF buf;
	buf.buf = mSendBuf;
	buf.len = mBytesToSend;
	int err = WSASend(mSocket, &buf, 1, &mBytesSent, 0, &mSendOverlap, nullptr);
	if (err && WSAGetLastError() != WSA_IO_PENDING) {
		dprintf("WSASend failed [%d]\n", WSAGetLastError());
		Disconnect();
		return E_FAIL;
	}
	DWORD flags = 0;
	if (!WSAGetOverlappedResult(mSocket, &mSendOverlap, &mBytesSent, TRUE, &flags)) {
		return E_FAIL;
	}
	//dprintf("[%p] Sending %d bytes, %d bytes sent\n", this, mBytesToSend, mBytesSent);
	//HexPrint("SEND:", aBuffer, mBytesSent);
	return S_OK;
}

HRESULT Proxy::Recv(LPVOID aBuffer, DWORD aBufferSize, LPDWORD aBytesReceived)
{
	if (!aBytesReceived)
		return E_POINTER;
	if (mSocket == INVALID_SOCKET)
		return E_FAIL;

	mRecvBuf = aBuffer;

	WSABUF buf;
	DWORD flags = 0;
	buf.buf = (CHAR*)aBuffer;
	buf.len = aBufferSize;
	int err = WSARecv(mSocket, &buf, 1, aBytesReceived, &flags, &mRecvOverlap, nullptr);
	if (err && WSAGetLastError() != WSA_IO_PENDING) {
		dprintf("WSARecv failed [%d]\n", WSAGetLastError());
		Disconnect();
		return E_FAIL;
	}

	DWORD bytesReceived = 0;
	if (!WSAGetOverlappedResult(mSocket, &mRecvOverlap, &bytesReceived, FALSE, &flags)) {
		if (WSAGetLastError() == WSA_IO_INCOMPLETE) {
			return S_FALSE;
		}
		dprintf("WSAGetOverlappedResult() failed [%d]\n", WSAGetLastError());
		return E_FAIL;
	}

	*aBytesReceived = mBytesRecv = bytesReceived;
	if (mBytesRecv) {
		//dprintf("[%p] Receiving %d bytes\n", this, *aBytesReceived);
		//HexPrint("RECV:", aBuffer, *aBytesReceived);
	}
	return S_OK;
}

BOOL Proxy::UpdateSendResult()
{
	return SendComplete();
}

BOOL Proxy::UpdateRecvResult()
{
	DWORD flags = 0;
	DWORD bytesReceived = 0;
	if (!WSAGetOverlappedResult(mSocket, &mRecvOverlap, &bytesReceived, FALSE, &flags)) {
		dprintf("WSAGetOverlappedResult() failed [%d]\n", WSAGetLastError());
		return FALSE;
	}
	mBytesRecv = bytesReceived;
	if (mBytesRecv) {
		//dprintf("[%p] Receiving %d bytes\n", this, bytesReceived);
		//HexPrint("RECV:", mRecvBuf, bytesReceived);
	}
	return TRUE;
}

HRESULT Proxy::Reset()
{
	if (mSocket != INVALID_SOCKET) {
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}
	if (mSendOverlap.hEvent) {
		WSACloseEvent(mSendOverlap.hEvent);
		mSendOverlap.hEvent = nullptr;
	}
	if (mRecvOverlap.hEvent) {
		WSACloseEvent(mRecvOverlap.hEvent);
		mRecvOverlap.hEvent = nullptr;
	}
	return S_OK;
}

HRESULT Proxy::CreateSocket(LPCSTR aHost, LPCSTR aPort, SOCKET& aSock, SmartAddrInfo& aInfo)
{
	struct addrinfo *result = nullptr;
	struct addrinfo hints;
	SmartAddrInfo ptr;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int iResult = GetAddrInfoA(aHost, aPort, &hints, &result);
	if (iResult != 0) {
		dprintf("getaddrinfo error [%d]\n", iResult);
		return E_FAIL;
	}

	ptr = SmartAddrInfo(result, &freeaddrinfo);
	SOCKET sock = WSASocketW(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		dprintf("socket error [%ld]\n", WSAGetLastError());
		return E_FAIL;
	}

	aSock = sock;
	aInfo = ptr;
	return S_OK;
}
