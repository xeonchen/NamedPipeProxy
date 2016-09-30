#pragma once

class IOCenter final
{
public:
	using StartFunction = std::function<void(void)>;
	using CallbackFuncion = std::function<void(ULONG_PTR, DWORD, LPVOID)>;

	explicit IOCenter();
	~IOCenter();

	HRESULT AddObserver(HANDLE aHandle, StartFunction aFunc, ULONG_PTR aParam);
	HRESULT RemoveObserver(ULONG_PTR aParam);
	CallbackFuncion SetCallback(CallbackFuncion aCallback);

private:
	HRESULT Initialize();
	HRESULT Finalize();

	void ThreadProc(HANDLE aIocp);

	HANDLE mIocp;
	std::atomic<bool> mStopped;
	CallbackFuncion mCallback;
	std::mutex mLock;
	std::set<ULONG_PTR> mObservers; // protected by mLock
	std::list<StartFunction> mInitFunctions; // protected by mLock

	std::unique_ptr<std::thread> mThread;
};
