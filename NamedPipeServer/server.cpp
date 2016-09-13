#include "stdafx.h"
#include "debug.h"
#include "io_center.h"
#include "proxy.h"
#include "server.h"

#define PIPE_TIMEOUT 5000
#define BUFSIZE 65535
#define INSTANCES 32
#define MAXRETRY 120
#define RETRY_WAIT 500 // ms

enum CallbackType {
	ePipe,
	eSock
};

struct PipeInst;

typedef struct CallbackParam {
	PipeInst* inst;
	CallbackType type;
} CALLBACKPARAM, *LPCALLBACKPARAM;

enum State {
	eIdle,
	eRead,
	eWrite,
};

typedef struct PipeInst {
	OVERLAPPED readOverlap;
	OVERLAPPED writeOverlap;

	HANDLE pipe;
	LPPROXY proxy;

	State requestState;
	State responseState;

	CHAR readBuf[BUFSIZE];
	DWORD bytesRead;
	CHAR writeBuf[BUFSIZE];
	DWORD bytesWrite;
	DWORD bytesWritten;
	CHAR sendBuf[BUFSIZE];
	DWORD bytesSend;
	DWORD bytesSent;
	CHAR recvBuf[BUFSIZE];
	DWORD bytesRecv;

	INT16 retry;

	CALLBACKPARAM param1;
	CALLBACKPARAM param2;
} CONNINST, *LPCONNINST;

BOOL CreatePipeServer();
BOOL CreateAndConnectInstance(LPOVERLAPPED aOverlapped, LPHANDLE aPipe);
BOOL ConnectToNewClient(HANDLE aPipe, LPOVERLAPPED aOverlapped);
VOID DisconnectAndClose(LPCONNINST aPipeInst);

void ReadComplete(LPCONNINST inst);
void WriteComplete(LPCONNINST inst);
void SendComplete(LPCONNINST inst);
void RecvComplete(LPCONNINST inst);

void ReadComplete(LPCONNINST inst)
{
	//HexPrint("ReadComplete:", inst->readBuf, inst->bytesRead);

	if (inst->bytesRead == 0) {
		if (inst->retry-- == 0) {
			dprintf("disconnect\n");
			DisconnectAndClose(inst);
			return;
		}
	} else {
		inst->retry = MAXRETRY;
	}

	memcpy(inst->sendBuf, inst->readBuf, inst->bytesRead);
	inst->bytesSend = inst->bytesRead;
	inst->bytesSent = 0;

	memset(inst->readBuf, 0, sizeof(inst->readBuf));
	inst->bytesRead = 0;

	inst->proxy->Send(inst->sendBuf, inst->bytesSend);
}

void WriteComplete(LPCONNINST inst)
{
	//HexPrint("WriteComplete:", inst->writeBuf, inst->bytesWritten);

	memset(inst->writeBuf, 0, sizeof(inst->writeBuf));
	inst->bytesWrite = inst->bytesWritten = 0;

	inst->proxy->Recv(inst->recvBuf, sizeof(inst->recvBuf), &inst->bytesRecv);
}

void SendComplete(LPCONNINST inst)
{
	//HexPrint("SendComplete:", inst->sendBuf, inst->bytesSent);
	inst->proxy->ResetSend();

	memset(inst->sendBuf, 0, sizeof(inst->sendBuf));
	inst->bytesSend = inst->bytesSent = 0;

	ReadFile(inst->pipe, inst->readBuf, sizeof(inst->readBuf), &inst->bytesRead, &inst->readOverlap);
}

void RecvComplete(LPCONNINST inst)
{
	//HexPrint("RecvComplete:", inst->recvBuf, inst->bytesRecv);
	inst->proxy->ResetRecv();

	if (inst->bytesRecv == 0) {
		if (inst->retry-- == 0) {
			dprintf("disconnect\n");
			DisconnectAndClose(inst);
			return;
		}
	} else {
		inst->retry = MAXRETRY;
	}

	memcpy(inst->writeBuf, inst->recvBuf, inst->bytesRecv);
	inst->bytesWrite = inst->bytesRecv;
	inst->bytesWritten = 0;

	memset(inst->recvBuf, 0, sizeof(inst->recvBuf));
	inst->bytesRecv = 0;

	WriteFile(inst->pipe, inst->writeBuf, inst->bytesWrite, &inst->bytesWritten, &inst->writeOverlap);
}

