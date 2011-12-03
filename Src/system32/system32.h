#ifdef __cplusplus
extern "C" {
#endif

#include <crtdbg.h>

// #define _USE_32BIT_TIME_T to request VC6 compatibility in conjunction with
// the System32 build configuration. This attempts to avoid any references to
// the MSVCRT.DLL which VC6's version of the MSVCRT.DLL cannot satisfy.

#ifdef _USE_32BIT_TIME_T
#define _ctime32 ctime
#define _localtime32 localtime
#define _time32 time
#endif

#include <stdio.h>
#include <time.h>
#include <wchar.h>

#ifdef _USE_32BIT_TIME_T
void _assert(const char *, const char *, unsigned);
#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#define assert(e) (e ? (void)0 : _assert(#e, __FILE__, __LINE__))
#endif
FILE _iob[];
#undef stdin
#define stdin (&_iob[0])
#undef stdout
#define stdout (&_iob[1])
#undef stderr
#define stderr (&_iob[2])
#undef _fstati64
_CRTIMP int __cdecl _fstati64(int, struct _stat32i64 *);
#undef _wstati64
_CRTIMP int __cdecl _wstati64(const wchar_t *, struct _stat32i64 *);
#define _vscprintf(pFormat, arguments) (-1)
#define _vscwprintf(pFormat, arguments) (-1)
#endif

#ifdef __cplusplus
}
#endif
