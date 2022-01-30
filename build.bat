@echo off

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)

set "diag_options= /nologo /FC /diagnostics:column /W4 /wd4996 /wd4116 /wd4100 /wd4201 /wd4101 /wd4204 /wd4200"
set "nocrt_options= /Zl /GR- /GS- /Gs9999999"
set "noctr_link= /STACK:0x100000,0x100000 /NODEFAULTLIB /SUBSYSTEM:windows"
set "fast_fp= /fp:fast /fp:except-"

IF NOT EXIST .\build mkdir build
pushd .\build

IF "%1"=="all" (
set /A main=1
set /A tracer=1
) ELSE IF "%1"=="tracer" (
set /A main=0
set /A tracer=1
) ELSE IF "%1"=="main" (
set /A main=1
set /A tracer=0
) ELSE (
echo Invalid arguments. Usage: build.bat [all/tracer/main] [debug/internal/release]
GOTO :DONE
)

IF "%2"=="debug" (
set "compile_options= /DTR_DEBUG %nocrt_options% %diag_options% /Od /Zo /Zf /Z7"
) ELSE IF "%1"=="internal" (
set "compile_options= %nocrt_options% %diag_options% /O2 /Zo /Zf /Z7"
) ELSE (
set "compile_options= %nocrt_options% %diag_options% /O2 /Zo /Zf /Z7"
)

set "link_options= /INCREMENTAL:NO /opt:ref /STACK:0x100000,0x100000 /NODEFAULTLIB /SUBSYSTEM:windows"

IF /I "%tracer%" EQU "1" (
	cl %compile_options% ..\src\tr_win32.c /link %link_options% /PDB:tracer.pdb /ENTRY:WinMainCRTStartup Kernel32.lib Winmm.lib User32.lib Gdi32.lib /out:tracer.exe
	del tr_win32.obj
	copy tracer.exe ..\run_path\.
	copy tracer.pdb ..\run_path\.
)
IF /I "%main%" EQU "1" (
	cl %compile_options% ..\src\tr_main.c /LD /link %link_options% Kernel32.lib /EXPORT:Tick /out:code.dll
	del tr_main.obj
	del tr_main.lib
	del tr_main.exp
	copy code.dll ..\run_path\.
	copy code.pdb ..\run_path\.
)

:DONE

popd