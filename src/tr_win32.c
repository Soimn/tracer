#define UNICODE
#define NOMINMAX            1
#define WIN32_LEAN_AND_MEAN 1
#define WIN32_MEAN_AND_LEAN 1
#define VC_EXTRALEAN        1
#include <windows.h>
#include <Timeapi.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef WIN32_MEAN_AND_LEAN
#undef VC_EXTRALEAN
#undef far
#undef near

#include "tr_platform.h"

global Memory_Arena Win32_Arena  = {0};
global u8* Backbuffer            = 0;

typedef struct Win32_File_Timestamp
{
    u64 creation;
    u64 last_write;
    u64 last_access;
} Win32_File_Timestamp;

typedef struct Win32_Code
{
    Win32_File_Timestamp timestamp;
    HMODULE handle;
    platform_tick tick;
} Win32_Code;

internal umm
String_FormatArgList(Buffer out, const char* format, va_list args)
{
    umm required_bytes = 0;
    
    for (char* scan = (char*)format; *scan; )
    {
        if (*scan == '%')
        {
            ++scan;
            
            switch (*scan)
            {
                case '%':
                {
                    if (out.size > required_bytes) out.data[required_bytes] = *scan;
                    ++required_bytes;
                    ++scan;
                } break;
                
                case 'f':
                {
                    f64 val = va_arg(args, f64);
                    
                    ++scan;
                    
                    if (val < 0)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = '-';
                        required_bytes += 1;
                        
                        val = -val;
                    }
                    
                    // HACK
                    u64 upper = (u64)(val / ((u64)1 << 63));
                    
                    umm largest_place = 0;
                    u64 val_copy = upper;
                    while (val_copy /= 10) largest_place *= 10;
                    
                    while (largest_place != 0)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = (upper / largest_place) % 10 + '0';
                        largest_place  /= 10;
                        required_bytes += 1;
                    }
                    u64 lower = (u64)(val - (f32)upper * ((u64)1 << 63));
                    
                    largest_place = 1;
                    val_copy = lower;
                    while (val_copy /= 10) largest_place *= 10;
                    
                    while (largest_place != 0)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = (lower / largest_place) % 10 + '0';
                        largest_place  /= 10;
                        required_bytes += 1;
                    }
                    
                    if (required_bytes < out.size) out.data[required_bytes] = '.';
                    required_bytes += 1;
                    
                    F64_Bits bits = { .flt = val };
                    
                    i64 exponent = ((bits.bits & 0x7FF0000000000000) >> 52) - 1023;
                    
                    bits.bits &= ~(0x000FFFFFFFFFFFFF >> MIN(MAX(exponent, 0), 52));
                    
                    f64 fract_val = (val - bits.flt) * 10;
                    
                    if (required_bytes < out.size) out.data[required_bytes] = (umm)fract_val % 10 + '0';
                    required_bytes += 1;
                    fract_val = (fract_val - (umm)fract_val)*10;
                    
                    for (umm i = 0; fract_val != 0 && i < 4; ++i)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = (umm)fract_val % 10 + '0';
                        required_bytes += 1;
                        fract_val = (fract_val - (umm)fract_val)*10;
                    }
                    
                } break;
                
                case 'u':
                case 'U':
                case 'i':
                case 'I':
                {
                    u64 val;
                    if      (*scan == 'u') val = va_arg(args, u32);
                    else if (*scan == 'U') val = va_arg(args, u64);
                    else
                    {
                        i64 i_val;
                        if      (*scan == 'i') i_val = va_arg(args, i32);
                        else                   i_val = va_arg(args, i64);
                        
                        if (i_val < 0)
                        {
                            if (required_bytes < out.size) out.data[required_bytes] = '-';
                            required_bytes += 1;
                            
                            i_val  = -i_val;
                        }
                        
                        val = (u64)i_val;
                    }
                    
                    ++scan;
                    
                    umm largest_place = 1;
                    u64 val_copy = val;
                    while (val_copy /= 10) largest_place *= 10;
                    
                    while (largest_place != 0)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = (val / largest_place) % 10 + '0';
                        largest_place  /= 10;
                        required_bytes += 1;
                    }
                } break;
                
                case 'x':
                case 'X':
                {
                    u64 val;
                    if      (*scan == 'x') val = va_arg(args, u32);
                    else                   val = va_arg(args, u64);
                    
                    umm largest_place = 1;
                    u64 val_copy = val;
                    while (val_copy /= 16) largest_place *= 16;
                    
                    while (largest_place != 0)
                    {
                        u8 digit = (val / largest_place) % 16;
                        
                        if (required_bytes < out.size) out.data[required_bytes] = (digit <= '9' ? digit + '0' : digit + 'A');
                        largest_place  /= 16;
                        required_bytes += 1;
                    }
                } break;
                
                case 'b':
                {
                    ++scan;
                    
                    bool val = va_arg(args, bool);
                    
                    char* str;
                    if (val) str = "true";
                    else     str = "false";
                    
                    for (; *str; ++str)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = *str;
                        required_bytes += 1;
                    }
                } break;
                
                case 'c':
                {
                    ++scan;
                    
                    char c = va_arg(args, char);
                    
                    if (required_bytes < out.size) out.data[required_bytes] = c;
                    required_bytes += 1;
                    
                } break;
                
                case 's':
                {
                    ++scan;
                    
                    char* str = va_arg(args, char*);
                    
                    for (; *str; ++str)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = *str;
                        required_bytes += 1;
                    }
                } break;
                
                case 'S':
                {
                    ++scan;
                    
                    String string = va_arg(args, String);
                    
                    for (umm i = 0; i < string.size; ++i)
                    {
                        if (required_bytes < out.size) out.data[required_bytes] = string.data[i];
                        required_bytes += 1;
                    }
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        }
        
        else
        {
            if (out.size > required_bytes) out.data[required_bytes] = *scan;
            ++required_bytes;
            ++scan;
        }
    }
    
    if (out.size > required_bytes) out.data[required_bytes] = 0;
    ++required_bytes;
    
    return required_bytes;
}

