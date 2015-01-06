#ifdef USE_EASTL
// This is the right place to override settings from EASTL/internal/config.h
#define EASTL_STD_ITERATOR_CATEGORY_ENABLED 0
#define EASTL_EXCEPTIONS_ENABLED 0
// Prevent #define WCHAR_MAX ((wchar_t)-1) from fooling EASTL
#define EA_WCHAR_SIZE 2
namespace eastl { }
namespace std { using namespace eastl; }
#define stl(x) <eastl/x.h>
#define stl_size_t eastl_size_t
// EASTL as currently used with WinMerge 2011 lacks an auto_ptr template.
// Resort to the implementation published in 1999 by Nicolai M. Josuttis.
#include "auto_ptr.h"
// Another lacking of EASTL is the rotate() algorithm.
#include "rotate.h"
#else
#define stl(x) <x>
#define stl_size_t size_t
#endif
