#pragma once

class IOCenter final
{
public:
	using StartFunction = std::function<void(void)>;
	using CallbackFuncion = std::function<void(ULONG_PTR, DWORD)>;

	explicit IOCenter();
	~IOCenter();

	HRESULT AddHandle(HANDLE aHandle, StartFunction aFunc, ULONG_PTR aParam);
	CallbackFuncion SetCallback(CallbackFuncion aCallback);

private:
	HRESULT Initialize();
	HRESULT Finalize();

	void ThreadProc(HANDLE aIocp);

	HANDLE mIocp;
	std::atomic<bool> mStopped;
	CallbackFuncion mCallback;
	std::unique_ptr<std::thread> mThread;
};
