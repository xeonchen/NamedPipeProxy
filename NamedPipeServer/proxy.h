#pragma once

#include <memory>
#include <type_traits>

using SmartAddrInfo = std::shared_ptr<struct addrinfo>;

class Proxy final
{
public:
	Proxy();
	virtual ~Proxy();

	HRESULT Connect(LPCSTR aHost, LPCSTR aPort);
	HRESULT Disconnect();

	HRESULT Send(LPVOID aBuffer, DWORD aBufferSize);
	HRESULT Recv(LPVOID aBuffer, DWORD aBufferSize, LPDWORD aBytesReceived);

	BOOL SendComplete(DWORD aTimeout = 0) const { return WSAWaitForMultipleEvents(1, &mSendOverlap.hEvent, TRUE, aTimeout, TRUE) == WSA_WAIT_EVENT_0; };
	BOOL RecvComplete(DWORD aTimeout = 0) const { return WSAWaitForMultipleEvents(1, &mRecvOverlap.hEvent, TRUE, aTimeout, TRUE) == WSA_WAIT_EVENT_0; };

	VOID ResetSend() { WSAResetEvent(mSendOverlap.hEvent); }
	VOID ResetRecv() { WSAResetEvent(mRecvOverlap.hEvent); }

	DWORD BytesSend() const { return mBytesToSend; }
	DWORD BytesSent() const { return mBytesSent; }
	DWORD BytesRecv() const { return mBytesRecv; }

	BOOL UpdateSendResult();
	BOOL UpdateRecvResult();

	SOCKET GetSocket() const { return mSocket; }

	HRESULT Reset();
private:
	enum { BUFSIZE = 65535 };

	HRESULT CreateSocket(LPCSTR aHost, LPCSTR aPort, SOCKET& aSock, SmartAddrInfo& aInfo);

	SOCKET mSocket;

	// send
	WSAOVERLAPPED mSendOverlap;
	CHAR mSendBuf[BUFSIZE];
	DWORD mBytesToSend = 0;
	DWORD mBytesSent = 0;

	// recv
	WSAOVERLAPPED mRecvOverlap;
	LPVOID mRecvBuf = nullptr;
	DWORD mBytesRecv = 0;
};

typedef Proxy PROXY, *LPPROXY;
