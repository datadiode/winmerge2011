/*/system32.cpp
 * Copyright (C) 2011 Jochen Neubeck.
 * Added a few things to support WinMerge 2011 System32 build configuration.
 */

/*
    This is a C++ run-time library for Windows kernel-mode drivers.
    Copyright (C) 2004 Bo Brantén.
*/

/*
    The following is a modified subset of wine/dlls/msvcrt/cppexcept.c
    and wine/dlls/msvcrt/cppexcept.h from version wine20040505.
*/

/*
 * msvcrt C++ exception handling
 *
 * Copyright 2002 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "StdAfx.h"
#include <HtmlHelp.h>
#include <typeinfo.h>

// Help linker construct the type_info vtable.
type_info::~type_info() { }

#ifdef _USE_32BIT_TIME_T
// Help out linker with otherwise unresolved vector new & delete.
void *operator new[](size_t size) { return operator new(size); }
void operator delete[](void *p) { operator delete(p); }
#endif

// Exception handling related code

#pragma warning(disable:4733) // Does not apply because of /SAFESEH:NO

#define CXX_FRAME_MAGIC_VC8 0x19930522
#define CXX_EXCEPTION       0xe06d7363
#define CXX_FRAME_MAGIC		CXX_FRAME_MAGIC_VC8

extern "C" namespace {

void __cdecl _global_unwind2(void *);

typedef void (__cdecl *_se_translator_function)(unsigned int, struct _EXCEPTION_POINTERS*);

_se_translator_function _se_translator = NULL;

_se_translator_function __cdecl _set_se_translator(_se_translator_function _new_se_translator)
{
    _se_translator_function _prev_se_translator = _se_translator;
    _se_translator = _new_se_translator;
    return _prev_se_translator;
}

/*
 * The exception frame, used for registering exception handlers
 * Win32 cares only about this, but compilers generally emit
 * larger exception frames for their own use.
 */

struct __EXCEPTION_FRAME;

typedef EXCEPTION_DISPOSITION (__stdcall *PEXCEPTION_HANDLER)(PEXCEPTION_RECORD,struct __EXCEPTION_FRAME*,
                               PCONTEXT,struct __EXCEPTION_FRAME **);

typedef struct __EXCEPTION_FRAME
{
    struct __EXCEPTION_FRAME *Prev;
    PEXCEPTION_HANDLER       Handler;
} EXCEPTION_FRAME, *PEXCEPTION_FRAME;

/*
 * From OS/2 2.0 exception handling
 * Win32 seems to use the same flags as ExceptionFlags in an EXCEPTION_RECORD
 */

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10

typedef void (*vtable_ptr)();

/* the object the typeid operator returns a reference to */
typedef struct _type_info
{
    vtable_ptr *vtable;  /* pointer to the objects vtable */
    void       *data;    /* unused in drivers, dynamically allocated undecorated type name in Win32 */
    char        name[1]; /* decorated (raw) type name */
} type_info;

/* the extended exception frame used by C++ */
typedef struct _cxx_exception_frame
{
    EXCEPTION_FRAME frame;    /* the standard exception frame */
    int             trylevel; /* current trylevel */
    ULONG           ebp;      /* saved ebp */
} cxx_exception_frame;

/* info about a single catch {} block */
typedef struct _catchblock_info
{
    unsigned int flags;      /* flags (see TYPE_FLAG_* flags below) */
    type_info   *type_info;  /* C++ type caught by this block */
    int          offset;     /* stack offset to copy exception object to */
    void       (*handler)(); /* catch block handler code */
} catchblock_info;

#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

/* info about a single try {} block */
typedef struct _tryblock_info
{
    int              start_level;      /* start trylevel of that block */
    int              end_level;        /* end trylevel of that block */
    int              catch_level;      /* initial trylevel of the catch block */
    int              catchblock_count; /* count of catch blocks in array */
    catchblock_info *catchblock;       /* array of catch blocks */
} tryblock_info;

/* info about the unwind handler for a given trylevel */
typedef struct _unwind_info
{
    int    prev;       /* prev trylevel unwind handler, to run after this one */
    void (*handler)(); /* unwind handler */
} unwind_info;

/* descriptor of all try blocks of a given function */
typedef struct _cxx_function_descr
{
    unsigned int   magic;          /* must be CXX_FRAME_MAGIC */
    int            unwind_count;   /* number of unwind handlers */
    unwind_info   *unwind_table;   /* array of unwind handlers */
    unsigned int   tryblock_count; /* number of try blocks */
    tryblock_info *tryblock;       /* array of try blocks */
    unsigned int   unknown[3];
} cxx_function_descr;

