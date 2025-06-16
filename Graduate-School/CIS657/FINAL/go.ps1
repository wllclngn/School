<#
.SYNOPSIS
    Builds and optionally runs the XINU simulation project.
    Version: 5.29 (Separated XINU/Sim environments, refined includes)
    Date: 2025-06-16 
.DESCRIPTION
    This script automates the compilation of the XINU core process for simulation.
    Key changes:
    - xinu_includes.h (Force Included for XINU .c files & xinu_core.c):
        1. Defines (_CRT_SECURE_NO_WARNINGS, XINU_SIMULATION, _MSC_VER).
        2. AGGRESSIVE SHIMS FIRST (e.g. #define printf xinu_printf_sim_redirect).
        3. Includes "xinu.h" (which should bring in kernel.h, conf.h etc.).
        4. NO HOST SDK HEADERS in this file anymore.
    - xinu_simulation_win.c (Shim Implementations):
        1. Includes a new "xinu_sim_declarations.h" (just declares our sim functions).
        2. Includes necessary HOST SDK HEADERS (stdio.h, stdlib.h, string.h).
        3. Implements the xinu_..._sim_redirect functions.
    - New file generated: xinu_sim_declarations.h
    - This attempts to prevent XINU code from seeing host SDK headers and vice-versa.
    The date and user login in the log file and generated file headers are determined dynamically at runtime.
#>

param (
    [string]$ProjectDir = (Get-Location).Path,
    [string]$MSVCVarsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    [switch]$CleanBuild = $true,
    [switch]$RunHost = $false, 
    [string]$StarQvationTest = "" 
)

# --- Script Setup ---
$ErrorActionPreference = "Stop" 
Clear-Host

$script:currentDateForLog = Get-Date -Format "yyyy-MM-dd HH:mm:ss UTC" 
$script:currentUserForLog = $env:USERNAME 

# --- Configuration ---
$SimOutputDir = Join-Path $ProjectDir "sim_output"
$ObjDir = Join-Path $SimOutputDir "obj" 
$LogDir = $ProjectDir
$LogFile = Join-Path $LogDir "compilation.txt"

$XinuCoreName = "xinu_core.exe"
$XinuCoreOutput = Join-Path $SimOutputDir $XinuCoreName

$makefileLocation = Join-Path $ProjectDir "compile\Makefile"
$makefileDir = Split-Path $makefileLocation -Parent

$genXinuIncludesPath = Join-Path $ProjectDir "xinu_includes.h" 
$genXinuSimDeclarationsPath = Join-Path $ProjectDir "xinu_sim_declarations.h" # New file

# --- Logging Function (Defined Early) ---
function Write-Log {
    param ([string]$Message, [string]$Color = "White", [switch]$NoNewLine, [switch]$ToFileOnly)
    $consoleTimestamp = Get-Date -Format "HH:mm:ss" 
    $logEntry = "$($script:currentDateForLog) (UTC) $consoleTimestamp (local) - $Message"
    if (-not $ToFileOnly) {
        if ($NoNewLine) { Write-Host "$consoleTimestamp - $Message" -ForegroundColor $Color -NoNewline }
        else { Write-Host "$consoleTimestamp - $Message" -ForegroundColor $Color }
    }
    try {
        if ($script:LogFile) { Add-Content -Path $script:LogFile -Value $logEntry }
    } catch {
        Write-Host "CRITICAL LOGGING ERROR: Failed to write to $($script:LogFile). Error: $($_.Exception.Message)" -ForegroundColor Red
    }
}

# Initialize/Clear Log File 
try {
    Remove-Item -Path $LogFile -ErrorAction SilentlyContinue 
    Set-Content -Path $LogFile -Value "XINU Simulation Build Log - $($script:currentDateForLog) by $($script:currentUserForLog)`n"
} catch {
    Write-Host "CRITICAL LOG FILE INIT ERROR: Failed to initialize $($LogFile). Error: $($_.Exception.Message)" -ForegroundColor Red; exit 1 
}

# --- Makefile Parsing Function ---
function Parse-MakefileVariableCSource { 
    param ([string]$MakefileContent, [string]$VariableName, [string]$PathPrefixRelativeToMakefileDir)
    Write-Log "ParseFN: Attempting to parse Makefile variable: $VariableName"
    $foundFileRelativePaths = [System.Collections.Generic.List[string]]::new()
    $regexString = "(?sm)^\s*$($VariableName)\s*[+:]?=\s*(.*?)(?:^\s*(?:[A-Z_]+)\s*[+:]?=|\Z)"
    $matchResult = $false 
    try {
        if ($MakefileContent -match $regexString) { $matchResult = $true }
    } catch { Write-Log "ParseFN: CRITICAL ERROR during regex match for $VariableName. Error: $($_.Exception.Message)" -Color Red; $matchResult = $false }
    if ($matchResult) {
        $content = $matches[1].Trim() -replace '\\\s*[\r\n]+', ' ' -replace '(?m)#.*', ''
        $files = $content -split '\s+' | Where-Object { $_ -and $_ -like '*.c' } 
        foreach ($file in $files) { $foundFileRelativePaths.Add((Join-Path $PathPrefixRelativeToMakefileDir $file)) }
    } else { Write-Log "ParseFN: Variable '$VariableName' was NOT matched or error. Warning." -Color Yellow }
    return $foundFileRelativePaths
}

# --- Function to Add Items to a List ---
Function Add-ItemsToList {
    param([System.Collections.Generic.List[string]]$TargetList, [System.Collections.Generic.List[string]]$SourceList, [string]$ListNameForLogging)
    if ($SourceList -ne $null -and $SourceList.Count -gt 0) {
        $TargetList.AddRange($SourceList)
    } 
}

# --- Build Helper Functions (Invoke-MSVCCommandAndLog) ---
function Invoke-MSVCCommandAndLog {
    param([string]$FullMSVCCommand, [string]$LogFilePathForOutput)
    Write-Log "Invoke-MSVCCommandAndLog: Executing: cmd /c ""$FullMSVCCommand"""
    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = "cmd.exe"; $processInfo.Arguments = "/c ""$FullMSVCCommand"""
    $processInfo.RedirectStandardOutput = $true; $processInfo.RedirectStandardError = $true
    $processInfo.UseShellExecute = $false; $processInfo.CreateNoWindow = $true
    $process = New-Object System.Diagnostics.Process; $process.StartInfo = $processInfo
    $process.Start() | Out-Null
    $rawStdOut = $process.StandardOutput.ReadToEnd(); $rawStdErr = $process.StandardError.ReadToEnd()
    $process.WaitForExit(); $exitCode = $process.ExitCode
    Write-Log "Invoke-MSVCCommandAndLog: Process exited with code: $exitCode."
    $processedOutputLines = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrEmpty($rawStdOut)) {
        ($rawStdOut -split "[\r\n]+") | ForEach-Object { if (-not [string]::IsNullOrWhiteSpace($_)) { $processedOutputLines.Add(($_ -replace [char]0, "[NULL]")) } }
    }
    if (-not [string]::IsNullOrEmpty($rawStdErr)) {
        ($rawStdErr -split "[\r\n]+") | ForEach-Object { if (-not [string]::IsNullOrWhiteSpace($_)) { $processedOutputLines.Add(("STDERR: " + $_ -replace [char]0, "[NULL]")) } }
    }
    if ($processedOutputLines.Count -gt 0) {
        Add-Content -Path $LogFilePathForOutput -Value ($processedOutputLines -join [System.Environment]::NewLine)
    } else { Add-Content -Path $LogFilePathForOutput -Value "[Invoke-MSVCCommandAndLog: No output from command]" }
    return $exitCode
}

