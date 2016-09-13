#pragma once

#define PIPE_NAME "\\\\.\\pipe\\MyTest"

#ifdef __cplusplus
extern "C" {
#endif

HRESULT CreateServer();

#ifdef __cplusplus
}
#endif
