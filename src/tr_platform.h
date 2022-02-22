#include <stdarg.h>

#ifndef TR_DEBUG
#define TR_DEBUG 0
#endif

#ifndef _WIN64
#error 32 bit mode is not supported yet
#endif

// NOTE: required to remove CRT
void* memset(void* ptr, int value, unsigned __int64 size);
void* memcpy(void* rdst, const void* rsrc, unsigned __int64 count);

#pragma function(memset)
#pragma function(memcpy)

void*
memset(void* ptr, int value, unsigned __int64 size)
{
    unsigned __int8* bptr = ptr;
    unsigned __int8 val   = (unsigned __int8)value;
    
    for (unsigned __int64 i = 0; i < size; ++i)
    {
        *bptr++ = val;
    }
    
    return ptr;
}

void*
memcpy(void* rdst, const void* rsrc, unsigned __int64 count)
{
    unsigned __int8* dst = (unsigned __int8*)rdst;
    const unsigned __int8* src = (const unsigned __int8*)rsrc;
    while (count--)
    {
        *dst++ = *src++;
    }
    return dst;
}


int _fltused;

int __stdcall
_DllMainCRTStartup(void* instance, unsigned __int32 reason, void* reserved)
{
    return 1;
}

typedef signed __int8  i8;
typedef signed __int16 i16;
typedef signed __int32 i32;
typedef signed __int64 i64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef u64 umm;
typedef i64 imm;

typedef u64 umm;
typedef i64 imm;

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

#define U8_MAX  0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#define I8_MIN  0xFF
#define I16_MIN 0xFFFF
#define I32_MIN 0xFFFFFFFF
#define I64_MIN 0xFFFFFFFFFFFFFFFF

#define I8_MAX  0x7F
#define I16_MAX 0x7FFF
#define I32_MAX 0x7FFFFFFF
#define I64_MAX 0x7FFFFFFFFFFFFFFF

typedef b8 bool;

#define false 0
#define true 1

typedef struct Buffer
{
    u8* data;
    u64 size;
} Buffer;

typedef Buffer String;
typedef String Path;

#define STRING(str) (String){ .data = (u8*)(str), .size = sizeof(str) - 1 }

#define Enum8(name)  u8
#define Enum16(name) u16
#define Enum32(name) u32
#define Enum64(name) u64

#define Flag8(name)  b8
#define Flag16(name) b16
#define Flag32(name) b32
#define Flag64(name) b64

#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)
#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

#if TR_DEBUG
#define ASSERT(EX) ((EX) ? 1 : (*(volatile int*)0 = 0))
#else
#define ASSERT(EX)
#endif


#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define INVALID_DEFAULT_CASE default: ASSERT(!"INVALID_DEFAULT_CASE"); break
#define INVALID_CODE_PATH ASSERT(!"INVALID_CODE_PATH")

#define OFFSETOF(element, type) (umm)&((type*)0)->element
#define ALIGNOF(T) OFFSETOF(t, struct { u8 b; T t; })

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(n) ((n) < 0 ? -(n) : (n))
#define SGN(n) ((n) < 0 ? -1 : ((n) == 0 ? 0 : 1))

#define BYTE_MASK(n) (~(u64)0 >> (8*(n)))

#define KB(N) ((umm)(N) << 10)
#define MB(N) ((umm)(N) << 20)
#define GB(N) ((umm)(N) << 30)
#define TB(N) ((umm)(N) << 40)

#define MS(N) ((f32)N / 1000)

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define internal static
#define global static

// NOTE: This is just a hack to work around a parsing bug in 4coder
#define TYPEDEF_FUNC(return_val, name, ...) typedef return_val (*name)(__VA_ARGS__)

struct Memory_Arena;

TYPEDEF_FUNC(void*, platform_reserve_memory, umm size);
TYPEDEF_FUNC(bool, platform_commit_memory, void* base_pointer, umm size);
TYPEDEF_FUNC(void, platform_print, char* format, ...);
TYPEDEF_FUNC(void, platform_swap_buffers);
TYPEDEF_FUNC(bool, platform_read_entire_file, struct Memory_Arena* arena, String path, String* file_contents);

typedef struct Platform_Data
{
    struct Memory_Arena* persistent_arena;
    struct Memory_Arena* transient_arena;
    
    platform_reserve_memory ReserveMemory;
    platform_commit_memory CommitMemory;
    platform_print Print;
    platform_swap_buffers SwapBuffers;
    platform_read_entire_file ReadEntireFile;
    
    u32* image;
    u32 width;
    u32 height;
} Platform_Data;

Platform_Data* Platform;

#include "tr_math.h"
#include "tr_memory.h"
#include "tr_targa.h"

typedef struct Platform_Input
{
    V2 dir;
    f32 dt;
} Platform_Input;

TYPEDEF_FUNC(void, platform_tick, Platform_Data* platform_data, Platform_Input input);