# --- Source File Collection ---
$xinuSourceFiles = [System.Collections.Generic.List[string]]::new() 
$allRelativePathsFromMakefile = [System.Collections.Generic.List[string]]::new()
Write-Log "Starting source file collection..." -Color Cyan
$makefileContent = Get-Content $makefileLocation -Raw

$system_C_TempList = Parse-MakefileVariableCSource $makefileContent "SYSTEM_CFILES" "../system"
Add-ItemsToList $allRelativePathsFromMakefile $system_C_TempList "SYSTEM_C_RelativePaths"
$tty_C_TempList = Parse-MakefileVariableCSource $makefileContent "TTY_CFILES" "../device/tty"
Add-ItemsToList $allRelativePathsFromMakefile $tty_C_TempList "TTY_C_RelativePaths"
$shell_C_TempList = Parse-MakefileVariableCSource $makefileContent "SHELL_CFILES" "../shell"
Add-ItemsToList $allRelativePathsFromMakefile $shell_C_TempList "SHELL_C_RelativePaths"
$libxc_C_TempList_From_Var = Parse-MakefileVariableCSource $makefileContent "LIBXCCFILES" "../lib/libxc" 
Add-ItemsToList $allRelativePathsFromMakefile $libxc_C_TempList_From_Var "LIBXC_C_RelativePaths_From_Var"

