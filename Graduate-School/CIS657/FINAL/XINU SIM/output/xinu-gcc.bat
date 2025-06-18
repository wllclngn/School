@echo off
REM XINU Windows compatibility wrapper for GCC
REM Generated on 2025-06-18 14:23:09

REM Save all arguments
set ARGS=

:parse
if "%1"=="" goto execute
set ARGS=%ARGS% %1
shift
goto parse

:execute
REM Call GCC with our compatibility directory first in include path
gcc -I"C:\Users\mol\Desktop\School\Graduate-School\CIS657\FINAL\XINU SIM\output" -I"C:\Users\mol\Desktop\School\Graduate-School\CIS657\FINAL\XINU SIM\output\windows_compat" %ARGS%
exit /b %ERRORLEVEL%