internal void
Win32_Print(char* format, ...)
{
    va_list args;
    
    Memory_Arena_Marker marker = Arena_BeginTempMemory(&Win32_Arena);
    
    Buffer buffer = {
        .data = Arena_PushSize(&Win32_Arena, MB(1), ALIGNOF(u8)),
        .size = MB(1),
    };
    
    va_start(args, format);
    umm req_buffer_size = String_FormatArgList(buffer, format, args);
    va_end(args);
    
    if (req_buffer_size > buffer.size)
    {
        Arena_EndTempMemory(&Win32_Arena, marker);
        marker = Arena_BeginTempMemory(&Win32_Arena);
        
        buffer = (Buffer){
            .data = Arena_PushSize(&Win32_Arena, req_buffer_size, ALIGNOF(u8)),
            .size = req_buffer_size,
        };
        
        va_start(args, format);
        req_buffer_size = String_FormatArgList(buffer, format, args);
        va_end(args);
        
        ASSERT(req_buffer_size == buffer.size);
    }
    
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer.data, (DWORD)req_buffer_size - 1, 0, 0);
    
    Arena_EndTempMemory(&Win32_Arena, marker);
}

internal void*
Win32_ReserveMemory(umm size)
{
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

internal bool
Win32_CommitMemory(void* base_pointer, umm size)
{
    return !!VirtualAlloc(base_pointer, size, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32_SwapBuffers()
{
    void* tmp = Platform->image;
    Platform->image = (u32*)Backbuffer;
    Backbuffer      = tmp;
}

internal bool
Win32_ReadEntireFile(Memory_Arena* arena, String path, String* file_contents)
{
    bool succeeded = false;
    
    Memory_Arena_Marker marker = Arena_BeginTempMemory(Platform->transient_arena);
    
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    
    int required_chars = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)path.data, (int)path.size, 0, 0);
    
    if (required_chars != 0)
    {
        WCHAR* wide_path = Arena_PushSize(Platform->transient_arena, (required_chars + 1) * sizeof(WCHAR), ALIGNOF(WCHAR));
        
        if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)path.data, (int)path.size, wide_path, required_chars) == required_chars)
        {
            wide_path[required_chars] = 0;
            
            file_handle = CreateFileW(wide_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        }
    }
    
    
    Arena_EndTempMemory(Platform->transient_arena, marker);
    
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        u32 file_size = GetFileSize(file_handle, 0);
        
        file_contents->size = file_size;
        file_contents->data = Arena_PushSize(arena, file_size + 1, ALIGNOF(WCHAR));
        
        DWORD bytes_read;
        if (ReadFile(file_handle, file_contents->data, (u32)file_size, &bytes_read, 0) && bytes_read == file_size)
        {
            file_contents->data[file_contents->size] = 0;
            
            succeeded = true;
        }
    }
    
    return succeeded;
}

internal bool
Win32_GetFileTimestamp(LPWSTR path, Win32_File_Timestamp* timestamp)
{
    bool succeeded = false;
    
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (GetFileAttributesExW(path, GetFileExInfoStandard, &attributes))
    {
        timestamp->creation    = (u64)attributes.ftCreationTime.dwHighDateTime   << 32 | attributes.ftCreationTime.dwLowDateTime;
        timestamp->last_write  = (u64)attributes.ftLastWriteTime.dwHighDateTime  << 32 | attributes.ftLastWriteTime.dwLowDateTime;
        timestamp->last_access = (u64)attributes.ftLastAccessTime.dwHighDateTime << 32 | attributes.ftLastAccessTime.dwLowDateTime;
        
        succeeded = true;
    }
    
    return succeeded;
}