void PipeCallback(LPCONNINST aInst, DWORD aBytesTransferred)
{
	HANDLE pipe = aInst->pipe;

	if (!pipe) {
		return;
	}

	//dprintf("%s: aInst=%p, proxy=%p pipe=%p, aBytesTransferred=%d\n", __func__, aInst, aInst->proxy, aInst->pipe, aBytesTransferred);

	DWORD readEvent = WaitForSingleObject(aInst->readOverlap.hEvent, 0);
	DWORD writeEvent = WaitForSingleObject(aInst->writeOverlap.hEvent, 0);
	BOOL isRead = writeEvent != WAIT_OBJECT_0;
	BOOL isWritten = readEvent != WAIT_OBJECT_0;

	int counter = MAXRETRY;
	while (isRead && isWritten) { // nothing is complete
		// impossible, why this function called?
		dprintf("PipeCallback ERROR!\n");
		if (counter-- > 0) {
			readEvent = WaitForSingleObject(aInst->readOverlap.hEvent, RETRY_WAIT);
			writeEvent = WaitForSingleObject(aInst->writeOverlap.hEvent, RETRY_WAIT);
			isRead = writeEvent != WAIT_OBJECT_0;
			isWritten = readEvent != WAIT_OBJECT_0;
		}
		else {
			DisconnectAndClose(aInst);
			return;
		}
	}

	BOOL successRead = FALSE, successWrite = FALSE;
	DWORD bytesTransferred = 0;
	if (readEvent == WAIT_OBJECT_0) { // read complete
		successRead = GetOverlappedResult(pipe, &aInst->readOverlap, &bytesTransferred, FALSE);
		if (!successRead) {
			dprintf("GetOverlappedResult (read) failed, errno=%d\n", GetLastError());
		} else {
			aInst->bytesRead = bytesTransferred;
		}
	}
	if (writeEvent == WAIT_OBJECT_0) { // write complete
		successWrite = GetOverlappedResult(pipe, &aInst->writeOverlap, &bytesTransferred, FALSE);
		if (!successWrite) {
			dprintf("GetOverlappedResult (write) failed, errno=%d\n", GetLastError());
		} else {
			aInst->bytesWritten = bytesTransferred;
		}
	}

	if (!isRead && !isWritten) { // both are complete
		if (aInst->bytesRead == aInst->bytesWritten &&
			aInst->bytesRead == aBytesTransferred) {
			// equal, doesn't matter, choose recv
			isRead = TRUE;
		}
		else if (aInst->bytesWritten == aBytesTransferred) {
			isWritten = TRUE;
		}
		else if (aInst->bytesRead == aBytesTransferred) {
			isRead = TRUE;
		}
		else {
			// error, report below.
		}
	}

	if (isRead) {
		dprintf("[PipeCallback] Read %s (arg=%d, chk=%d)\n", successRead ? "success" : "failed", aBytesTransferred, aInst->bytesRead);
		ReadComplete(aInst);
	}
	else if (isWritten) {
		dprintf("[PipeCallback] Write %s (arg=%d, chk=%d, exp=%d)\n", successWrite ? "success" : "failed", aBytesTransferred, aInst->bytesWritten, aInst->bytesWrite);
		WriteComplete(aInst);
	}
	else {
		dprintf("ERROR! readEvent=%d, writeEvent=%d\n", readEvent, writeEvent);
		dprintf("ERROR! tr=%d readEvent=%d (%d), writeEvent=%d (%d, %d)\n",
			aBytesTransferred, readEvent, aInst->bytesRead, writeEvent, aInst->bytesWrite, aInst->bytesWritten);
	}
}

void SockCallback(LPCONNINST aInst, DWORD aBytesTransferred)
{
	LPPROXY proxy = aInst->proxy;

	if (!proxy) {
		return;
	}

	//dprintf("%s: aInst=%p, proxy=%p pipe=%p, aBytesTransferred=%d\n", __func__, aInst, aInst->proxy, aInst->pipe, aBytesTransferred);

	BOOL isSent = !proxy->RecvComplete();
	BOOL isRecv = !proxy->SendComplete();

	int counter = MAXRETRY;
	while (isSent && isRecv) { // nothing is complete
		// impossible, why this function called?
		dprintf("SockCallback ERROR!\n");
		
		if (counter-- > 0) {
			isSent = !proxy->RecvComplete(RETRY_WAIT);
			isRecv = !proxy->SendComplete(RETRY_WAIT);
		} else {
			DisconnectAndClose(aInst);
			return;
		}
	}

	BOOL successSend = FALSE, successRecv = FALSE;
	if (proxy->SendComplete()) {
		successSend = proxy->UpdateSendResult();
		if (!successSend) {
			dprintf("UpdateSendResult failed\n");
		}
		else {
			aInst->bytesSent = proxy->BytesSent();
		}
	}

	if (proxy->RecvComplete()) {
		successRecv = proxy->UpdateRecvResult();
		if (!successRecv) {
			dprintf("UpdateRecvResult failed\n");
		} else {
			aInst->bytesRecv = proxy->BytesRecv();
		}
	}

	if (!isSent && !isRecv) { // both are complete
		if (aInst->bytesSent == aInst->bytesRecv &&
			aInst->bytesSent == aBytesTransferred) {
			// equal, doesn't matter, choose recv
			isRecv = TRUE;
		} else if (aInst->bytesSent == aBytesTransferred) {
			isSent = TRUE;
		} else if (aInst->bytesRecv == aBytesTransferred) {
			isRecv = TRUE;
		} else {
			// error, report below.
		}
	}

	if (isSent) {
		dprintf("[SockCallback] Send %s (arg=%d, get=%d, exp=%d)\n", successSend ? "success" : "failed", aBytesTransferred, proxy->BytesSent(), proxy->BytesSend());
		SendComplete(aInst);
	}
	else if (isRecv) {
		dprintf("[SockCallback] Recv %s (arg=%d, get=%d)\n", successRecv ? "success" : "failed", aBytesTransferred, proxy->BytesRecv());
		RecvComplete(aInst);
	} else {
		dprintf("ERROR! tr=%d SendComplete=%d (%d, %d, %d), RecvComplete=%d (%d)\n",
			aBytesTransferred, proxy->SendComplete(), aInst->bytesSend, proxy->BytesSent(), proxy->BytesSend(),
			proxy->RecvComplete(), proxy->BytesRecv());
	}
}

