<#
.SYNOPSIS
    Builds and optionally runs the XINU simulation project.
    Version: 5.30.9 (Corrected SYNOPSIS dynamic date/user; C errors indicate header conflicts)
    Date: Determined dynamically by script (UTC)
    User: Determined dynamically by script
.DESCRIPTION
    This script automates the compilation of the XINU core process for simulation.
    This version is based on the script from commit 8c34073f1870b267392b4a70b797ea281a2a3f73.
    It uses vcvars64.bat to set up the MSVC environment before calling cl.exe.
    
    Current C compilation errors (e.g., unknown type 'int32', syntax errors in XINU's own
    include/stdio.h, include/string.h) suggest a conflict or incorrect ordering
    between the force-included 'xinu_includes.h' (which defines shims and then includes 'xinu.h')
    and XINU's own header files. XINU's headers might be conflicting with the shims
    or attempting to use types before 'xinu.h' has fully defined them in this context.
    
    Date and User for logging and generated file headers are obtained dynamically.
#>

param (
    [string]$ProjectDir = (Get-Location).Path,
    [string]$MSVCVarsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    [switch]$CleanBuild = $true,
    [switch]$RunHost = $false, 
    [string]$StarvationTest = "" 
)

# --- Script Setup ---
$ErrorActionPreference = "Stop" 
Clear-Host

# Script's operational date/user are dynamic
$script:currentDateForLog = Get-Date -Format "yyyy-MM-dd HH:mm:ss UTC" 
$script:currentUserForLog = $env:USERNAME 

