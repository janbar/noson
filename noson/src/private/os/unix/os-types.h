#pragma once

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

/* Enable Large File Support (LFS) */
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__APPLE__)
#include <sys/cdefs.h>
#include <sys/syslimits.h>
#elif defined(__FreeBSD__)
#include <limits.h>
#elif defined(__GNU__) || defined(__linux__) || defined(__sun)
#include <limits.h>
#endif

#if defined(__APPLE__)
#include <stdio.h> /* for fpos_t */
#include <AvailabilityMacros.h>
typedef int64_t   off64_t;
typedef off_t     __off_t;
typedef off64_t   __off64_t;
typedef fpos_t    fpos64_t;
#define stat64    stat
#define statfs64  statfs
#define fstat64   fstat
#elif defined(__FreeBSD__)
#include <stdio.h> /* for fpos_t */
typedef int64_t   off64_t;
typedef off_t     __off_t;
typedef off64_t   __off64_t;
typedef fpos_t    fpos64_t;
#define stat64    stat
#define statfs64  statfs
#define fstat64   fstat
#endif

#ifndef PATH_MAX
#warning "PATH_MAX is not defined"
#define PATH_MAX 256
#endif

#ifndef PATH_SEPARATOR_CHAR
#define PATH_SEPARATOR_CHAR         '/'
#define PATH_SEPARATOR_STRING       "/"
#endif

#define INVALID_SOCKET_VALUE        (-1)
typedef int net_socket_t;
