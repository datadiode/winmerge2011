#ifdef USE_EASTL
// This is the right place to override settings from EASTL/internal/config.h
#define EASTL_STD_ITERATOR_CATEGORY_ENABLED 0
#define EASTL_EXCEPTIONS_ENABLED 0
namespace eastl { }
namespace stl = eastl;
#define stl(x) <eastl/x.h>
// EASTL as currently used with WinMerge 2011 lacks an auto_ptr template.
// Resort to the implementation published in 1999 by Nicolai M. Josuttis.
#include "auto_ptr.h"
#else
namespace std { }
namespace stl = std;
#define stl(x) <x>
#endif