foreach ($relativePath in ($allRelativePathsFromMakefile | Select-Object -Unique)) { 
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $makefileDir $relativePath))
    if (Test-Path $fullPath -PathType Leaf) { if ($xinuSourceFiles -notcontains $fullPath) {$xinuSourceFiles.Add($fullPath)} }
}
$libXcDirFullPath = [System.IO.Path]::GetFullPath((Join-Path $makefileDir "../lib/libxc")) 
if (Test-Path $libXcDirFullPath -PathType Container) {
    Get-ChildItem -Path $libXcDirFullPath -Filter "*.c" -Recurse | ForEach-Object {
        if ($xinuSourceFiles -notcontains $_.FullName) { $xinuSourceFiles.Add($_.FullName) }
    }
}
$mainXinuCoreFile = Join-Path $ProjectDir "xinu_core.c"
if (Test-Path $mainXinuCoreFile) { if ($xinuSourceFiles -notcontains $mainXinuCoreFile) { $xinuSourceFiles.Add($mainXinuCoreFile) } }
else { Write-Log "ERROR: Main xinu_core.c not found: $mainXinuCoreFile." -Color Red; exit 1 }

$genWinSimHelperPath = Join-Path $ProjectDir "xinu_simulation_win.c" 
# Add to compile list, will be generated if missing before compilation loop
if ($xinuSourceFiles -notcontains $genWinSimHelperPath) { $xinuSourceFiles.Add($genWinSimHelperPath) }
Write-Log "Final XINU C source files for compilation (pre-build): $($xinuSourceFiles.Count)" -Color Cyan

# --- Helper File Generation Functions ---
function Generate-XinuIncludesFile { # Renamed for clarity
    param ([string]$outputPath) 
    Write-Log "Generating XINU includes wrapper ($([System.IO.Path]::GetFileName($outputPath)))..." -color Yellow
    $headerContent = @"
/* $($([System.IO.Path]::GetFileName($outputPath))) - Wrapper for XINU code compilation.
 * Generated on: $($script:currentDateForLog) by $($script:currentUserForLog)
 * Version: 5.29 (Shims first, then xinu.h. NO HOST HEADERS HERE)
 */
#ifndef _XINU_INCLUDES_H_ // Guard for this specific wrapper file
#define _XINU_INCLUDES_H_

#define _CRT_SECURE_NO_WARNINGS 
#define XINU_SIMULATION        
#ifndef _MSC_VER
  #define _MSC_VER 1930 
#endif

/* --- STAGE 1: AGGRESSIVE SHIMS FIRST --- */
/* Define standard functions to point to our simulation redirects *before*
   XINU's own headers (via xinu.h) are included. This ensures that when
   XINU headers declare these functions, they are already mapping to our redirects. */
#ifdef getchar 
#undef getchar
#endif
#define getchar() xinu_getchar_sim_redirect()
#ifdef putchar
#undef putchar
#endif
#define putchar(c) xinu_putchar_sim_redirect(c)

#define printf xinu_printf_sim_redirect
#define fprintf xinu_fprintf_sim_redirect
#define sprintf xinu_sprintf_sim_redirect
#define scanf xinu_scanf_sim_redirect
#define fscanf xinu_fscanf_sim_redirect
#define sscanf xinu_sscanf_sim_redirect
#define fgetc xinu_fgetc_sim_redirect
#define fgets xinu_fgets_sim_redirect
#define fputc xinu_fputc_sim_redirect
#define fputs xinu_fputs_sim_redirect

// Shim XINU's internal stdio helpers if they are exposed and used by XINU code
#define _doprnt xinu_doprnt_sim_redirect 
#define _doscan xinu_doscan_sim_redirect 

#ifdef abs
#undef abs
#endif
#define abs(n) xinu_abs_sim_redirect(n)
#ifdef labs
#undef labs
#endif
#define labs(n) xinu_labs_sim_redirect(n)

#define atoi(s) xinu_atoi_sim_redirect(s)
#define atol(s) xinu_atol_sim_redirect(s)

#ifdef rand
#undef rand
#endif
#define rand() xinu_rand_sim_redirect()
#ifdef srand
#undef srand
#endif
#define srand(s) xinu_srand_sim_redirect(s)

#ifdef qsort
#undef qsort
#endif
#define qsort(b,n,s,c) xinu_qsort_sim_redirect(b,n,s,c)

#define strcpy xinu_strcpy_sim_redirect
#define strncpy xinu_strncpy_sim_redirect
#define strcat xinu_strcat_sim_redirect
#define strncat xinu_strncat_sim_redirect
#define strcmp xinu_strcmp_sim_redirect
#define strncmp xinu_strncmp_sim_redirect
#define strlen xinu_strlen_sim_redirect
#define strnlen xinu_strnlen_sim_redirect 
#define strchr xinu_strchr_sim_redirect
#define strrchr xinu_strrchr_sim_redirect
#define strstr xinu_strstr_sim_redirect
#define memcpy xinu_memcpy_sim_redirect
#define memmove xinu_memmove_sim_redirect
#define memcmp xinu_memcmp_sim_redirect
#define memset xinu_memset_sim_redirect // Crucial: XINU uses this widely

/* --- STAGE 2: Full XINU Environment --- */
/* Include XINU's main header. This should define all XINU-specific types 
   (like int32, bool, pid32, etc.) and declare XINU's functions.
   The shims from Stage 1 should ensure XINU's std lib functions point to ours.
   It's CRITICAL that xinu.h and its sub-headers (kernel.h, conf.h, etc.)
   have proper include guards (e.g. #ifndef _KERNEL_H_) to prevent issues
   if they include each other in a complex way. */
#include "xinu.h" 

/* NO HOST SYSTEM HEADERS (e.g. <stdio.h>, <windows.h>) IN THIS FILE.
   Those are only for xinu_simulation_win.c's private use. */

#endif /* _XINU_INCLUDES_H_ */
"@
    $headerContent | Out-File -FilePath $outputPath -Encoding UTF8 -Force
    Write-Log "Generated XINU includes wrapper at: $outputPath" -color Green
}

