#define STRICT
#define UNICODE
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX            1

#include <windows.h>
#include <timeapi.h>

#undef STRICT
#undef UNICODE
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#undef far
#undef near

typedef signed __int8   i8;
typedef signed __int16  i16;
typedef signed __int32  i32;
typedef signed __int64  i64;

typedef unsigned __int8   u8;
typedef unsigned __int16  u16;
typedef unsigned __int32  u32;
typedef unsigned __int64  u64;

typedef float  f32;
typedef double f64;

typedef i64 imm;
typedef u64 umm;

typedef u8 bool;

#define true (bool)(1)
#define false (bool)(0)

#define U8_MAX   (u8)  0xFF
#define U16_MAX  (u16) 0xFFFF
#define U32_MAX  (u32) 0xFFFFFFFF
#define U64_MAX  (u64) 0xFFFFFFFFFFFFFFFF

#define I8_MIN   (i8)  0x80
#define I16_MIN  (i16) 0x8000
#define I32_MIN  (i32) 0x80000000
#define I64_MIN  (i64) 0x8000000000000000

#define I8_MAX   (i8)  0x7F
#define I16_MAX  (i16) 0x7FFF
#define I32_MAX  (i32) 0x7FFFFFFF
#define I64_MAX  (i64) 0x7FFFFFFFFFFFFFFF

typedef struct String
{
    u8* data;
    u32 size;
} String;

#define STRING(str_lit) (String){ .data = (u8*)(str_lit), .size = sizeof(str_lit) - 1 }

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define STATIC_ASSERT(EX)                                                   \
struct CONCAT(STATIC_ASSERT_, CONCAT(__COUNTER__, CONCAT(_, __LINE__))) \
{                                                                       \
int static_assert_fails_on_negative_bit_width : (EX) ? 1 : -1;      \
}

#if TR_DEBUG
#define ASSERT(EX) ((EX) ? 1 : (*(volatile int*)0 = 0))
#define NOT_IMPLEMENTED ASSERT(!"NOT_IMPLEMENTED")
#define ILLEGAL_CODE_PATH ASSERT(!"ILLEGAL_CODE_PATH")
#else
#define ASSERT(EX)
#define NOT_IMPLEMENTED STATIC_ASSERT(!"NOT_IMPLEMENTED")
#define ILLEGAL_CODE_PATH
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define ROUND_UP(N, align) (((umm)(N) + ((umm)(align) - 1)) & ~((umm)(align) - 1))
#define ROUND_DOWN(N, align) ((umm)(N) & ~((umm)(align) - 1))
#define IS_POW_2(N) ((((umm)(N) - 1) & (umm)(N)) == 0 && (N) > 0)

#define OFFSETOF(s, e) ((unsigned int)&((s*)0)->e)
#define ALIGNOF(T) OFFSETOF(struct { char c; T t; }, t)

#define TYPEDEF_FUNC(ret, name, params) typedef ret name##params

#include <intrin.h>
#include <immintrin.h>

#include "tr_math.h"

u32* Backbuffer;
u32* Frontbuffer;
u32 BackbufferWidth;
u32 BackbufferHeight;

LRESULT
WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result;
    
	if (msg == WM_QUIT || msg == WM_CLOSE || msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		
        result = 0;
	}
    else if (msg == WM_PAINT)
    {
        PAINTSTRUCT p = {0};
        HDC dc = BeginPaint(window, &p);
        
        BITMAPINFO bmp_info = {
            .bmiHeader = {
                .biSize          = sizeof(BITMAPINFOHEADER),
                .biWidth         = BackbufferWidth,
                .biHeight        = BackbufferHeight,
                .biPlanes        = 1,
                .biBitCount      = 32,
                .biCompression   = BI_RGB,
                .biSizeImage     = BackbufferWidth*BackbufferHeight*4,
            },
        };
        
        StretchDIBits(dc, 0, BackbufferHeight, BackbufferWidth, -(i64)BackbufferHeight, 0, 0, BackbufferWidth, BackbufferHeight, Frontbuffer, &bmp_info, DIB_RGB_COLORS, SRCCOPY);
        
        EndPaint(window, &p);
        
        result = 0;
    }
	else result = DefWindowProcW(window, msg, wparam, lparam);
    
    return result;
}

void __stdcall
WinMainCRTStartup()
{
	HINSTANCE instance = GetModuleHandle(0);
    
    BackbufferWidth  = GetSystemMetrics(SM_CXSCREEN);
    BackbufferHeight = GetSystemMetrics(SM_CYSCREEN);
    u32* backbuffer_memory = VirtualAlloc(0, 2*ROUND_UP(BackbufferWidth*BackbufferHeight*sizeof(u32), 32), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Backbuffer  = backbuffer_memory;
    Frontbuffer = (u32*)((u8*)backbuffer_memory + ROUND_UP(BackbufferWidth*BackbufferHeight*sizeof(u32), 32));
    
    if (backbuffer_memory == 0) OutputDebugStringA("Failed to allocate memory\n");
    else
    {
        WNDCLASSEXW window_class_info = {
            .cbSize        = sizeof(WNDCLASSEXW),
            .style         = CS_OWNDC,
            .lpfnWndProc   = &WndProc,
            .hInstance     = instance,
            .hIcon         = LoadIcon(0, IDI_APPLICATION),
            .hCursor       = LoadCursor(0, IDC_ARROW),
            .lpszClassName = L"Tracer",
        };
        
        if (!RegisterClassExW(&window_class_info)) OutputDebugStringA("Failed to register window class\n");
        else
        {
            HWND window = CreateWindowW(window_class_info.lpszClassName, L"Tracer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, BackbufferWidth, BackbufferHeight, 0, 0, 0, 0);
            if (window == 0) OutputDebugStringA("Failed to create window\n");
            else
            {
                LARGE_INTEGER perf_counter_freq = {0};
                timeBeginPeriod(1);
                QueryPerformanceFrequency(&perf_counter_freq);
                
                LARGE_INTEGER flip_time;
                QueryPerformanceCounter(&flip_time);
                
                ShowWindow(window, SW_SHOW);
                
                bool is_running = true;
                while (is_running)
                {
                    MSG msg;
                    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
                    {
                        if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
                        {
                            is_running = false;
                            break;
                        }
                        else DispatchMessage(&msg);
                    }
                    
                    // render
                    
                    u32* tmp = Frontbuffer;
                    Frontbuffer = Backbuffer;
                    Backbuffer  = tmp;
                    InvalidateRect(window, 0, 0);
                    
                    LARGE_INTEGER end_time;
                    QueryPerformanceCounter(&end_time);
                    
                    f32 frame_time = (f32)(end_time.QuadPart - flip_time.QuadPart) / (f32)perf_counter_freq.QuadPart;
                    flip_time = end_time;
                    
                    WCHAR window_title_buffer[256];
                    wsprintfW(window_title_buffer, L"Tracer - %u ms, %u fps", (unsigned int)(frame_time*1000), (unsigned int)(1/frame_time));
                    SetWindowTextW(window, window_title_buffer);
                }
            }
        }
    }
    
    ExitProcess(0);
}