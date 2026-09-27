/*
  Linux/GCC platform config for vendored GameMonkeyScript (vendor/gmscript).
  Upstream ships win32msvc/win32gcc/ps2cw variants only (see
  gmsrc/src/platform/) -- this is the missing Linux one, adapted from
  win32gcc's, kept outside the vendored tree so upstream stays a clean
  unmodified checkout. See engine/CMakeLists.txt for how this gets added to
  the include path ahead of gmsrc/src/gm itself.
*/

#ifndef _GMCONFIG_P_H_
#define _GMCONFIG_P_H_

#include <cstdlib>
#include <cstdint> // intptr_t/uintptr_t
#include <alloca.h> // alloca (glibc, not in malloc.h/stdlib.h)
#include <new>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <strings.h> // strcasecmp

#ifndef NDEBUG
  #define _DEBUG
#endif

// system defines

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
  #define GM_LITTLE_ENDIAN 1 // both x86_64 and (default) aarch64 are LE
#endif
#define GM_COMPILER_GCC
#define GM_X86

#define GM_CDECL
#ifdef _DEBUG
  #define GM_ASSERT(A)        assert(A)
#else //_DEBUG
  #define GM_ASSERT(A)
#endif //_DEBUG
#define GM_NL                 "\n"
#define GM_FORCEINLINE        inline
#define GM_INLINE             inline
#define _gmstricmp            strcasecmp
#define _gmsnprintf           snprintf
#define _gmvsnprintf           vsnprintf
#ifdef _DEBUG
  #define GM_DEBUG_BUILD
#endif // _DEBUG
#define GM_PRINTF             printf

#define GM_NEW( alloc_params ) new alloc_params
#define GM_PLACEMENT_NEW( alloc_params, address ) new(address) alloc_params

#define GM_DEFAULT_ALLOC_ALIGNMENT 4

#define GM_MAKE_ID32( a, b, c, d )  ( ((d)<<24) | ((c)<<16) | ((b)<<8) | (a))

#define GM_MIN_FLOAT32        -3.402823466e38f
#define GM_MAX_FLOAT32        3.402823466e38f

#define GM_MIN_FLOAT64        -1.7976931348623158e308
#define GM_MAX_FLOAT64        1.7976931348623158e308

#define GM_SMALLEST_FLOAT32   1.175494351e-38f
#define GM_SMALLEST_FLOAT64   2.2250738585072014e-308

#define GM_MIN_UINT8          0
#define GM_MAX_UINT8          255

#define GM_MIN_INT8           -128
#define GM_MAX_INT8           127

#define GM_MIN_UINT16         0
#define GM_MAX_UINT16         65535
#define GM_MIN_INT16          -32768
#define GM_MAX_INT16          32767

#define GM_MIN_UINT32         0
#define GM_MAX_UINT32         4294967295
#define GM_MIN_INT32          -2147483648
#define GM_MAX_INT32          2147483647

#define GM_MAX_INT            2147483647
#define GM_MAX_SHORT          32767

#define GM_MAX_CHAR_STRING    256
#define GM_MAX_PATH           256

// basic types
typedef const char * LPCTSTR;
typedef unsigned int gmuint;
typedef char gmint8;
typedef unsigned char gmuint8;
typedef short gmint16;
typedef unsigned short gmuint16;
typedef int gmint32;
typedef unsigned int gmuint32;
typedef gmint32 gmint;   // generic default int -- referenced throughout gm/ core
                         // (gmArraySimple::Count, gmStream::Seek/Tell/Write, ...).
                         // Present in platform/win32msvc/gmConfig_p.h but
                         // missing from the win32gcc variant this file was
                         // based on -- an upstream inconsistency, not
                         // something this port introduced.
typedef float gmfloat;

// gmUtil.h/gmByteCode.cpp reference MSVC's builtin `__int64`/`unsigned
// __int64` directly rather than a portable typedef -- another
// win32gcc-config gap (win32msvc doesn't need this since __int64 is a real
// MSVC keyword there). Macro-expand it to GCC's `long long` so `unsigned
// __int64` parses the same way `unsigned long long` would.
#ifndef __int64
#define __int64 long long
#endif
typedef long long gmint64;
typedef unsigned long long gmuint64;

// gmptr/gmuptr store real C++ pointers via direct cast (see
// gmObject::GetRef(), gmThread::PushString/Table/Function/NewUser) --
// upstream typedefs these as plain 32-bit int, correct for the PS3 retail
// target (confirmed 32-bit addressing despite the 64-bit PPU CPU, see
// host. We don't need PS3 pointer-width binary compatibility, just internal
// consistency, so these are native-pointer-width here -- specifically
// gmint64/gmuint64 (`long long`) rather than intptr_t/uintptr_t (`long` on
// this platform): gmStream's SwapEndian() overload set only covers `long
// long`, and `long`/`long long` are distinct types for overload resolution
// even though both are 64-bit here.
#define GM_PTR_SIZE_64 // gmStream.h gates its gmptr/gmuint64 SwapEndian overloads on this
typedef gmint64 gmptr;
typedef gmuint64 gmuptr;

#endif // _GMCONFIG_P_H_