typedef void (*cxx_copy_ctor)(void);

/* complete information about a C++ type */
typedef struct _cxx_type_info
{
    unsigned int  flags;            /* flags (see CLASS_* flags below) */
    type_info    *type_info;        /* C++ type info */
    int           this_offset;      /* offset of base class this pointer from start of object */
    int           vbase_descr;      /* offset of virtual base class descriptor */
    int           vbase_offset;     /* offset of this pointer offset in virtual base class descriptor */
    size_t        size;             /* object size */
    cxx_copy_ctor copy_constructor; /* copy constructor */
} cxx_type_info;

#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

/* table of C++ types that apply for a given object */
typedef struct _cxx_type_info_table
{
    unsigned int         count;            /* number of types */
    const cxx_type_info *cxx_type_info[1]; /* array of types */
} cxx_type_info_table;

/* type information for an exception object */
typedef struct _cxx_exception_type
{
    unsigned int               flags;           /* TYPE_FLAG_* flags */
    void                     (*destructor)();   /* exception object destructor */
    void*                      custom_handler;  /* custom handler for this exception */
    const cxx_type_info_table *type_info_table; /* list of types for this exception object */
} cxx_exception_type;

/* installs a new exception handler */
#define cxx_push_handler(handler) {         \
    __asm push offset handler               \
    __asm mov eax,fs:[0]                    \
    __asm push eax                          \
    __asm mov fs:[0],esp                    \
}

/* restores the previous exception handler */
#define cxx_pop_handler() {                 \
    __asm mov eax,[esp]                     \
    __asm mov fs:[0],eax                    \
    __asm add esp,8                         \
}

/* exception handler for unexpected exceptions thrown while handling an exception */
static EXCEPTION_DISPOSITION __stdcall unexpected_handler(PEXCEPTION_RECORD rec, PEXCEPTION_FRAME frame,
                                                          PCONTEXT context, void *dispatch)
{
    //DbgPrint("unexpected exception\n");
    return ExceptionContinueSearch;
}

/* frame pointer register 'ebp' modified by inline assembly code */
#pragma warning(disable:4731)

/* call a function with a given ebp */
__declspec(naked) static void *__stdcall cxx_call_handler(void *func, void *func_ebp)
{
    __asm
    {
		push ebp
		mov ebp,esp
        mov eax,func
        mov ebp,func_ebp
        call eax
        pop ebp
		ret 8
    }
}

#pragma warning(default:4731)

/* call a copy constructor */
__declspec(naked) static void __stdcall cxx_call_copy_constructor(void *func, void *object, void *src_object)
{
    __asm
    {
		push ebp
		mov ebp,esp
        mov ecx,object
        push 1 // just in case has_vbase
        push src_object
        call func
		mov esp,ebp // just in opposite case
		pop ebp
		ret 12
    }
}

/* call the destructor of an object */
static void cxx_call_destructor(void *func, void *object)
{
    __asm
    {
        mov ecx,object
        call func
    }
}

/* compute the this pointer for a base class of a given type */
static void *get_this_pointer(const cxx_type_info *type, void *object)
{
    void *this_ptr;
    int *offset_ptr;

    if (!type || !object) return NULL;
    this_ptr = (char *)object + type->this_offset;
    if (type->vbase_descr >= 0)
    {
        /* move this ptr to vbase descriptor */
        this_ptr = (char *)this_ptr + type->vbase_descr;
        /* and fetch additional offset from vbase descriptor */
        offset_ptr = (int *)(*(char **)this_ptr + type->vbase_offset);
        this_ptr = (char *)this_ptr + *offset_ptr;
    }
    return this_ptr;
}

/* check if the exception type is caught by a given catch block, and
   return the type that matched */
static const cxx_type_info *find_caught_type(cxx_exception_type *exception_type,
                                             catchblock_info *catchblock)
{
    unsigned int i;

    if (!catchblock->type_info)
    {
        return exception_type->type_info_table->cxx_type_info[0]; /* catch(...) matches any type */
    }

    for (i = 0; i < exception_type->type_info_table->count; i++)
    {
        const cxx_type_info *type = exception_type->type_info_table->cxx_type_info[i];

        if (catchblock->type_info != type->type_info)
        {
            if (strcmp(catchblock->type_info->name, type->type_info->name)) continue;
        }
        /* type is the same, now check the flags */
        if ((exception_type->flags & TYPE_FLAG_CONST) &&
            !(catchblock->flags & TYPE_FLAG_CONST)) continue;
        if ((exception_type->flags & TYPE_FLAG_VOLATILE) &&
            !(catchblock->flags & TYPE_FLAG_VOLATILE)) continue;
        return type;  /* it matched */
    }

    return NULL;
}

