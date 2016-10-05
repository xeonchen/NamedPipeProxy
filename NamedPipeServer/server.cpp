#include "stdafx.h"
#include "debug.h"
#include "io_center.h"
#include "proxy.h"
#include "server.h"

#define PIPE_TIMEOUT 5000
#define BUFSIZE 65535
#define INSTANCES 64

typedef struct ConnInst {
	HANDLE pipe;
	LPPROXY proxy;

	BOOL closed;

	OVERLAPPED readOverlap;
	CHAR readBuf[BUFSIZE];
	DWORD bytesRead;

	OVERLAPPED writeOverlap;
	CHAR writeBuf[BUFSIZE];
	DWORD bytesWrite;
	DWORD bytesWritten;

	WSAOVERLAPPED sendOverlap;
	CHAR sendBuf[BUFSIZE];
	DWORD bytesSend;
	DWORD bytesSent;

	WSAOVERLAPPED recvOverlap;
	CHAR recvBuf[BUFSIZE];
	DWORD bytesRecv;
} CONNINST, *LPCONNINST;

CONNINST gInstances[INSTANCES];
DWORD nextInstance = 0;

BOOL CreatePipeServer();
BOOL CreateAndConnectInstance(LPOVERLAPPED aOverlapped, LPHANDLE aPipe);
BOOL ConnectToNewClient(HANDLE aPipe, LPOVERLAPPED aOverlapped);
VOID DisconnectAndClose(LPCONNINST aPipeInst);

void ReadComplete(LPCONNINST inst);
void WriteComplete(LPCONNINST inst);
void SendComplete(LPCONNINST inst);
void RecvComplete(LPCONNINST inst);

IOCenter io;

void ReadComplete(LPCONNINST inst)
{
	if (inst->closed) {
		return;
	}

	HexPrint("ReadComplete:", inst->readBuf, inst->bytesRead);

	memcpy(inst->sendBuf, inst->readBuf, inst->bytesRead);
	inst->bytesSend = inst->bytesRead;
	inst->bytesSent = 0;

	memset(inst->readBuf, 0x1C, sizeof(inst->readBuf));
	inst->bytesRead = 0;

	if (inst->proxy) {
		inst->proxy->Send(inst->sendBuf, inst->bytesSend, &inst->sendOverlap);
	}
}

void WriteComplete(LPCONNINST inst)
{
	if (inst->closed) {
		return;
	}

	HexPrint("WriteComplete:", inst->writeBuf, inst->bytesWritten);

	memset(inst->writeBuf, 0x1C, sizeof(inst->writeBuf));
	inst->bytesWrite = inst->bytesWritten = 0;

	if (inst->proxy) {
		inst->proxy->Recv(inst->recvBuf, sizeof(inst->recvBuf), &inst->bytesRecv, &inst->recvOverlap);
	}
}

void SendComplete(LPCONNINST inst)
{
	if (inst->closed) {
		return;
	}

	HexPrint("SendComplete:", inst->sendBuf, inst->bytesSent);

	memset(inst->sendBuf, 0x1C, sizeof(inst->sendBuf));
	inst->bytesSend = inst->bytesSent = 0;

	if (inst->pipe) {
		ReadFile(inst->pipe, inst->readBuf, sizeof(inst->readBuf), &inst->bytesRead, &inst->readOverlap);
	}
}

void RecvComplete(LPCONNINST inst)
{
	if (inst->closed) {
		return;
	}

	HexPrint("RecvComplete:", inst->recvBuf, inst->bytesRecv);

	memcpy(inst->writeBuf, inst->recvBuf, inst->bytesRecv);
	inst->bytesWrite = inst->bytesRecv;
	inst->bytesWritten = 0;

	memset(inst->recvBuf, 0x1C, sizeof(inst->recvBuf));
	inst->bytesRecv = 0;

	if (inst->pipe) {
		WriteFile(inst->pipe, inst->writeBuf, inst->bytesWrite, &inst->bytesWritten, &inst->writeOverlap);
	}
}

