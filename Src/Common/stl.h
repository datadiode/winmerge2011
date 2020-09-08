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

// Neglect C++11 and use EASTL's deprecated scoped_ptr/scoped_array templates
#include <eastl/scoped_ptr.h>
#include <eastl/scoped_array.h>
using eastl::scoped_ptr;
using eastl::scoped_array;

// std::begin() and std::end() as available in C++11.
namespace eastl
{
	template <typename T, size_t N> T *begin(T (&r)[N]) { return r; }
	template <typename T, size_t N> T *end(T (&r)[N]) { return r + _countof(r); }
}
#else
#define stl(x) <x>
#define stl_size_t size_t
#endif
