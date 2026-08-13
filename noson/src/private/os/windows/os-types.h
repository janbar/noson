#pragma once

#if !defined(__WINDOWS__)
#define __WINDOWS__
#endif

/* Enable LEAN_AND_MEAN support */
#define WIN32_LEAN_AND_MEAN

/* Don't define min() and max() to prevent a clash with std::min() and std::max */
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

/* Disable warning C4005: '_WINSOCKAPI_' : macro redefinition */
#pragma warning(disable:4005)
#include <WinSock2.h>
#pragma warning(default:4005)
#include <Windows.h>
#include <wchar.h>
#include <time.h>
#include <sys/timeb.h>
#include <io.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/* Prevent inclusion of wingdi.h */
#define NOGDI

#ifndef _SSIZE_T_DEFINED
#if (defined(_WIN64) || defined(_M_ARM64))
typedef __int64    ssize_t;
#else
typedef _W64 int   ssize_t;
#endif
#define _SSIZE_T_DEFINED
#endif

#ifndef PATH_MAX
#ifdef _MAX_PATH
#define PATH_MAX _MAX_PATH
#else
#define PATH_MAX 256
#endif
#endif

#ifndef PATH_SEPARATOR_CHAR
#define PATH_SEPARATOR_CHAR         '\\'
#define PATH_SEPARATOR_STRING       "\\"
#endif

#define INVALID_SOCKET_VALUE        INVALID_SOCKET
typedef SOCKET net_socket_t;

__inline int usleep(unsigned int usec)
{
  Sleep((DWORD)(usec / 1000));
  return 0;
}

__inline unsigned int sleep(unsigned int sec)
{
  Sleep((DWORD)(sec * 1000));
  return 0;
}

struct timezone
{
  int	tz_minuteswest;
  int	tz_dsttime;
};

#if defined(_MSC_VER)

#if (_MSC_VER < 1800)
#include "msc_inttypes.h"
#define atoll(S) _atoi64(S)
#else
#include <inttypes.h>
#endif

#if (_MSC_VER < 1900)
#define snprintf _snprintf
#endif

#define strnicmp _strnicmp
#define strncasecmp _strnicmp
#define stricmp _stricmp
#define strcasecmp _stricmp

#else
#include <inttypes.h>
#endif /* _MSC_VER */