void IoCallback(ULONG_PTR ptr, DWORD aBytesTransferred, LPVOID aOverlapped)
{
	LPCONNINST inst = (LPCONNINST)ptr;
	if (!inst || !inst->pipe || !inst->proxy || inst->closed) {
		return;
	}

	if (aBytesTransferred == (DWORD)-1) {
		DisconnectAndClose(inst);
		return;
	}

	BOOL success = FALSE;
	DWORD bytesTransferred = 0;
	if (aOverlapped == (LPVOID)(&inst->readOverlap)) {
		success = GetOverlappedResult(inst->pipe, &inst->readOverlap, &bytesTransferred, FALSE);
		if (success) {
			_ASSERTE(aBytesTransferred == bytesTransferred);
			inst->bytesRead = bytesTransferred;
			ReadComplete(inst);
		} else {
			dprintf("GetOverlapped Result failed (%d)\n", GetLastError());
		}
	} else if (aOverlapped == (LPVOID)(&inst->writeOverlap)) {
		success = GetOverlappedResult(inst->pipe, &inst->writeOverlap, &bytesTransferred, FALSE);
		if (success) {
			_ASSERTE(aBytesTransferred == bytesTransferred);
			inst->bytesWritten = bytesTransferred;
			_ASSERTE(inst->bytesWrite == inst->bytesWritten);
			WriteComplete(inst);
		} else {
			dprintf("GetOverlapped Result failed (%d)\n", GetLastError());
		}
	} else if (aOverlapped == (LPVOID)(&inst->sendOverlap)) {
		success = SUCCEEDED(inst->proxy->GetOverlappedResult(&inst->sendOverlap, &bytesTransferred, FALSE));
		if (success) {
			_ASSERTE(aBytesTransferred == bytesTransferred);
			inst->bytesSent = bytesTransferred;
			_ASSERTE(inst->bytesSend == inst->bytesSent);
			SendComplete(inst);
		} else {
			dprintf("GetOverlapped Result failed (%d)\n", WSAGetLastError());
		}
	} else if (aOverlapped == (LPVOID)(&inst->recvOverlap)) {
		success = SUCCEEDED(inst->proxy->GetOverlappedResult(&inst->recvOverlap, &bytesTransferred, FALSE));
		if (success) {
			_ASSERTE(aBytesTransferred == bytesTransferred);
			inst->bytesRecv = bytesTransferred;
			RecvComplete(inst);
		} else {
			dprintf("GetOverlapped Result failed (%d)\n", WSAGetLastError());
		}
	}
	else {
		_ASSERTE("How do you turn this on?");
	}
}

static void StartProxy(HANDLE aPipe, IOCenter& aIo)
{
	LPCONNINST pipeInst = nullptr;
	for (int i = 0; i < INSTANCES; ++i) {
		DWORD index = (nextInstance + i) % INSTANCES;
		if (gInstances[index].closed) {
			pipeInst = &gInstances[index];
			pipeInst->closed = FALSE;
			break;
		}
	}
	if (!pipeInst) {
		dprintf("GlobalAlloc failed [%d]\n", GetLastError());
		return;
	}

	SecureZeroMemory(pipeInst, sizeof(CONNINST));

	pipeInst->pipe = aPipe;
	pipeInst->proxy = new Proxy();

	if (FAILED(pipeInst->proxy->Connect("127.0.0.1", "9050"))) {
		dprintf("Connect failed\n");
		DisconnectAndClose(pipeInst);
	}
	else {
		dprintf("[%p] new client connected\n", pipeInst->proxy);

		pipeInst->readOverlap.hEvent = ::CreateEventA(nullptr, TRUE, FALSE, "readOverlap");
		pipeInst->writeOverlap.hEvent = ::CreateEventA(nullptr, TRUE, FALSE, "writeOverlap");
		pipeInst->sendOverlap.hEvent = WSACreateEvent();
		pipeInst->recvOverlap.hEvent = WSACreateEvent();

		IOCenter::StartFunction startRead = std::bind(SendComplete, pipeInst);
		aIo.AddObserver(pipeInst->pipe, startRead, (ULONG_PTR)pipeInst);
		IOCenter::StartFunction startRecv = std::bind(WriteComplete, pipeInst);
		aIo.AddObserver((HANDLE)pipeInst->proxy->GetSocket(), startRecv, (ULONG_PTR)pipeInst);
	}
}

