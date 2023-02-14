@echo off
setlocal

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt or alternatively call vcvarsall.
    exit /b 1
)

set "root_dir=%cd%"
set "build_dir=%root_dir%\build"
set "src_dir=%root_dir%\src"
set "run_dir=%root_dir%\run_path"

set "common_comp_options=/GS- /Oi /W4 /wd4201"
set "common_link_options=/INCREMENTAL:no /opt:icf /opt:ref libvcruntime.lib kernel32.lib %temp_link_libs%"

if "%1"=="debug" (
	set "comp_options=%common_comp_options% /DTR_DEBUG=1 /Od /Zo /Z7 /MTd /RTC1 /wd4100"
	set "link_options=%common_link_options% libucrtd.lib libvcruntimed.lib"
) else if "%1"=="release" (
	set "comp_options=%common_comp_options% /O2"
	set "link_options=%common_link_options%"
) else (
	echo Illegal first argument^, must be one of ^<debug^|release^>
	goto end
)

if "%2" neq "" (
	echo Illegal number of arguments^, expected^: build ^<debug^|release^>
	goto end
)

if not exist %build_dir% mkdir %build_dir%
cd %build_dir%

cl /nologo /diagnostics:caret %comp_options% %src_dir%\tracer.c /link %link_options% /fixed /SUBSYSTEM:windows /out:tracer.exe /pdb:tracer.pdb user32.lib opengl32.lib gdi32.lib winmm.lib

:end
endlocal
