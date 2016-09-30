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

	HRESULT Send(LPVOID aBuffer, DWORD aBufferSize, LPWSAOVERLAPPED aOverlapped);
	HRESULT Recv(LPVOID aBuffer, DWORD aBufferSize, LPDWORD aBytesReceived, LPWSAOVERLAPPED aOverlapped);
	HRESULT GetOverlappedResult(LPWSAOVERLAPPED aOverlapped, LPDWORD aBytesTransfered, BOOL aWait);

	SOCKET GetSocket() const { return mSocket; }

private:
	HRESULT CreateSocket(LPCSTR aHost, LPCSTR aPort, SOCKET& aSock, SmartAddrInfo& aInfo);

	SOCKET mSocket;
};

typedef Proxy PROXY, *LPPROXY;
