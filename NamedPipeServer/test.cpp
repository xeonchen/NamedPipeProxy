#include "stdafx.h"
#include "debug.h"
#include "io_center.h"
#include "test.h"
#include "proxy.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <thread>

HRESULT RunTests()
{
	auto impl = std::make_unique<Proxy>();
	CHAR buf[500] = "";
	DWORD n;
	HRESULT hr;

	IOCenter io;
	WSAOVERLAPPED overlapped;
	overlapped.hEvent = WSACreateEvent();

	if (FAILED(hr = impl->Connect("127.0.0.1", "1080"))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }

	if (FAILED(hr = io.AddObserver((HANDLE)impl->GetSocket(), nullptr, (ULONG_PTR)impl.get()))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }

	if (FAILED(hr = impl->Send("\x05\x01\00", 3, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Recv(buf, sizeof(buf), &n, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }

	if (FAILED(hr = impl->Send("\x05\x01\x00\x01\x5d\xb8\xd8\x22\x00\x50", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }

	if (FAILED(hr = impl->Recv(buf, sizeof(buf), &n, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }

	if (FAILED(hr = impl->Send("\x47\x45\x54\x20\x2f\x20\x48\x54\x54\x50", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x2f\x31\x2e\x31\x0d\x0a\x48\x6f\x73\x74", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x3a\x20\x65\x78\x61\x6d\x70\x6c\x65\x2e", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x63\x6f\x6d\x0d\x0a\x55\x73\x65\x72\x2d", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x41\x67\x65\x6e\x74\x3a\x20\x4d\x6f\x7a", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x69\x6c\x6c\x61\x2f\x35\x2e\x30\x20\x28", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x57\x69\x6e\x64\x6f\x77\x73\x20\x4e\x54", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x20\x31\x30\x2e\x30\x3b\x20\x57\x4f\x57", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x36\x34\x3b\x20\x72\x76\x3a\x35\x31\x2e", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x30\x29\x20\x47\x65\x63\x6b\x6f\x2f\x32", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x30\x31\x30\x30\x31\x30\x31\x20\x46\x69", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x72\x65\x66\x6f\x78\x2f\x35\x31\x2e\x30", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x0d\x0a\x41\x63\x63\x65\x70\x74\x3a\x20", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x74\x65\x78\x74\x2f\x68\x74\x6d\x6c\x2c", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x61\x70\x70\x6c\x69\x63\x61\x74\x69\x6f", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x6e\x2f\x78\x68\x74\x6d\x6c\x2b\x78\x6d", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x6c\x2c\x61\x70\x70\x6c\x69\x63\x61\x74", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x69\x6f\x6e\x2f\x78\x6d\x6c\x3b\x71\x3d", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x30\x2e\x39\x2c\x2a\x2f\x2a\x3b\x71\x3d", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x30\x2e\x38\x0d\x0a\x41\x63\x63\x65\x70", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x74\x2d\x4c\x61\x6e\x67\x75\x61\x67\x65", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x3a\x20\x65\x6e\x2d\x55\x53\x2c\x65\x6e", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x3b\x71\x3d\x30\x2e\x35\x0d\x0a\x41\x63", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x63\x65\x70\x74\x2d\x45\x6e\x63\x6f\x64", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x69\x6e\x67\x3a\x20\x67\x7a\x69\x70\x2c", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x20\x64\x65\x66\x6c\x61\x74\x65\x0d\x0a", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x43\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x3a\x20\x6b\x65\x65\x70\x2d\x61\x6c\x69", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x76\x65\x0d\x0a\x55\x70\x67\x72\x61\x64", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x65\x2d\x49\x6e\x73\x65\x63\x75\x72\x65", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x2d\x52\x65\x71\x75\x65\x73\x74\x73\x3a", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x20\x31\x0d\x0a\x50\x72\x61\x67\x6d\x61", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x3a\x20\x6e\x6f\x2d\x63\x61\x63\x68\x65", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x0d\x0a\x43\x61\x63\x68\x65\x2d\x43\x6f", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x6e\x74\x72\x6f\x6c\x3a\x20\x6e\x6f\x2d", 10, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Send("\x63\x61\x63\x68\x65\x0d\x0a\x0d\x0a", 9, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->Recv(buf, sizeof(buf), &n, &overlapped))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }
	if (FAILED(hr = impl->GetOverlappedResult(&overlapped, &n, TRUE))) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }

	if (FAILED(hr = impl->Disconnect())) { dprintf("line=%d, error=%08x\n", __LINE__, hr); return hr; }

	return S_OK;
}