/* copy the exception object where the catch block wants it */
static void copy_exception(void *object, cxx_exception_frame *frame,
	catchblock_info *catchblock, const cxx_type_info *type)
{
    void **dest_ptr = (void **)((char *)&frame->ebp + catchblock->offset);
	if (catchblock->flags & TYPE_FLAG_REFERENCE)
	{
		*dest_ptr = get_this_pointer(type, object);
	}
	else if (type->flags & CLASS_IS_SIMPLE_TYPE)
	{
		memcpy(dest_ptr, object, type->size);
		/* if it is a pointer, adjust it */
		if (type->size == sizeof(void *))
			*dest_ptr = get_this_pointer(type, *dest_ptr);
	}
	else if (type->copy_constructor)
	{
		cxx_push_handler(unexpected_handler);
		cxx_call_copy_constructor(type->copy_constructor, dest_ptr, get_this_pointer(type, object));
		cxx_pop_handler();
	}
	else
	{
		memcpy(dest_ptr, get_this_pointer(type, object), type->size);
	}
}

/* unwind the local function up to a given trylevel */
static void cxx_local_unwind(cxx_exception_frame *frame, cxx_function_descr *descr,
                             int last_level)
{
    while (frame->trylevel != last_level)
    {
        if (frame->trylevel < 0 || frame->trylevel >= descr->unwind_count)
        {
            abort();
        }
        if (descr->unwind_table[frame->trylevel].handler)
        {
            cxx_push_handler(unexpected_handler);
            cxx_call_handler(descr->unwind_table[frame->trylevel].handler, &frame->ebp);
            cxx_pop_handler();
        }
        frame->trylevel = descr->unwind_table[frame->trylevel].prev;
    }
}

/* exception handler for exceptions thrown from a catch block */
static EXCEPTION_DISPOSITION __stdcall catch_block_protector(PEXCEPTION_RECORD rec, PEXCEPTION_FRAME frame,
                                                             PCONTEXT context, void *dispatch)
{
    if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
    {
        /* get the previous exception saved on the stack */
        void* exception_object = (void*) ((ULONG*)frame)[2];
        cxx_exception_type* exception_type = (cxx_exception_type*) ((ULONG*)frame)[3];

        if (rec->ExceptionCode == CXX_EXCEPTION &&
            rec->NumberParameters == 3 &&
            rec->ExceptionInformation[0] == CXX_FRAME_MAGIC &&
            rec->ExceptionInformation[1] == 0 &&
            rec->ExceptionInformation[2] == 0)
        {
            /* rethrow the previous exception */
            rec->ExceptionInformation[1] = (ULONG) exception_object;
            rec->ExceptionInformation[2] = (ULONG) exception_type;
        }
        else
        {
            /* throw of new exception, delete the previous exception object */
            if (exception_object && exception_type->destructor)
            {
                cxx_push_handler(unexpected_handler);
                cxx_call_destructor(exception_type->destructor, exception_object);
                cxx_pop_handler();
            }
        }
    }
    return ExceptionContinueSearch;
}

