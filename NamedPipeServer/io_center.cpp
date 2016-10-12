#include "stdafx.h"
#include "debug.h"
#include "io_center.h"
#include "proxy.h"


IOCenter::IOCenter()
	: mIocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1))
	, mStopped(false)
	, mCallback(nullptr)
	, mLock()
	, mObservers()
	, mInitFunctions()
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

HRESULT IOCenter::AddObserver(HANDLE aHandle, StartFunction aFunc, ULONG_PTR aParam)
{
	if (!mIocp || mIocp == INVALID_HANDLE_VALUE) {
		return E_FAIL;
	}
	if (!aHandle || aHandle == INVALID_HANDLE_VALUE) {
		return E_INVALIDARG;
	}

	HANDLE h = ::CreateIoCompletionPort(aHandle, mIocp, aParam, 1);
	if (h != mIocp) {
		::CloseHandle(h);
		return E_UNEXPECTED;
	}

	{
		std::lock_guard<std::mutex> g(mLock);
		mObservers.insert(aParam);
		if (aFunc) {
			mInitFunctions.push_back(aFunc);
		}
	}

	return S_OK;
}

HRESULT IOCenter::RemoveObserver(ULONG_PTR aParam)
{
	std::lock_guard<std::mutex> g(mLock);
	mObservers.erase(aParam);
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
	LPOVERLAPPED overlap = nullptr;
	BOOL ret = FALSE;
	BOOL valid = FALSE;

	while (!mStopped) {
		while (true) {
			std::unique_lock<std::mutex> g(mLock);
			if (mInitFunctions.empty()) {
				break;
			}

			StartFunction func = mInitFunctions.front();
			mInitFunctions.pop_front();
			g.unlock();

			func();
		}
		{
			ret = GetQueuedCompletionStatus(aIocp, &bytesTransferred, &ptr, &overlap, 1000);
			std::lock_guard<std::mutex> g(mLock);
			valid = mObservers.find(ptr) != mObservers.end();
		}

		if (ret) {
			if (valid && mCallback) {
				mCallback(ptr, bytesTransferred, overlap);
			}
			continue;
		}

		switch (GetLastError()) {
		case WAIT_TIMEOUT:
			continue;
		default:
			if (valid && mCallback) {
				dprintf("[ThreadProc] GetQueuedCompletionStatus error (%d)\n", GetLastError());
				mCallback(ptr, (DWORD)-1, overlap);
			}
			break;
		}
	}
}