function Generate-XinuSimDeclarationsFile { # NEW FUNCTION
    param ([string]$outputPath) 
    Write-Log "Generating XINU sim declarations header ($([System.IO.Path]::GetFileName($outputPath)))..." -color Yellow
    $headerContent = @"
/* $($([System.IO.Path]::GetFileName($outputPath))) - Declarations for XINU simulation shim functions.
 * Generated on: $($script:currentDateForLog) by $($script:currentUserForLog)
 * Version: 5.29
 * This file is for xinu_simulation_win.c to know about the functions it needs to implement.
 * It should NOT include xinu.h or the main xinu_includes.h to avoid circular dependencies
 * or namespace pollution from XINU types into the shim implementation's host environment.
 */
#ifndef _XINU_SIM_DECLARATIONS_H_
#define _XINU_SIM_DECLARATIONS_H_

#include <stdarg.h> // For va_list in function prototypes like printf

/* Declare all xinu_..._sim_redirect functions here.
   These must match the implementations in xinu_simulation_win.c
   and the types expected by XINU code (but using host-compatible types like int, char*).
   Using host types like FILE* from host's <stdio.h> is fine here because this
   header is only included by xinu_simulation_win.c which also includes host stdio.h. */

// Stdio
int xinu_printf_sim_redirect(const char *format, ...);
int xinu_fprintf_sim_redirect(void *stream, const char *format, ...); // Using void* for FILE* for now
int xinu_sprintf_sim_redirect(char *buffer, const char *format, ...);
int xinu_scanf_sim_redirect(const char *format, ...);
int xinu_fscanf_sim_redirect(void *stream, const char *format, ...); // Using void* for FILE*
int xinu_sscanf_sim_redirect(const char *buffer, const char *format, ...);
int xinu_getchar_sim_redirect(void);
int xinu_putchar_sim_redirect(int c);
int xinu_fgetc_sim_redirect(void *stream); // Using void* for FILE*
char* xinu_fgets_sim_redirect(char *str, int num, void *stream); // Using void* for FILE*
int xinu_fputc_sim_redirect(int c, void *stream); // Using void* for FILE*
int xinu_fputs_sim_redirect(const char *str, void *stream); // Using void* for FILE*

// XINU's internal stdio helpers (if shimming them)
// The actual signatures from XINU's stdio.h are needed if these are to be accurate.
// Assuming generic signatures for now.
typedef int (*xinu_putc_func_t)(int, int); // Example type for putc_func in _doprnt
typedef int (*xinu_getc_func_t)(int);      // Example type for getc_func in _doscan
typedef int (*xinu_ungetc_func_t)(int,int); // Example type for ungetc_func in _doscan

int xinu_doprnt_sim_redirect(char *fmt, va_list ap, xinu_putc_func_t putc_func, int putc_arg);
int xinu_doscan_sim_redirect(char *fmt, va_list ap, xinu_getc_func_t getc_func, xinu_ungetc_func_t ungetc_func, int getc_arg, int ungetc_arg);

// Stdlib
int xinu_abs_sim_redirect(int n);
long xinu_labs_sim_redirect(long n);
int xinu_atoi_sim_redirect(const char *str);
long xinu_atol_sim_redirect(const char *str);
int xinu_rand_sim_redirect(void);
void xinu_srand_sim_redirect(unsigned int seed);
// qsort's comparison function type: int (*compare)(const void *, const void *)
typedef int (*xinu_qsort_cmp_t)(const void *, const void *);
void xinu_qsort_sim_redirect(void *base, size_t num, size_t size, xinu_qsort_cmp_t compare);

// String
char* xinu_strcpy_sim_redirect(char *dest, const char *src);
char* xinu_strncpy_sim_redirect(char *dest, const char *src, size_t n);
char* xinu_strcat_sim_redirect(char *dest, const char *src);
char* xinu_strncat_sim_redirect(char *dest, const char *src, size_t n);
int xinu_strcmp_sim_redirect(const char *s1, const char *s2);
int xinu_strncmp_sim_redirect(const char *s1, const char *s2, size_t n);
size_t xinu_strlen_sim_redirect(const char *s);
size_t xinu_strnlen_sim_redirect(const char *s, size_t maxlen);
char* xinu_strchr_sim_redirect(const char *s, int c);
char* xinu_strrchr_sim_redirect(const char *s, int c);
char* xinu_strstr_sim_redirect(const char *haystack, const char *needle);
void* xinu_memcpy_sim_redirect(void *dest, const void *src, size_t n);
void* xinu_memmove_sim_redirect(void *dest, const void *src, size_t n);
int xinu_memcmp_sim_redirect(const void *s1, const void *s2, size_t n);
void* xinu_memset_sim_redirect(void *s, int c, size_t n);

// Simulation specific helpers (not shims of standard functions)
void xinu_simulation_yield(void);
void xinu_trigger_clock_interrupt(void);

#endif /* _XINU_SIM_DECLARATIONS_H_ */
"@
    $headerContent | Out-File -FilePath $outputPath -Encoding UTF8 -Force
    Write-Log "Generated XINU sim declarations at: $outputPath" -color Green
}