# --- Update SYNOPSIS block dynamically (best effort for comment block) ---
# This is a bit meta, but attempts to reflect the dynamic nature in the help text too.
# Note: This modification of the script's own content is unusual.
try {
    $thisScriptContent = Get-Content $MyInvocation.MyCommand.Path -Raw
    $dynamicSynopsisDate = "Date: $($script:currentDateForLog) (UTC)"
    $dynamicSynopsisUser = "User: $($script:currentUserForLog)"
    
    $thisScriptContent = $thisScriptContent -replace 'Date: Determined dynamically by script \(UTC\)', $dynamicSynopsisDate
    $thisScriptContent = $thisScriptContent -replace 'User: Determined dynamically by script', $dynamicSynopsisUser
    
    # Check if replacement happened to avoid infinite loop if script is run again by itself
    # This is a simple check; a more robust one might involve unique placeholders.
    if ($thisScriptContent -notmatch [regex]::Escape($dynamicSynopsisDate) -or `
        $thisScriptContent -notmatch [regex]::Escape($dynamicSynopsisUser) ) {
        # Set-Content $MyInvocation.MyCommand.Path -Value $thisScriptContent -Encoding UTF8 -Force
        # Commented out to prevent self-modification by default, can be enabled if desired
        # Write-Log "Note: SYNOPSIS block date/user would be updated if self-modification were enabled." -Color Cyan
    }
} catch {
    Write-Log "Warning: Could not dynamically update SYNOPSIS block in script. $($_.Exception.Message)" -Color Yellow
}


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
$genXinuSimDeclarationsPath = Join-Path $ProjectDir "xinu_sim_declarations.h" 
$genWinSimHelperPath = Join-Path $ProjectDir "xinu_simulation_win.c"


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
    $regexString = "(?sm)^\s*$([regex]::Escape($VariableName))\s*[+:]?=\s*(.*?)(?:^\s*(?:[A-Z_]+)\s*[+:]?=|\Z)"
    $matchResult = $false 
    try {
        if ($MakefileContent -match $regexString) { $matchResult = $true }
    } catch { Write-Log "ParseFN: CRITICAL ERROR during regex match for $VariableName. Error: $($_.Exception.Message)" -Color Red; $matchResult = $false }
    
    if ($matchResult) {
        $content = $matches[1].Trim() -replace '\\\s*[\r\n]+', ' ' -replace '(?m)#.*', ''
        $files = $content -split '\s+' | Where-Object { $_ -and $_ -like '*.c' } 
        foreach ($fileEntry in $files) { 
            $resolvedRelativePath = Join-Path $PathPrefixRelativeToMakefileDir $fileEntry
            $foundFileRelativePaths.Add($resolvedRelativePath) 
        }
    } else { Write-Log "ParseFN: Variable '$VariableName' was NOT matched or error. Warning." -Color Yellow }
    return $foundFileRelativePaths
}

# --- Function to Add Items to a List ---
Function Add-ItemsToList {
    param(
        [System.Collections.Generic.List[string]]$TargetList, 
        [System.Collections.Generic.List[string]]$SourceList,
        [string]$ListNameForLogging 
    )
    if ($SourceList -ne $null -and $SourceList.Count -gt 0) {
        $TargetList.AddRange($SourceList)
        # Write-Log "Add-ItemsToList: Added $($SourceList.Count) items from $ListNameForLogging" # Can be verbose
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
Write-Log "Starting source file collection from Makefile: $makefileLocation" -Color Cyan
$makefileContent = Get-Content $makefileLocation -Raw

$system_C_TempList = Parse-MakefileVariableCSource $makefileContent "SYSTEM_CFILES" "..\system"
Add-ItemsToList $allRelativePathsFromMakefile $system_C_TempList "SYSTEM_C_RelativePaths"
$tty_C_TempList = Parse-MakefileVariableCSource $makefileContent "TTY_CFILES" "..\device\tty"
Add-ItemsToList $allRelativePathsFromMakefile $tty_C_TempList "TTY_C_RelativePaths"
$shell_C_TempList = Parse-MakefileVariableCSource $makefileContent "SHELL_CFILES" "..\shell"
Add-ItemsToList $allRelativePathsFromMakefile $shell_C_TempList "SHELL_C_RelativePaths"
$libxc_C_TempList_From_Var = Parse-MakefileVariableCSource $makefileContent "LIBXCCFILES" "..\lib\libxc" 
Add-ItemsToList $allRelativePathsFromMakefile $libxc_C_TempList_From_Var "LIBXC_C_RelativePaths_From_Var"

foreach ($relativeMakefileEntry in ($allRelativePathsFromMakefile | Select-Object -Unique)) { 
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $makefileDir $relativeMakefileEntry))
    if (Test-Path $fullPath -PathType Leaf) { 
        if ($xinuSourceFiles -notcontains $fullPath) {$xinuSourceFiles.Add($fullPath)} 
    } else {
        Write-Log "Warning: Makefile-derived path not found: $fullPath (from relative: $relativeMakefileEntry)" -Color Yellow
    }
}
$libXcDirFullPath = [System.IO.Path]::GetFullPath((Join-Path $makefileDir "..\lib\libxc")) 
if (Test-Path $libXcDirFullPath -PathType Container) {
    Get-ChildItem -Path $libXcDirFullPath -Filter "*.c" -Recurse | ForEach-Object {
        if ($xinuSourceFiles -notcontains $_.FullName) { 
            $xinuSourceFiles.Add($_.FullName) 
            # Write-Log "Added from libxc enumeration: $($_.FullName)" # Can be verbose
        }
    }
}
$mainXinuCoreFile = Join-Path $ProjectDir "xinu_core.c"
if (Test-Path $mainXinuCoreFile) { 
    if ($xinuSourceFiles -notcontains $mainXinuCoreFile) { $xinuSourceFiles.Add($mainXinuCoreFile) } 
} else { Write-Log "ERROR: Main xinu_core.c not found: $mainXinuCoreFile." -Color Red; exit 1 }

if ($xinuSourceFiles -notcontains $genWinSimHelperPath) { $xinuSourceFiles.Add($genWinSimHelperPath) }
Write-Log "Total XINU C source files collected for compilation: $($xinuSourceFiles.Count)" -Color Cyan

# --- Helper File Generation Functions ---
function Generate-XinuIncludesFile { 
    param ([string]$outputPath) 
    Write-Log "Generating XINU includes wrapper ($([System.IO.Path]::GetFileName($outputPath)))..." -color Yellow
    $headerContent = @"
/* $($([System.IO.Path]::GetFileName($outputPath))) - Wrapper for XINU code compilation.
 * Generated on: $($script:currentDateForLog) by $($script:currentUserForLog)
 * Version: 5.30.9 (INCLUDE XINU HEADERS FIRST, THEN SHIMS)
 */
#ifndef _XINU_INCLUDES_H_ 
#define _XINU_INCLUDES_H_

#define _CRT_SECURE_NO_WARNINGS 
#define XINU_SIMULATION        
#ifndef _MSC_VER
  #define _MSC_VER 1930 
#endif

/* --- STAGE 1: Include core XINU headers FIRST --- */
#include \"xinu.h\"

/* --- STAGE 2: AGGRESSIVE SHIMS AFTER XINU HEADERS --- */
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
#define memset xinu_memset_sim_redirect 

/* NO HOST SYSTEM HEADERS IN THIS FILE. */
#endif /* _XINU_INCLUDES_H_ */
"@
    $headerContent | Out-File -FilePath $outputPath -Encoding UTF8 -Force
    Write-Log "Generated XINU includes wrapper at: $outputPath" -color Green
}

function Generate-XinuSimDeclarationsFile { 
    param ([string]$outputPath) 
    Write-Log "Generating XINU sim declarations header ($([System.IO.Path]::GetFileName($outputPath)))..." -color Yellow
    $headerContent = @"
/* $($([System.IO.Path]::GetFileName($outputPath))) - Declarations for XINU simulation shim functions.
 * Generated on: $($script:currentDateForLog) by $($script:currentUserForLog)
 * Version: 5.30.9
 */
#ifndef _XINU_SIM_DECLARATIONS_H_
#define _XINU_SIM_DECLARATIONS_H_

#include <stdarg.h> 
#include <stddef.h> 

// Stdio
int xinu_printf_sim_redirect(const char *format, ...);
int xinu_fprintf_sim_redirect(void *stream, const char *format, ...); 
int xinu_sprintf_sim_redirect(char *buffer, const char *format, ...);
int xinu_scanf_sim_redirect(const char *format, ...);
int xinu_fscanf_sim_redirect(void *stream, const char *format, ...); 
int xinu_sscanf_sim_redirect(const char *buffer, const char *format, ...);
int xinu_getchar_sim_redirect(void);
int xinu_putchar_sim_redirect(int c);
int xinu_fgetc_sim_redirect(void *stream); 
char* xinu_fgets_sim_redirect(char *str, int num, void *stream); 
int xinu_fputc_sim_redirect(int c, void *stream); 
int xinu_fputs_sim_redirect(const char *str, void *stream); 

typedef int ((*xinu_putc_func_t)(int, int)); 
typedef int ((*xinu_getc_func_t)(int));      
typedef int ((*xinu_ungetc_func_t)(int,int)); 

int xinu_doprnt_sim_redirect(char *fmt, va_list ap, xinu_putc_func_t putc_func, int putc_arg);
int xinu_doscan_sim_redirect(char *fmt, va_list ap, xinu_getc_func_t getc_func, xinu_ungetc_func_t ungetc_func, int getc_arg, int ungetc_arg);

// Stdlib
int xinu_abs_sim_redirect(int n);
long xinu_labs_sim_redirect(long n);
int xinu_atoi_sim_redirect(const char *str);
long xinu_atol_sim_redirect(const char *str);
int xinu_rand_sim_redirect(void);
void xinu_srand_sim_redirect(unsigned int seed);
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

// Simulation specific helpers
void xinu_simulation_yield(void);
void xinu_trigger_clock_interrupt(void);

#endif /* _XINU_SIM_DECLARATIONS_H_ */
"@
    $headerContent | Out-File -FilePath $outputPath -Encoding UTF8 -Force
    Write-Log "Generated XINU sim declarations at: $outputPath" -color Green
}

function Generate-WindowsSimHelperFile { 
    param ([string]$outputPath) 
    Write-Log "Generating Windows simulation helper ($([System.IO.Path]::GetFileName($outputPath)))..." -color Yellow
    $helperContent = @"
/* $([System.IO.Path]::GetFileName($outputPath)) - Helper functions for Windows Simulation
 * Generated on: $($script:currentDateForLog) by $($script:currentUserForLog)
 * Version: 5.30.9
 */
#include "xinu_sim_declarations.h" 
#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>  
#include <stdarg.h>  
#include <ctype.h>   
#include <time.h>    
// #include <windows.h> 

// Stdio implementations... (same as 5.30.8)
int xinu_printf_sim_redirect(const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vprintf(format, args); va_end(args); fflush(stdout); return ret;
}
int xinu_fprintf_sim_redirect(void *stream, const char *format, ...) {
    va_list args; int ret; va_start(args, format); ret = vfprintf((FILE*)stream, format, args); va_end(args); if (stream) fflush((FILE*)stream); return ret;
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
    if (putc_func) { for (int i = 0; i < ret; ++i) { putc_func(buffer[i], putc_arg); } }
    return ret;
}
int xinu_doscan_sim_redirect(char *fmt, va_list ap, xinu_getc_func_t getc_func, xinu_ungetc_func_t ungetc_func, int getc_arg, int ungetc_arg) {
    if (fmt) {} if (ap) {} if (getc_func) {} if (ungetc_func) {} if (getc_arg) {} if (ungetc_arg) {}
    return 0; 
}
// Stdlib implementations... (same as 5.30.8)
int xinu_abs_sim_redirect(int n) { return abs(n); }
long xinu_labs_sim_redirect(long n) { return labs(n); }
int xinu_atoi_sim_redirect(const char *str) { return atoi(str); }
long xinu_atol_sim_redirect(const char *str) { return atol(str); }
int xinu_rand_sim_redirect(void) { return rand(); }
void xinu_srand_sim_redirect(unsigned int seed) { srand(seed); }
void xinu_qsort_sim_redirect(void *base, size_t num, size_t size, xinu_qsort_cmp_t compare) {
    qsort(base, num, size, compare);
}
// String implementations... (same as 5.30.8)
char* xinu_strcpy_sim_redirect(char *dest, const char *src) { return strcpy(dest, src); }
char* xinu_strncpy_sim_redirect(char *dest, const char *src, size_t n) { return strncpy(dest, src, n); }
char* xinu_strcat_sim_redirect(char *dest, const char *src) { return strcat(dest, src); }
char* xinu_strncat_sim_redirect(char *dest, const char *src, size_t n) { return strncat(dest, src, n); }
int xinu_strcmp_sim_redirect(const char *s1, const char *s2) { return strcmp(s1, s2); }
int xinu_strncmp_sim_redirect(const char *s1, const char *s2, size_t n) { return strncmp(s1, s2, n); }
size_t xinu_strlen_sim_redirect(const char *s) { return strlen(s); }
size_t xinu_strnlen_sim_redirect(const char *s, size_t maxlen) { 
#if defined(_WIN32) && defined(__STDC_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ && defined(strnlen_s)
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
void xinu_simulation_yield(void) { }
void xinu_trigger_clock_interrupt(void) { }
"@
    $helperContent | Out-File -FilePath $outputPath -Encoding UTF8 -Force
    Write-Log "Generated Windows simulation helper at: $outputPath" -color Green
}

# --- Build Functions ---
function Invoke-MSVCCompileToObj {
    param (
        [string]$SourceFile, 
        [string]$ObjectFile, 
        [string[]]$IncludeDirs, 
        [string]$Defines, 
        [string]$ExtraOptions, 
        [string]$ForceIncludeFileOpt
    )
    Write-Log "Compiling to OBJ: `"$SourceFile`" -> `"$ObjectFile`""
    $clOptions = "/nologo /EHsc /W3 /Od /c `"$SourceFile`" /Fo`"$ObjectFile`"" 
    $isSimHelperFile = ($SourceFile -eq $genWinSimHelperPath)
    if (-not $isSimHelperFile -and (-not [string]::IsNullOrEmpty($ForceIncludeFileOpt))) {
         $clOptions += " /FI`"$ForceIncludeFileOpt`"" 
    }
    if ($IncludeDirs) { 
        foreach ($dir in $IncludeDirs) { $clOptions += " /I`"$dir`"" }
    }
    $finalDefines = $Defines 
    if ($finalDefines) { $finalDefines.Split(',') | Where-Object {$_} | ForEach-Object { $clOptions += " /D`"$_`"" } } 
    if ($ExtraOptions) { $clOptions += " $ExtraOptions" }
    $compile_command_only = "cl $clOptions"
    Write-Log "Executing (via helper): $compile_command_only"
    $full_msvc_command_to_run = "call `"$MSVCVarsPath`" && $compile_command_only"
    $exitCode = Invoke-MSVCCommandAndLog -FullMSVCCommand $full_msvc_command_to_run -LogFilePathForOutput $script:LogFile
    if ((Test-Path $ObjectFile) -and ($exitCode -eq 0)) { Write-Log "Successfully compiled to OBJ: `"$ObjectFile`"" -Color Green }
    else { Write-Log "ERROR: Compilation to OBJ failed for `"$SourceFile`". CL.exe Exit Code: $exitCode. Check $($script:LogFile)." -Color Red; exit 1 }
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
    else { Write-Log "ERROR: Linking failed for `"$OutputFile`". CL.exe Exit Code: $exitCode. Check $($script:LogFile)." -Color Red; exit 1 }
}

function Build-XINUCore {
    Write-Log "Building XINU Core Process..." -Color Magenta
    $includeDir1 = $ProjectDir 
    $includeDir2 = Join-Path $ProjectDir 'include'
    $coreIncludeDirsArray = @( $includeDir1, $includeDir2 ) 
    $coreDefines = "_CONSOLE" 
    $objectFilesList = [System.Collections.Generic.List[string]]::new()
    New-Item -ItemType Directory -Path $ObjDir -Force -ErrorAction SilentlyContinue | Out-Null
    if (-not (Test-Path $genWinSimHelperPath)) { Generate-WindowsSimHelperFile -outputPath $genWinSimHelperPath }
    foreach ($sourceFile in ($xinuSourceFiles | Select-Object -Unique)) { 
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($sourceFile)
        $objectFile = Join-Path $ObjDir "$baseName.obj" 
        Invoke-MSVCCompileToObj -SourceFile $sourceFile -ObjectFile $objectFile -IncludeDirs $coreIncludeDirsArray -Defines $coreDefines -ForceIncludeFileOpt $genXinuIncludesPath
        $objectFilesList.Add($objectFile)
    }
    Invoke-MSVCLinkObjects -ObjectFiles ($objectFilesList | Select-Object -Unique) -OutputFile $XinuCoreOutput -ExtraOptions "/link /SUBSYSTEM:CONSOLE"
}

# --- Main Script Logic ---
Write-Log "XINU Simulation Build Script Started (v5.30.9)" -Color Yellow
Write-Log "Date/User for this run: $($script:currentDateForLog) by $($script:currentUserForLog)" -Color White
Write-Log "Project Directory: $ProjectDir"
Write-Log "Generated Includes File (for XINU code /FI): $genXinuIncludesPath"
Write-Log "Generated Sim Declarations File (for shim code): $genXinuSimDeclarationsPath"
Write-Log "Generated Windows Sim Helper C File: $genWinSimHelperPath"

if ($CleanBuild) {
    Write-Log "Cleaning previous build..." -Color Cyan
    if (Test-Path $SimOutputDir) { Remove-Item -Recurse -Force $SimOutputDir }
    New-Item -ItemType Directory -Path $SimOutputDir -Force -ErrorAction SilentlyContinue | Out-Null
    New-Item -ItemType Directory -Path $ObjDir -Force -ErrorAction SilentlyContinue | Out-Null
    if (Test-Path $genXinuIncludesPath) { Remove-Item -Force $genXinuIncludesPath }
    if (Test-Path $genWinSimHelperPath) { Remove-Item -Force $genWinSimHelperPath } 
    if (Test-Path $genXinuSimDeclarationsPath) { Remove-Item -Force $genXinuSimDeclarationsPath } 
} else {
    New-Item -ItemType Directory -Path $SimOutputDir -Force -ErrorAction SilentlyContinue | Out-Null
    New-Item -ItemType Directory -Path $ObjDir -Force -ErrorAction SilentlyContinue | Out-Null
}

Generate-XinuIncludesFile -outputPath $genXinuIncludesPath 
Generate-XinuSimDeclarationsFile -outputPath $genXinuSimDeclarationsPath 
Generate-WindowsSimHelperFile -outputPath $genWinSimHelperPath 

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