internal bool
Win32_LoadCode(Win32_Code* code, LPWSTR code_path, LPWSTR temp_code_path, LPWSTR loaded_code_path)
{
    bool succeeded = false;
    
    if (CopyFileW(code_path, temp_code_path, 0))
    {
        HMODULE handle = LoadLibraryW(temp_code_path);
        
        if (handle == 0) Win32_Print("Failed to load code dll\n");
        else
        {
            platform_tick tick = (platform_tick)GetProcAddress(handle, "Tick");
            
            if (tick == 0) Win32_Print("Failed to load tick function\n");
            else
            {
                FreeLibrary(handle);
                FreeLibrary(code->handle);
                
                DeleteFile(loaded_code_path);
                if (MoveFile(temp_code_path, loaded_code_path))
                {
                    handle = LoadLibraryW(loaded_code_path);
                    tick = (platform_tick)GetProcAddress(handle, "Tick");
                    
                    if (handle != 0 && tick != 0)
                    {
                        code->handle    = handle;
                        code->tick = tick;
                        
                        succeeded = true;
                    }
                }
            }
        }
    }
    
    return succeeded;
}

LRESULT CALLBACK
WindowProc(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    
    if (message == WM_QUIT || message == WM_CLOSE) PostQuitMessage(0);
    else if (message == WM_CREATE || message == WM_SIZE)
    {
        RECT window_rect;
        GetWindowRect(window_handle, &window_rect);
        u32 width  = (u32)(window_rect.right - window_rect.left);
        u32 height = (u32)(window_rect.bottom - window_rect.top);
        umm byte_size = (umm)width * (umm)height * 4;
        
        ASSERT(byte_size <= U32_MAX);
        
        if (Backbuffer != 0) VirtualFree(Backbuffer, (umm)Platform->width*(umm)Platform->height * 4, MEM_RELEASE);
        Backbuffer = VirtualAlloc(0, byte_size * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        Platform->image  = (u32*)(Backbuffer + byte_size);
        Platform->width  = width;
        Platform->height = height;
    }
    
    else if (message == WM_PAINT)
    {
        PAINTSTRUCT p = {0};
        HDC dc = BeginPaint(window_handle, &p);
        
        u32 width  = Platform->width;
        u32 height = Platform->height;
        
        BITMAPINFO bmp_info = {
            .bmiHeader = {
                .biSize          = sizeof(BITMAPINFOHEADER),
                .biWidth         = width,
                .biHeight        = height,
                .biPlanes        = 1,
                .biBitCount      = 32,
                .biCompression   = BI_RGB,
                .biSizeImage     = width*height*4,
            },
        };
        
        StretchDIBits(dc, 0, height, width, -(i32)height, 0, 0, width, height, Backbuffer, &bmp_info, DIB_RGB_COLORS, SRCCOPY);
        
        EndPaint(window_handle, &p);
    }
    else result = DefWindowProc(window_handle, message, wparam, lparam);
    
    return result;
}

void __stdcall
WinMainCRTStartup()
{
    Win32_Arena = (Memory_Arena){ .base_address = (u64)Win32_ReserveMemory(GB(1)) };
    
    Memory_Arena arena_bank[2] = {0};
    arena_bank[0] = (Memory_Arena){ .base_address = (u64)Win32_ReserveMemory(GB(1)) };
    arena_bank[1] = (Memory_Arena){ .base_address = (u64)Win32_ReserveMemory(GB(1)) };
    
    Platform_Data platform_data = {
        .persistent_arena = &arena_bank[0],
        .transient_arena  = &arena_bank[1],
        .ReserveMemory    = Win32_ReserveMemory,
        .CommitMemory     = Win32_CommitMemory,
        .Print            = Win32_Print,
        .SwapBuffers      = Win32_SwapBuffers,
        .ReadEntireFile   = Win32_ReadEntireFile,
    };
    
    Platform = &platform_data;
    
    AllocConsole();
    
    HINSTANCE instance = GetModuleHandle(0);
    Win32_Code code    = {0};
    HWND window_handle = 0;
    
    WNDCLASSW window_class = {
        .lpfnWndProc   = &WindowProc,
        .hInstance     = instance,
        .lpszClassName = L"Tracer",
    };
    
    if (!RegisterClassW(&window_class)) Win32_Print("Failed to register window class\n");
    else
    {
        window_handle = CreateWindowExW(0, L"Tracer", L"Tracer", WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, // CW_USEDEFAULT, CW_USEDEFAULT,
                                        0, 0, instance, 0);
        
        if (window_handle == INVALID_HANDLE_VALUE) Win32_Print("Failed to create window\n");
        else
        {
            bool setup_failed = false;
            
            /// Setup paths
            LPWSTR code_path        = L"code.dll";
            LPWSTR temp_code_path   = L"code_loaded.dll";
            LPWSTR loaded_code_path = L"code_temp.dll";
            
            if (!setup_failed)
            {
                u32 size = MAX_PATH;
                
                void* buffer = Arena_PushSize(Platform->transient_arena, size, ALIGNOF(WCHAR));
                
                PWSTR file_path = 0;
                umm length      = 0;
                
                while (file_path == 0)
                {
                    u32 max_length = size / sizeof(WCHAR);
                    length         = GetModuleFileNameW(instance, buffer, max_length);
                    
                    if      (length == 0) break;
                    else if (length == max_length)
                    {
                        // NOTE: This will probably never happen, but failing to launch a game because the
                        //       file path to the game engine it too long is exceptionally stupid
                        size += MAX_PATH;
                    }
                    
                    else file_path = buffer;
                }
                
                umm last_backslash = 0;
                for (umm i = 0; file_path[i] != 0; ++i)
                {
                    if (file_path[i] == L'\\') last_backslash = i;
                }
                
                file_path[last_backslash + 1] = 0;
                
                BOOL set_cwd = SetCurrentDirectoryW(file_path);
                ASSERT(set_cwd);
            }
            
            /// Load code
            if (!setup_failed)
            {
                for (umm tries = 0;; ++tries)
                {
                    if (Win32_LoadCode(&code, code_path, temp_code_path, loaded_code_path))
                    {
                        Win32_File_Timestamp timestamp;
                        if (Win32_GetFileTimestamp(code_path, &timestamp))
                        {
                            code.timestamp = timestamp;
                        }
                        
                        Win32_Print("Successfully loaded code\n");
                        break;
                    }
                    
                    else if (tries == 10)
                    {
                        Win32_Print("Failed to load code\n");
                        setup_failed = true;
                        break;
                    }
                    
                    else continue;
                }
            }
            
            if (!setup_failed)
            {
                ShowWindow(window_handle, SW_SHOW);
                
                // NOTE: request minimum resolution of 1 ms for timers, affects Sleep
                timeBeginPeriod(1);
                
                LARGE_INTEGER perf_freq;
                QueryPerformanceFrequency(&perf_freq);
                
                LARGE_INTEGER flip_time;
                QueryPerformanceCounter(&flip_time);
                
                bool running = true;
                while (running)
                {
                    /// Reload code if necessary
                    Win32_File_Timestamp code_timestamp;
                    if (Win32_GetFileTimestamp(code_path, &code_timestamp) && !StructCompare(&code_timestamp, &code.timestamp))
                    {
                        if (Win32_LoadCode(&code, code_path, temp_code_path, loaded_code_path))
                        {
                            if (Win32_GetFileTimestamp(code_path, &code_timestamp))
                            {
                                code.timestamp = code_timestamp;
                            }
                            
                            Win32_Print("Successfully reloaded code\n");
                        }
                    }
                    
                    MSG msg;
                    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
                    {
                        if (msg.message == WM_QUIT)
                        {
                            running = false;
                            break;
                        }
                        
                        else DispatchMessage(&msg);
                    }
                    
                    Platform_Input input = {
                        .dir = {
                            .x = (f32)(-((GetKeyState(VK_LEFT) & (1 << 15)) != 0) + ((GetKeyState(VK_RIGHT) & (1 << 15)) != 0)),
                            .y = (f32)(-((GetKeyState(VK_DOWN) & (1 << 15)) != 0) + ((GetKeyState(VK_UP)    & (1 << 15)) != 0)),
                        },
                        .dt = 1 / 60.0f,
                    };
                    
                    u32* image = Platform->image;
                    code.tick(Platform, input);
                    
                    InvalidateRect(window_handle, 0, 0);
                    
                    /// End of frame cleanup
                    LARGE_INTEGER end_time;
                    QueryPerformanceCounter(&end_time);
                    
                    if (image != Platform->image)
                    {
                        Win32_Print("frame time: %ums\n", (u32)(1000 * (f32)(end_time.QuadPart - flip_time.QuadPart) / perf_freq.QuadPart));
                    }
                    
                    flip_time = end_time;
                }
                
                timeEndPeriod(1);
                DeleteFile(loaded_code_path);
            }
        }
    }
    
    ExitProcess(0);
}