BOOL CreatePipeServer()
{
	dprintf("CreatePipeServer\n");

	HANDLE connectEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (!connectEvent) {
		dprintf("CreateEvent failed [%d].\n", GetLastError());
		return FALSE;
	}

	for (int i = 0; i < INSTANCES; ++i) {
		gInstances[i].closed = TRUE;
	}

	OVERLAPPED oConnect;
	oConnect.hEvent = connectEvent;

	HANDLE hPipe;
	BOOL hasPendingIO = CreateAndConnectInstance(&oConnect, &hPipe);

	io.SetCallback(&IoCallback);

	while (TRUE) {
		switch (WaitForSingleObjectEx(connectEvent, INFINITE, TRUE)) {
		case WAIT_OBJECT_0:

			if (hasPendingIO) {
				DWORD bytesRead;
				if (!GetOverlappedResult(hPipe, &oConnect, &bytesRead, FALSE)) {
					return FALSE;
				}
			}

			StartProxy(hPipe, io);

			hasPendingIO = CreateAndConnectInstance(&oConnect, &hPipe);
			break;

		case WAIT_IO_COMPLETION:
			break;
		default:
			dprintf("WaitForSingleObjectEx failed [%d]\n", GetLastError());
			return FALSE;
		}
	}

	return TRUE;
}

// @return: TRUE if connection operation is pending.
//			FALSE if the connection has been completed.
BOOL CreateAndConnectInstance(LPOVERLAPPED aOverlapped, LPHANDLE aPipe)
{
	if (!aPipe) {
		dprintf("Parameter aPipe is NULL\n");
		return FALSE;
	}

	SECURITY_ATTRIBUTES attr;
	SecureZeroMemory(&attr, sizeof(SECURITY_ATTRIBUTES));
	attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	attr.bInheritHandle = TRUE;
	attr.lpSecurityDescriptor = nullptr;

	// FIXME: adjust parameters
	*aPipe = CreateNamedPipeA(
		PIPE_NAME,
		PIPE_ACCESS_DUPLEX |
		FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE |
		PIPE_READMODE_MESSAGE |
		PIPE_WAIT,
		INSTANCES ? INSTANCES : PIPE_UNLIMITED_INSTANCES,
		BUFSIZE,
		BUFSIZE,
		PIPE_TIMEOUT,
		&attr);

	if (*aPipe == INVALID_HANDLE_VALUE) {
		dprintf("CreateNamedPipe failed [%d]\n", GetLastError());
		return FALSE;
	}

	return ConnectToNewClient(*aPipe, aOverlapped);
}

BOOL ConnectToNewClient(HANDLE aPipe, LPOVERLAPPED aOverlapped)
{
	if (ConnectNamedPipe(aPipe, aOverlapped)) {
		dprintf("Unexpected, overlapped ConnectNamedPipe() always returns 0.\n");
		return FALSE;
	}

	switch (GetLastError())
	{
	case ERROR_IO_PENDING:
		return TRUE;

	case ERROR_PIPE_CONNECTED:
		if (SetEvent(aOverlapped->hEvent))
			break;

	default: // error
		dprintf("ConnectNamedPipe failed [%d]\n", GetLastError());
		break;
	}

	return FALSE;
}


VOID DisconnectAndClose(LPCONNINST aPipeInst)
{
	_ASSERTE(aPipeInst);
	if (aPipeInst->closed) {
		return;
	}

	aPipeInst->closed = TRUE;
	dprintf("DisconnectAndClose %p\n", aPipeInst);

	io.RemoveObserver((ULONG_PTR)&aPipeInst);
	io.RemoveObserver((ULONG_PTR)&aPipeInst);

	if (aPipeInst->proxy) {
		if (FAILED(aPipeInst->proxy->Disconnect())) {
			dprintf("Disconnect proxy failed\n");
		}
		delete aPipeInst->proxy;
		aPipeInst->proxy = nullptr;
	}

	if (aPipeInst->pipe) {
		if (!::DisconnectNamedPipe(aPipeInst->pipe)) {
			dprintf("DisconnectNamedPipe failed [%d]\n", GetLastError());
		}
		::CloseHandle(aPipeInst->pipe);
	}

	if (aPipeInst->readOverlap.hEvent) {
		::CloseHandle(aPipeInst->readOverlap.hEvent);
		aPipeInst->readOverlap.hEvent = nullptr;
	}

	if (aPipeInst->writeOverlap.hEvent) {
		::CloseHandle(aPipeInst->writeOverlap.hEvent);
		aPipeInst->writeOverlap.hEvent = nullptr;
	}
}

HRESULT CreateServer()
{
	return CreatePipeServer() ? S_OK : E_FAIL;
}
