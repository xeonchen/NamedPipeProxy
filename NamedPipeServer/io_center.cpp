#include "stdafx.h"
#include "debug.h"
#include "io_center.h"
#include "proxy.h"


IOCenter::IOCenter()
	: mIocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0))
	, mStopped(false)
	, mCallback(nullptr)
	, mThread(std::make_unique<std::thread>(std::bind(&IOCenter::ThreadProc, this, mIocp)))
{
}

IOCenter::~IOCenter()
{
	Finalize();
}

HRESULT IOCenter::Initialize()
{
	return S_OK;
}

HRESULT IOCenter::Finalize()
{
	if (mThread) {
		mStopped = true;
		mThread->join();
		mThread = nullptr;
	}

	if (mIocp && mIocp != INVALID_HANDLE_VALUE) {
		::CloseHandle(mIocp);
		mIocp = nullptr;
	}

	return S_OK;
}

HRESULT IOCenter::AddHandle(HANDLE aHandle, StartFunction aFunc, ULONG_PTR aParam)
{
	if (!mIocp || mIocp == INVALID_HANDLE_VALUE) {
		return E_FAIL;
	}
	if (!aHandle || aHandle == INVALID_HANDLE_VALUE) {
		return E_INVALIDARG;
	}

	HANDLE h = ::CreateIoCompletionPort(aHandle, mIocp, aParam, 0);
	if (h != mIocp) {
		::CloseHandle(h);
		return E_UNEXPECTED;
	}

	if (aFunc) {
		aFunc();
	}

	return S_OK;
}

IOCenter::CallbackFuncion IOCenter::SetCallback(CallbackFuncion aCallback)
{
	CallbackFuncion callback(mCallback);
	mCallback = aCallback;
	return callback;
}

void IOCenter::ThreadProc(HANDLE aIocp)
{
	DWORD bytesTransferred = 0;
	ULONG_PTR ptr = 0;
	LPOVERLAPPED overlap = {};
	BOOL ret = FALSE;

	while (!mStopped) {
		ret = GetQueuedCompletionStatus(aIocp, &bytesTransferred, &ptr, &overlap, 1000);
		if (ret) {
			if (bytesTransferred) {
				//dprintf("[%s][%p] bytesTransferred = %d\n", __func__, ptr, bytesTransferred);
			}
			if (mCallback) {
				mCallback(ptr, bytesTransferred);
			}
		} else {
			switch (GetLastError()) {
			case ERROR_BROKEN_PIPE:
			case ERROR_PIPE_NOT_CONNECTED:
				if (mCallback) {
					mCallback(ptr, (DWORD)-1);
				}
				break;
			case ERROR_OPERATION_ABORTED:
				dprintf("connection closed\n");
				break;
			case WAIT_TIMEOUT:
				continue;
			default:
				dprintf("GetQueuedCompletionStatus error (%d)\n", GetLastError());
				break;
			}
		}
	}
}
