/*
  Windows platform configuration for the vendored GameMonkeyScript VM.
  Keep the desktop pointer-sized integer adaptations used by this port while
  supplying the CRT/compiler details required by MSVC and MinGW.
*/

#ifndef _GMCONFIG_P_H_
#define _GMCONFIG_P_H_

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <new>
#include <io.h>

#define GM_LITTLE_ENDIAN 1
#if defined(__GNUC__)
#define GM_COMPILER_GCC
#endif
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#define GM_X86
#endif
#if defined(_WIN64) || defined(__x86_64__)
#define GM_PTR_SIZE_64
#define GM_DEFAULT_ALLOC_ALIGNMENT 16
#else
#define GM_DEFAULT_ALLOC_ALIGNMENT 4
#endif

#if defined(_MSC_VER)
#define GM_CDECL __cdecl
#define GM_FORCEINLINE __forceinline
#define _gmstricmp _stricmp
#define _gmsnprintf _snprintf
#define _gmvsnprintf _vsnprintf
#define alloca _alloca
#else
#define GM_CDECL
#define GM_FORCEINLINE inline
#define _gmstricmp _stricmp
#define _gmsnprintf snprintf
#define _gmvsnprintf vsnprintf
#ifndef alloca
#define alloca __builtin_alloca
#endif
#endif

/* Flex's generated scanner uses POSIX spellings for these CRT functions. */
#define isatty _isatty
#define fileno _fileno

#ifdef _DEBUG
#define GM_ASSERT(A) assert(A)
#define GM_DEBUG_BUILD
#else
#define GM_ASSERT(A)
#endif

#define GM_NL "\r\n"
#define GM_INLINE inline
#define GM_PRINTF printf
#define GM_NEW(alloc_params) new alloc_params
#define GM_PLACEMENT_NEW(alloc_params, address) new(address) alloc_params
#define GM_MAKE_ID32(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

#define GM_MIN_FLOAT32 -3.402823466e38f
#define GM_MAX_FLOAT32 3.402823466e38f
#define GM_MIN_FLOAT64 -1.7976931348623158e308
#define GM_MAX_FLOAT64 1.7976931348623158e308
#define GM_SMALLEST_FLOAT32 1.175494351e-38f
#define GM_SMALLEST_FLOAT64 2.2250738585072014e-308
#define GM_MIN_UINT8 0
#define GM_MAX_UINT8 255
#define GM_MIN_INT8 -128
#define GM_MAX_INT8 127
#define GM_MIN_UINT16 0
#define GM_MAX_UINT16 65535
#define GM_MIN_INT16 -32768
#define GM_MAX_INT16 32767
#define GM_MIN_UINT32 0
#define GM_MAX_UINT32 4294967295U
#define GM_MIN_INT32 (-2147483647 - 1)
#define GM_MAX_INT32 2147483647
#define GM_MAX_INT 2147483647
#define GM_MAX_SHORT 32767
#define GM_MAX_CHAR_STRING 256
#define GM_MAX_PATH 256

typedef const char* LPCTSTR;
typedef unsigned int gmuint;
typedef char gmint8;
typedef unsigned char gmuint8;
typedef short gmint16;
typedef unsigned short gmuint16;
typedef int gmint32;
typedef unsigned int gmuint32;
typedef int gmint;
typedef float gmfloat;
typedef long long gmint64;
typedef unsigned long long gmuint64;
typedef gmint64 gmptr;
typedef gmuint64 gmuptr;

#if defined(__GNUC__)
#define __int64 long long
#endif

#endif /* _GMCONFIG_P_H_ */