function Generate-WindowsSimHelperFile { # Renamed for clarity
    param ([string]$outputPath) 
    Write-Log "Generating Windows simulation helper ($([System.IO.Path]::GetFileName($outputPath)))..." -color Yellow
    $helperContent = @"
/* $([System.IO.Path]::GetFileName($outputPath)) - Helper functions for Windows Simulation
 * Generated on: $($script:currentDateForLog) by $($script:currentUserForLog)
 * Version: 5.29
 */

/* --- STAGE 1: Declarations of the functions we are implementing. --- */
#include "xinu_sim_declarations.h" // Contains prototypes for xinu_..._sim_redirect functions

/* --- STAGE 2: Host System Headers --- */
/* Include necessary host headers for the *implementations* of the shims. */
#include <stdio.h>   // For vprintf, FILE*, etc.
#include <stdlib.h>  // For abs, atoi, rand, qsort etc.
#include <string.h>  // For strcpy, strcmp, memcpy, memset etc.
#include <stdarg.h>  // For va_list, va_start, va_end
#include <ctype.h>   
#include <time.h>    
// If specific Windows functionality is needed (e.g. Sleep for yield), include <windows.h>
// #include <windows.h> // Be cautious, it's large and can introduce its own clashes if not careful

/* --- STAGE 3: Implementations for Redirected Standard Functions --- */

// Stdio
int xinu_printf_sim_redirect(const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vprintf(format, args); va_end(args); fflush(stdout); return ret;
}
int xinu_fprintf_sim_redirect(void *stream, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vfprintf((FILE*)stream, format, args); va_end(args); fflush((FILE*)stream); return ret;
}
int xinu_sprintf_sim_redirect(char *buffer, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vsprintf(buffer, format, args); va_end(args); return ret;
}
int xinu_scanf_sim_redirect(const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vscanf(format, args); va_end(args); return ret;
}
int xinu_fscanf_sim_redirect(void *stream, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vfscanf((FILE*)stream, format, args); va_end(args); return ret;
}
int xinu_sscanf_sim_redirect(const char *buffer, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vsscanf(buffer, format, args); va_end(args); return ret;
}
int xinu_getchar_sim_redirect(void) { return getchar(); }
int xinu_putchar_sim_redirect(int c) { return putchar(c); }
int xinu_fgetc_sim_redirect(void *stream) { return fgetc((FILE*)stream); }
char* xinu_fgets_sim_redirect(char *str, int num, void *stream) { return fgets(str, num, (FILE*)stream); }
int xinu_fputc_sim_redirect(int c, void *stream) { return fputc(c, (FILE*)stream); }
int xinu_fputs_sim_redirect(const char *str, void *stream) { return fputs(str, (FILE*)stream); }

int xinu_doprnt_sim_redirect(char *fmt, va_list ap, xinu_putc_func_t putc_func, int putc_arg) {
    char buffer[4096]; 
    int ret = vsprintf(buffer, fmt, ap);
    if (putc_func) { // putc_func is XINU's function pointer, e.g. fputc_xinu
        for (int i = 0; i < ret; ++i) { 
            // This call assumes XINU's putc_func matches int (*)(int char_to_put, int device_or_stream_arg)
            putc_func(buffer[i], putc_arg); 
        }
    }
    return ret;
}
int xinu_doscan_sim_redirect(char *fmt, va_list ap, xinu_getc_func_t getc_func, xinu_ungetc_func_t ungetc_func, int getc_arg, int ungetc_arg) {
    // This remains a very difficult function to shim correctly without a custom vsscanf.
    // For now, it's a non-functional stub.
    if (fmt) {} if (ap) {} if (getc_func) {} if (ungetc_func) {} if (getc_arg) {} if (ungetc_arg) {}
    // To avoid "unused parameter" warnings.
    // Consider logging if this is ever called:
    // printf("SIM_DEBUG: xinu_doscan_sim_redirect STUB called. fmt: %s\n", fmt ? fmt : "NULL");
    return 0; // Indicates failure or no items assigned (standard for *scanf).
}

// Stdlib
int xinu_abs_sim_redirect(int n) { return abs(n); }
long xinu_labs_sim_redirect(long n) { return labs(n); }
int xinu_atoi_sim_redirect(const char *str) { return atoi(str); }
long xinu_atol_sim_redirect(const char *str) { return atol(str); }
int xinu_rand_sim_redirect(void) { return rand(); }
void xinu_srand_sim_redirect(unsigned int seed) { srand(seed); }
void xinu_qsort_sim_redirect(void *base, size_t num, size_t size, xinu_qsort_cmp_t compare) {
    qsort(base, num, size, compare);
}

// String
char* xinu_strcpy_sim_redirect(char *dest, const char *src) { return strcpy(dest, src); }
char* xinu_strncpy_sim_redirect(char *dest, const char *src, size_t n) { return strncpy(dest, src, n); }
char* xinu_strcat_sim_redirect(char *dest, const char *src) { return strcat(dest, src); }
char* xinu_strncat_sim_redirect(char *dest, const char *src, size_t n) { return strncat(dest, src, n); }
int xinu_strcmp_sim_redirect(const char *s1, const char *s2) { return strcmp(s1, s2); }
int xinu_strncmp_sim_redirect(const char *s1, const char *s2, size_t n) { return strncmp(s1, s2, n); }
size_t xinu_strlen_sim_redirect(const char *s) { return strlen(s); }
size_t xinu_strnlen_sim_redirect(const char *s, size_t maxlen) { 
#if defined(_WIN32) && defined(__STDC_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__
    return strnlen_s(s, maxlen); 
#else 
    size_t i = 0; while (i < maxlen && s[i]) { ++i; } return i;
#endif
}
char* xinu_strchr_sim_redirect(const char *s, int c) { return (char*)strchr(s, c); } 
char* xinu_strrchr_sim_redirect(const char *s, int c) { return (char*)strrchr(s, c); } 
char* xinu_strstr_sim_redirect(const char *haystack, const char *needle) { return (char*)strstr(haystack, needle); } 
void* xinu_memcpy_sim_redirect(void *dest, const void *src, size_t n) { return memcpy(dest, src, n); }
void* xinu_memmove_sim_redirect(void *dest, const void *src, size_t n) { return memmove(dest, src, n); }
int xinu_memcmp_sim_redirect(const void *s1, const void *s2, size_t n) { return memcmp(s1, s2, n); }
void* xinu_memset_sim_redirect(void *s, int c, size_t n) { return memset(s, c, n); }

// Simulation specific helpers
void xinu_simulation_yield(void) { /* TODO: Implement if needed, e.g., Sleep(0); */ }
void xinu_trigger_clock_interrupt(void) { /* TODO: Implement if needed */ }
"@
    $headerContent | Out-File -FilePath $outputPath -Encoding UTF8 -Force
    Write-Log "Generated Windows simulation helper at: $outputPath" -color Green
}