/* find and call the appropriate catch block for an exception */
/* returns the address to continue execution to after the catch block was called */
static void *call_catch_block(
	PEXCEPTION_RECORD rec, cxx_exception_frame *frame, cxx_function_descr *descr)
{
    unsigned int i;
    int j;
    void *ret_addr;
    void *exception_object = (void *) rec->ExceptionInformation[1];
    cxx_exception_type *exception_type = (cxx_exception_type *) rec->ExceptionInformation[2];
    int trylevel = frame->trylevel;

    for (i = 0; i < descr->tryblock_count; i++)
    {
        tryblock_info *tryblock = &descr->tryblock[i];
        if (trylevel < tryblock->start_level || trylevel > tryblock->end_level)
			continue;

        /* got a try block */
        for (j = 0; j < tryblock->catchblock_count; j++)
        {
            catchblock_info *catchblock = &tryblock->catchblock[j];
			const cxx_type_info *type = find_caught_type(exception_type, catchblock);
            if (!type)
				continue;

            /* copy the exception to its destination on the stack */
			if (catchblock->type_info &&
				catchblock->type_info->name[0] &&
				catchblock->offset)
			{
				copy_exception(exception_object, frame, catchblock, type);
			}

            /* unwind the stack */
            _global_unwind2(frame);
            cxx_local_unwind(frame, descr, tryblock->start_level);

            frame->trylevel = tryblock->end_level + 1;

            /* call the catch block */
            __asm push exception_type
            __asm push exception_object
            cxx_push_handler(catch_block_protector);
            ret_addr = cxx_call_handler(catchblock->handler, &frame->ebp);
            cxx_pop_handler();
            __asm add esp, 8

            /* delete the exception object */
            if (exception_object && exception_type->destructor)
            {
                cxx_push_handler(unexpected_handler);
                cxx_call_destructor(exception_type->destructor, exception_object);
                cxx_pop_handler();
            }
            return ret_addr;
        }
    }
    return NULL;
}

static EXCEPTION_DISPOSITION __stdcall cxx_frame_handler(
	cxx_function_descr *descr,
	PEXCEPTION_RECORD rec,
	cxx_exception_frame *frame,
	PCONTEXT context,
	void *dispatch)
{
	if (descr->magic != CXX_FRAME_MAGIC)
	{
		return ExceptionContinueSearch;
	}

    /* stack unwinding */
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        if(frame->trylevel >= descr->unwind_count) //stack corruption
        {
            abort();
        }
        if (descr->unwind_count)
        {
            cxx_local_unwind(frame, descr, -1);
        }
    }
    /* non C++ exception */
    else if (rec->ExceptionCode != CXX_EXCEPTION)
    {
        if (_se_translator)
        {
            EXCEPTION_POINTERS ep = {rec, context};
            (*_se_translator)(rec->ExceptionCode, &ep);
        }
    }
    /* C++ exception */
    else if (rec->NumberParameters == 3 &&
             rec->ExceptionInformation[0] == CXX_FRAME_MAGIC &&
             rec->ExceptionInformation[1] != 0 &&
             rec->ExceptionInformation[2] != 0)
    {
        void *ret_addr = call_catch_block(rec, frame, descr);

        if (ret_addr)
        {
            rec->ExceptionFlags &= ~EH_NONCONTINUABLE;
            context->Eip = (ULONG)ret_addr;
            context->Ebp = (ULONG)&frame->ebp;
            context->Esp = ((ULONG*)frame)[-1];
            return ExceptionContinueExecution;
        }
    }
    return ExceptionContinueSearch;
}

__declspec(naked) EXCEPTION_DISPOSITION __cdecl __CxxFrameHandler3(
	PEXCEPTION_RECORD rec, PEXCEPTION_FRAME frame, PCONTEXT context, void *dispatch)
{
	__asm
	{
		cld
		xchg [esp],eax
		push eax
		jmp cxx_frame_handler
	}
}

void __stdcall _CxxThrowException(ULONG_PTR pObj, ULONG_PTR pInfo)
{
	ULONG_PTR throwParams[] = { CXX_FRAME_MAGIC_VC8, pObj, pInfo };
	RaiseException(0xE06D7363, 1, 3, throwParams);
}

__declspec(naked) void _alloca_probe_16()
{
	extern void _alloca_probe();
	__asm
	{
		neg eax
		lea eax,[eax+esp+4]
		and eax,~15
		neg eax
		lea eax,[eax+esp+4]
		jmp _alloca_probe
	}
}

__declspec(naked) void _ftol2()
{
	extern void _ftol();
	__asm jmp _ftol
}

__declspec(naked) void _ftol2_sse()
{
	extern void _ftol();
	__asm jmp _ftol
}

int _fltused = 1;

typedef void (__cdecl *_PVFV)(void);

#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCZ",long,read)

__declspec(allocate(".CRT$XCA")) _PVFV __xc_a[] = { NULL };
__declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[] = { NULL };

void __cdecl _initterm(_PVFV *, _PVFV *);

void wWinMainCRTStartup()
{
	int nRet;
	STARTUPINFO si;
	si.dwFlags = 0;
	GetStartupInfo(&si);
	_initterm(__xc_a, __xc_z);
	nRet = _tWinMain(GetModuleHandle(NULL), NULL, PathGetArgs(GetCommandLine()),
		si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
	exit(nRet);
}

} // extern "C" namespace