void IoCallback(ULONG_PTR ptr, DWORD aBytesTransferred)
{
	LPCALLBACKPARAM param = (LPCALLBACKPARAM)ptr;
	if (!param || !param->inst) {
		return;
	}

	if (aBytesTransferred == (DWORD)-1) {
		DisconnectAndClose(param->inst);
		return;
	}

	switch (param->type) {
	case ePipe:
		PipeCallback(param->inst, aBytesTransferred);
		break;
	case eSock:
		SockCallback(param->inst, aBytesTransferred);
		break;
	default:
		dprintf("Ohhhhhh!!!!");
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

	OVERLAPPED oConnect;
	oConnect.hEvent = connectEvent;

	HANDLE hPipe;
	BOOL hasPendingIO = CreateAndConnectInstance(&oConnect, &hPipe);

	IOCenter io;
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

			LPCONNINST pipeInst;
			pipeInst = (LPCONNINST)GlobalAlloc(GPTR, sizeof(CONNINST));
			if (!pipeInst) {
				dprintf("GlobalAlloc failed [%d]\n", GetLastError());
				return FALSE;
			}

			pipeInst->retry = MAXRETRY;
			pipeInst->pipe = hPipe;
			pipeInst->proxy = new Proxy();
			if (FAILED(pipeInst->proxy->Connect("127.0.0.1", "9050"))) {
				dprintf("Connect failed\n");
				delete pipeInst->proxy;
				pipeInst->proxy = nullptr;
				DisconnectAndClose(pipeInst);
			} else {
				dprintf("[%p] new client connected\n", pipeInst->proxy);

				pipeInst->readOverlap.hEvent = ::CreateEventA(nullptr, TRUE, FALSE, "readOverlap");
				pipeInst->writeOverlap.hEvent = ::CreateEventA(nullptr, TRUE, FALSE, "writeOverlap");

				pipeInst->requestState = eRead;
				pipeInst->responseState = eIdle;

				pipeInst->param1.inst = pipeInst;
				pipeInst->param1.type = ePipe;
				pipeInst->param2.inst = pipeInst;
				pipeInst->param2.type = eSock;

				auto startRead = std::bind(ReadFile, pipeInst->pipe, pipeInst->readBuf, static_cast<DWORD>(sizeof(pipeInst->readBuf)), &pipeInst->bytesRead, &pipeInst->readOverlap);
				io.AddHandle(pipeInst->pipe, startRead, (ULONG_PTR)&pipeInst->param1);
				auto startRecv = std::bind(&Proxy::Recv, pipeInst->proxy, pipeInst->recvBuf, static_cast<DWORD>(sizeof(pipeInst->recvBuf)), &pipeInst->bytesRecv);
				io.AddHandle((HANDLE)pipeInst->proxy->GetSocket(), startRecv, (ULONG_PTR)&pipeInst->param2);
			}

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
		NULL);

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
	if (!aPipeInst)
		return;

	dprintf("DisconnectAndClose %p\n", aPipeInst->proxy);

	if (FAILED(aPipeInst->proxy->Disconnect())) {
		dprintf("Disconnect proxy failed\n");
	}
	delete aPipeInst->proxy;
	aPipeInst->proxy = nullptr;

	aPipeInst->param1.inst = nullptr;
	aPipeInst->param2.inst = nullptr;

	if (!DisconnectNamedPipe(aPipeInst->pipe)) {
		dprintf("DisconnectNamedPipe failed [%d]\n", GetLastError());
	}

	::CloseHandle(aPipeInst->readOverlap.hEvent);
	::CloseHandle(aPipeInst->writeOverlap.hEvent);

	CloseHandle(aPipeInst->pipe);
	//GlobalFree(aPipeInst);
}

HRESULT CreateServer()
{
	return CreatePipeServer() ? S_OK : E_FAIL;
}
