#include <limits.h>
#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <basetyps.h>
#include <vector>

#define LANG

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[1]))
#define MY_ARRAY_NEW(p, T, size) p = new T[size];
