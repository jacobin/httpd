#ifndef __HTTPD_H__
#define __HTTPD_H__

#define _CRT_SECURE_NO_WARNINGS

#ifdef  FD_SETSIZE
#undef  FD_SETSIZE
#define FD_SETSIZE  1024
#else 
#define FD_SETSIZE  1024
#endif

#define BUFFER_UNIT 4096

#ifdef _DEBUG
#define ASSERT(x)   assert(x)
#else
#define ASSERT(x)   if (!(x)) log_error("{%s:%d} ASSERT("#x") failed.", __FUNCTION__, __LINE__);
#endif

#include <stdio.h>
#include <Winsock2.h>
#include <Windows.h>
#include <assert.h>
#include <time.h>
#include "types.h"
#include "utils.h"
#include "Logger.h"
#include "network.h"
#include "event.h"
#include "http.h"

#pragma pack(1)
#pragma comment(lib, "Ws2_32.lib")

#endif