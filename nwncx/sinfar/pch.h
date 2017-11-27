#pragma once

#include <cstdio>
#include <stdarg.h>
#include <cmath>

#ifdef WIN32
#include <Windows.h>
#include <Winsock.h>
#elif __linux__
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

//remove "decorated name length exceeded, name was truncated" warning
#ifdef WIN32
#pragma warning(disable : 4503)
#endif

#include "nwncx_sinfar.h"
#include "CXImage.h"
#include "resman.h"