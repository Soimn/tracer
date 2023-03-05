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
u32 BufferWidth;
u32 BufferHeight;

u32 StarmapWidth  = 512;
u32 StarmapHeight = 256;
u32* Starmap;

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
        
        ASSERT(BufferWidth  < I32_MAX);
        ASSERT(BufferHeight < I32_MAX);
        ASSERT(BufferWidth*BufferHeight < U32_MAX);
        BITMAPINFO bmp_info = {
            .bmiHeader = {
                .biSize          = sizeof(BITMAPINFOHEADER),
                .biWidth         = (LONG)BufferWidth,
                .biHeight        = -(LONG)BufferHeight,
                .biPlanes        = 1,
                .biBitCount      = 32,
                .biCompression   = BI_RGB,
            },
            .bmiColors = {0},
        };
        
        SetDIBitsToDevice(dc, 0, 0, BufferWidth, BufferHeight, 0, 0, 0, BufferHeight, Frontbuffer, &bmp_info, DIB_RGB_COLORS);
        
        EndPaint(window, &p);
        
        result = 0;
    }
	else result = DefWindowProcW(window, msg, wparam, lparam);
    
    return result;
}

void
Render()
{
    // Clear screen
    for (umm i = 0; i < BufferWidth*BufferHeight; ++i) Backbuffer[i] = 0;
    
    V3 sphere_pos = V3(0, 0, 4);
    f32 sphere_r  = 1;
    
    V3x8 camera_pos = V3x8_Set1V3(V3(0, 0, 0));
    
    V3 to_light = V3(0, 1, 0);
    
    V3x8 l     = V3x8_Set1V3(to_light);
    V3x8 p     = V3x8_Set1V3(sphere_pos);
    V3x8 neg_p = V3x8_Sub(p, camera_pos);
    
    __m256 one     = _mm256_set1_ps(1);
    __m256 zero    = _mm256_setzero_ps();
    __m256 n255    = _mm256_set1_ps(255);
    __m256 r       = _mm256_set1_ps(sphere_r);
    __m256 c       = _mm256_fmsub_ps(r, r, V3x8_Inner(neg_p, neg_p));
    __m256 ambient = _mm256_set1_ps(0.15f);
    
    __m256 neg_2x_rbufferwidth             = _mm256_set1_ps(-2.0f/BufferWidth);
    __m256 inv_aspect                      = _mm256_set1_ps((f32)BufferHeight/BufferWidth);
    __m256 counting_neg_2x_rbufferwidth_p1 = _mm256_fmadd_ps(_mm256_load_ps((float[8]){0,1,2,3,4,5,6,7}), neg_2x_rbufferwidth, one);
    
    
    __m256 starmap_height = _mm256_set1_ps((f32)StarmapHeight);
    __m256 starmap_width  = _mm256_set1_ps((f32)StarmapWidth);
    
    __m256i rgb_mask = _mm256_set1_epi32(0xFFFFFF);
    
    for (umm sy = 0; sy < BufferHeight; ++sy)
    {
        __m256 y = _mm256_fmadd_ps(_mm256_set1_ps((f32)sy), neg_2x_rbufferwidth, inv_aspect);
        
        for (umm sx = 0; sx < BufferWidth; sx += 8)
        {
            __m256 x = _mm256_fmadd_ps(_mm256_set1_ps((f32)sx), neg_2x_rbufferwidth, counting_neg_2x_rbufferwidth_p1);
            
            V3x8 v = V3x8_Normalize(V3x8(x, y, one));
            
            __m256 b = V3x8_Inner(neg_p, v);
            __m256 discriminant = _mm256_fmadd_ps(b, b, c);
            __m256 t            = _mm256_sub_ps(b, _mm256_sqrt_ps(discriminant));
            __m256i mask        = _mm256_castps_si256(_mm256_and_ps(_mm256_cmp_ps(discriminant, zero, _CMP_GE_OQ), _mm256_cmp_ps(t, zero, _CMP_GE_OQ)));
            
            V3x8 hit = V3x8_Scale(v, t);
            V3x8 n   = V3x8_Normalize(V3x8_Sub(hit, p));
            __m256 cos_theta = V3x8_Inner(n, l);
            
            __m256 intensity        = _mm256_min_ps(_mm256_add_ps(_mm256_max_ps(cos_theta, zero), ambient), one);
            __m256i intensity_256   = _mm256_cvtps_epi32(_mm256_mul_ps(intensity, n255));
            __m256i intensity_color = _mm256_or_si256(_mm256_or_si256(_mm256_slli_epi32(intensity_256, 16), _mm256_slli_epi32(intensity_256, 8)), intensity_256);
            
            __m256 starmap_lon = _mm256_atan2_ps(v.x, v.z);
            __m256 starmap_lat = _mm256_atan2_ps(v.y, _mm256_sqrt_ps(_mm256_add_ps(_mm256_mul_ps(v.x, v.x), _mm256_mul_ps(v.z, v.z))));
            
            __m256 starmap_u = _mm256_div_ps(_mm256_add_ps(starmap_lon, _mm256_set1_ps(PI32)), _mm256_set1_ps(TAU32));
            __m256 starmap_v = _mm256_div_ps(_mm256_add_ps(starmap_lat, _mm256_set1_ps(PI32/2)), _mm256_set1_ps(PI32));
            
            __m256i starmap_index = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(starmap_v, starmap_height), starmap_u), starmap_width));
            
            __m256i starmap_color = _mm256_and_si256(_mm256_i32gather_epi32((int*)Starmap, starmap_index, 4), rgb_mask);
            
            __m256i color = _mm256_blendv_epi8(starmap_color, intensity_color, mask);
            
            _mm256_store_si256((__m256i*)(Backbuffer + sy*BufferWidth + sx), starmap_color);
        }
    }
}

void __stdcall
WinMainCRTStartup()
{
    HINSTANCE instance = GetModuleHandle(0);
    
    Starmap = VirtualAlloc(0, StarmapWidth*StarmapHeight*4, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    
    HANDLE starmap_file_handle = CreateFileA("res/starmap.tga", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    
    OVERLAPPED overlapped = {
        .Offset     = 18,
        .OffsetHigh = 0,
    };
    ReadFile(starmap_file_handle, Starmap, StarmapWidth*StarmapHeight*4, 0, &overlapped);
    CloseHandle(starmap_file_handle);
    
    // TODO: Ensure BufferWidth % 32 == 0
    BufferWidth  = GetSystemMetrics(SM_CXSCREEN);
    BufferHeight = GetSystemMetrics(SM_CYSCREEN);
    u32* backbuffer_memory = VirtualAlloc(0, 2*ROUND_UP(BufferWidth*BufferHeight*sizeof(u32), 32), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Backbuffer  = backbuffer_memory;
    Frontbuffer = (u32*)((u8*)backbuffer_memory + ROUND_UP(BufferWidth*BufferHeight*sizeof(u32), 32));
    
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
            HWND window = CreateWindowW(window_class_info.lpszClassName, L"Tracer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, BufferWidth, BufferHeight, 0, 0, 0, 0);
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
                    
                    Render();
                    
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