# --- Build Functions ---
function Invoke-MSVCCompileToObj {
    param ([string]$SourceFile, [string]$ObjectFile, [string]$IncludeDirs, [string]$Defines, [string]$ExtraOptions, [string]$ForceIncludeFileOpt) # Renamed
    Write-Log "Compiling to OBJ: `"$SourceFile`" -> `"$ObjectFile`""
    $clOptions = "/nologo /EHsc /W3 /Od /c `"$SourceFile`" /Fo`"$ObjectFile`"" 
    
    # Determine if this is xinu_simulation_win.c - it does NOT get xinu_includes.h force-included
    $isSimHelperFile = ($SourceFile -eq $genWinSimHelperPath)

    if (-not $isSimHelperFile -and (-not [string]::IsNullOrEmpty($ForceIncludeFileOpt))) {
         $clOptions += " /FI`"$ForceIncludeFileOpt`""
    } # Else, if it IS the sim helper, it includes xinu_sim_declarations.h itself.
    
    if ($IncludeDirs -is [string] -and $IncludeDirs) { $IncludeDirs.Split(',') | ForEach-Object { $clOptions += " /I`"$_`"" } }
    
    $finalDefines = $Defines # Use the defines passed (e.g., _CONSOLE)
    if ($finalDefines) { $finalDefines.Split(',') | Where-Object {$_} | ForEach-Object { $clOptions += " /D`"$_`"" } } # Ensure no empty defines
    
    if ($ExtraOptions) { $clOptions += " $ExtraOptions" }
    $compile_command_only = "cl $clOptions"
    Write-Log "Executing (via helper): $compile_command_only"
    $full_msvc_command_to_run = "call `"$MSVCVarsPath`" && $compile_command_only"
    $exitCode = Invoke-MSVCCommandAndLog -FullMSVCCommand $full_msvc_command_to_run -LogFilePathForOutput $script:LogFile
    if ((Test-Path $ObjectFile) -and ($exitCode -eq 0)) { Write-Log "Successfully compiled to OBJ: `"$ObjectFile`"" -Color Green }
    else { Write-Log "ERROR: Compilation to OBJ failed for `"$SourceFile`". CL.exe Exit Code: $exitCode. Check $script:LogFile." -Color Red; exit 1 }
}

function Invoke-MSVCLinkObjects {
    param ([string[]]$ObjectFiles, [string]$OutputFile, [string]$ExtraOptions)
    Write-Log "Linking OBJ files to: `"$OutputFile`""
    $objFilesString = ($ObjectFiles | ForEach-Object { "`"$_`"" }) -join " "
    $clOptions = "/nologo /Fe`"$OutputFile`" $objFilesString"
    if ($ExtraOptions) { $clOptions += " $ExtraOptions" }
    $link_command_only = "cl $clOptions" 
    Write-Log "Executing (via helper): $link_command_only"
    $full_msvc_command_to_run = "call `"$MSVCVarsPath`" && $link_command_only"
    $exitCode = Invoke-MSVCCommandAndLog -FullMSVCCommand $full_msvc_command_to_run -LogFilePathForOutput $script:LogFile
    if ((Test-Path $OutputFile) -and ($exitCode -eq 0)) { Write-Log "Successfully linked: `"$OutputFile`"" -Color Green }
    else { Write-Log "ERROR: Linking failed for `"$OutputFile`". CL.exe Exit Code: $exitCode. Check $script:LogFile." -Color Red; exit 1 }
}

function Build-XINUCore {
    Write-Log "Building XINU Core Process..." -Color Magenta
    
    $includeDir1 = $ProjectDir
    $includeDir2 = Join-Path $ProjectDir 'include'
    $tempArrayOfIncludePaths = @( "`"$includeDir1`"", "`"$includeDir2`"" )
    $coreIncludeDirsString = $tempArrayOfIncludePaths -join ','
    $coreDefines = "_CONSOLE" # XINU_SIMULATION is now only in xinu_includes.h
    
    $objectFilesList = [System.Collections.Generic.List[string]]::new()
    New-Item -ItemType Directory -Path $ObjDir -Force -ErrorAction SilentlyContinue | Out-Null
    
    # Ensure helper files are generated BEFORE the loop if they are missing
    if (-not (Test-Path $genWinSimHelperPath)) { Generate-WindowsSimHelperFile -outputPath $genWinSimHelperPath }
    # xinu_includes.h and xinu_sim_declarations.h are generated in Main Script Logic

    foreach ($sourceFile in ($xinuSourceFiles | Select-Object -Unique)) { 
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($sourceFile)
        $objectFile = Join-Path $ObjDir "$baseName.obj" 
        Invoke-MSVCCompileToObj -SourceFile $sourceFile -ObjectFile $objectFile -IncludeDirs $coreIncludeDirsString -Defines $coreDefines -ForceIncludeFileOpt $genXinuIncludesPath
        $objectFilesList.Add($objectFile)
    }
    Invoke-MSVCLinkObjects -ObjectFiles ($objectFilesList | Select-Object -Unique) -OutputFile $XinuCoreOutput -ExtraOptions "/link /SUBSYSTEM:CONSOLE"
}

# --- Main Script Logic ---
Write-Log "XINU Simulation Build Script Started (v5.29)" -Color Yellow
Write-Log "Generated Includes File (for XINU code /FI): $genXinuIncludesPath"
Write-Log "Generated Sim Declarations File (for shim code): $genXinuSimDeclarationsPath"

if ($CleanBuild) {
    Write-Log "Cleaning previous build..." -Color Cyan
    if (Test-Path $SimOutputDir) { Remove-Item -Recurse -Force $SimOutputDir }
    New-Item -ItemType Directory -Path $SimOutputDir -Force -ErrorAction SilentlyContinue | Out-Null
    New-Item -ItemType Directory -Path $ObjDir -Force -ErrorAction SilentlyContinue | Out-Null
    if (Test-Path $genXinuIncludesPath) { Remove-Item -Force $genXinuIncludesPath }
    if (Test-Path $genWinSimHelperPath) { Remove-Item -Force $genWinSimHelperPath } 
    if (Test-Path $genXinuSimDeclarationsPath) { Remove-Item -Force $genXinuSimDeclarationsPath } # Clean new file
} else {
    New-Item -ItemType Directory -Path $SimOutputDir -Force -ErrorAction SilentlyContinue | Out-Null
    New-Item -ItemType Directory -Path $ObjDir -Force -ErrorAction SilentlyContinue | Out-Null
}

# Generate ALL helper C header/source files first
Generate-XinuIncludesFile -outputPath $genXinuIncludesPath 
Generate-XinuSimDeclarationsFile -outputPath $genXinuSimDeclarationsPath # Generate new declarations file
Generate-WindowsSimHelperFile -outputPath $genWinSimHelperPath # Generates xinu_simulation_win.c

if (-not (Test-Path $MSVCVarsPath)) { Write-Log "ERROR: MSVCVarsPath not found: $MSVCVarsPath." -Color Red; exit 1 }

Build-XINUCore
Write-Log "XINU Core Build Process Finished." -Color Magenta

if ($RunHost) {
    if (Test-Path $XinuCoreOutput) {
        Write-Log "Running XINU Core: $XinuCoreOutput $StarvationTest" -Color Cyan
        Start-Process -FilePath $XinuCoreOutput -ArgumentList $StarvationTest
    } else { Write-Log "ERROR: XINU Core executable not found to run: $XinuCoreOutput" -Color Red }
}
Write-Log "XINU Simulation Build Script Finished." -Color Yellow