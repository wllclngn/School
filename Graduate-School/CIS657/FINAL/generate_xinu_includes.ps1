[CmdletBinding()]
param ()

$PSScriptRoot = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$projectDir = $PSScriptRoot 
$makefilePath = Join-Path -Path $projectDir -ChildPath "compile\Makefile" 
$file_list_json = Join-Path -Path $projectDir -ChildPath "file_list.json"
$includesFile = Join-Path -Path $projectDir -ChildPath "xinu.h" 
$xinu_init_file = Join-Path -Path $projectDir -ChildPath "xinu_init.c" 

Write-Host "--- generate_xinu_includes.ps1 (User: $env:USERNAME, Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss UTC')) ---"
Write-Host "Attempting to parse Makefile: $makefilePath"

if (-not (Test-Path $makefilePath)) {
    Write-Host "FATAL ERROR: Makefile not found at $makefilePath" -ForegroundColor Red
    exit 1
}

$makefileContent = Get-Content -Path $makefilePath -Raw

function Get-MakefileVariableValue {
    param (
        [string]$Content,
        [string]$VariableName
    )
    $escapedVarName = [regex]::Escape($VariableName) 
    # Original complex regex for '='
    $regexPattern = "(?sm)^\s*$escapedVarName\s*=\s*((?:(?:(?!\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)).)*?\\\r?\n)*.*?(?=\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)))"
    $match = [regex]::Match($Content, $regexPattern)
    if ($match.Success) {
        return ($match.Groups[1].Value.Trim() -replace '\\\r?\n', ' ' -replace '\s+', ' ').Trim()
    }
    return $null
}

function Get-MakefileVariableAppends {
    param (
        [string]$Content,
        [string]$VariableName
    )
    Write-Host "DEBUG (Get-Appends): Called for '$VariableName'" -ForegroundColor Cyan
    $appendedValues = ""
    
    # REMOVED HACK for SYSTEM_CFILES to test the new simpler regex for all.
    
    $escapedVarName = [regex]::Escape($VariableName)
    # === SIMPLIFIED REGEX for += (will NOT handle multi-line correctly) ===
    $regexPattern = "(?m)^\s*$escapedVarName\s*\+=\s*(.*)" 
    Write-Host "DEBUG (Get-Appends): SIMPLIFIED Regex for '$VariableName' is: $regexPattern" -ForegroundColor Yellow
    
    $matches = $null
    try {
        $matches = [regex]::Matches($Content, $regexPattern)
    } catch {
        Write-Host "ERROR (Get-Appends): Regex match operation failed for '$VariableName'. Exception: $($_.Exception.Message)" -ForegroundColor Red
        return $null
    }

    if ($matches -eq $null) {
        Write-Host "DEBUG (Get-Appends): Regex matches returned null for '$VariableName'." -ForegroundColor Yellow
        return $null
    }

    Write-Host "DEBUG (Get-Appends): Found $($matches.Count) append matches for '$VariableName' (using SIMPLIFIED regex)." -ForegroundColor Cyan
    
    if ($matches.Count -gt 0) {
        foreach ($matchInstance in $matches) {
            Write-Host "DEBUG (Get-Appends): Processing a match instance..." -ForegroundColor Cyan
            if ($matchInstance.Success) {
                # With the simplified regex, the value is in Group 1
                Write-Host "DEBUG (Get-Appends): Match successful. Group 1 Value: $($matchInstance.Groups[1].Value)" -ForegroundColor Cyan
                # Simpler processing as we don't expect escaped newlines with this regex
                $currentAppend = $matchInstance.Groups[1].Value.Trim() 
                Write-Host "DEBUG (Get-Appends): Trimmed append: '$currentAppend'" -ForegroundColor Cyan
                if ($currentAppend) { 
                     if ($appendedValues -ne "") { $appendedValues += " " }
                     $appendedValues += $currentAppend
                }
            } else {
                Write-Host "DEBUG (Get-Appends): Match instance was not successful." -ForegroundColor Yellow
            }
        }
    }
    Write-Host "DEBUG (Get-Appends): Returning for '$VariableName': '$appendedValues'" -ForegroundColor Cyan
    return if ($appendedValues) { $appendedValues } else { $null } 
}

# --- Stage 1: Parse COMPS ---
# (No changes to Stage 1)
$components = [System.Collections.Generic.List[string]]::new()
Write-Host "Attempting to parse COMPS variable..."
$compsValueString = Get-MakefileVariableValue -Content $makefileContent -VariableName "COMPS"

if ($compsValueString) {
    [string[]]$componentNamesArray = $compsValueString.Split(' ') | Where-Object { $_ -ne "" -and -not $_.StartsWith("#") }
    if ($componentNamesArray.Length -gt 0) {
        $components.AddRange($componentNamesArray)
        Write-Host "SUCCESS: Found components: $($components -join ', ')" -ForegroundColor Green
    } else {
         Write-Host "WARNING: COMPS variable parsed, but yielded no components after filtering. Value: '$compsValueString'" -ForegroundColor Yellow
    }
} else {
    Write-Host "ERROR: Could not parse COMPS variable from Makefile." -ForegroundColor Red
}

# --- DEBUG TEST BLOCK ---
# (No changes to the DEBUG TEST BLOCK structure, it will now use the simplified regex for appends)
Write-Host "DEBUG: Entering pre-Stage 2 test block."
try {
    $testVar1 = "SYSTEM_CFILES"
    Write-Host "DEBUG: Testing Get-MakefileVariableValue for $testVar1"
    $val1 = Get-MakefileVariableValue -Content $makefileContent -VariableName $testVar1
    Write-Host "DEBUG: Get-MakefileVariableValue for $testVar1 OK. Value length: $($val1.Length)"

    Write-Host "DEBUG: Testing Get-MakefileVariableAppends for $testVar1 (using SIMPLIFIED regex)"
    $app1 = Get-MakefileVariableAppends -Content $makefileContent -VariableName $testVar1
    Write-Host "DEBUG: Get-MakefileVariableAppends for $testVar1 OK. Value: '$app1'"

    $testVar2 = "SYSTEM_SFILES" 
    Write-Host "DEBUG: Testing Get-MakefileVariableValue for $testVar2"
    $val2 = Get-MakefileVariableValue -Content $makefileContent -VariableName $testVar2
    Write-Host "DEBUG: Get-MakefileVariableValue for $testVar2 OK. Value: '$val2'"

    Write-Host "DEBUG: Testing Get-MakefileVariableAppends for $testVar2 (using SIMPLIFIED regex)"
    $app2 = Get-MakefileVariableAppends -Content $makefileContent -VariableName $testVar2
    Write-Host "DEBUG: Get-MakefileVariableAppends for $testVar2 OK. Value: '$app2'"

} catch {
    Write-Host "ERROR in DEBUG TEST BLOCK: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host "DEBUG: Exiting pre-Stage 2 test block."
# --- END DEBUG TEST BLOCK ---


# --- Stage 2: Parse _CFILES and _SFILES for each component ---
# (Calls within the loop remain commented out)
$sourceFilePaths = [System.Collections.Generic.List[string]]::new()
# ... (rest of Stage 2, 3, 4 are the same) ...
if ($components.Count -gt 0) {
    Write-Host "Attempting to parse source files for each component..."
    foreach ($componentName_outer in $components) { 
        $compMakefileVarBase = $componentName_outer.ToUpper() 
        if ($componentName_outer -eq "device/tty") {
            $compMakefileVarBase = "TTY" 
        } else {
             $compMakefileVarBase = $compMakefileVarBase -replace '[\\/]', '_' 
        }
        Write-Host "  Component: $componentName_outer (Variable base for Makefile: ${compMakefileVarBase})"

        foreach ($fileType_outer in @("CFILES", "SFILES")) { 
            if ($fileType_outer -eq "CFILES") {
                $fileExt_outer = ".c" 
            } else {
                $fileExt_outer = ".S" 
            }
            $varName_outer = "${compMakefileVarBase}_${fileType_outer}"
            
            # $initialValue = Get-MakefileVariableValue -Content $makefileContent -VariableName $varName_outer 
            # $appendedValue = Get-MakefileVariableAppends -Content $makefileContent -VariableName $varName_outer 
            
        }
    }
} else {
    Write-Host "Skipping source file parsing as no components were found." -ForegroundColor Yellow
}

# --- Stage 3: Generate file_list.json ---
$fileListData = @{ "files" = @() }
$actualSourceFilesForCompilation = [System.Collections.Generic.List[string]]::new()

foreach ($sf_path_json in $sourceFilePaths) { 
    $fileListData.files += @{ "path" = $sf_path_json; "type" = "source" } 
    $actualSourceFilesForCompilation.Add($sf_path_json)
}

$simCFile_json = "xinu_simulation.c"; $initCFile_json = "xinu_init.c" 
if (Test-Path (Join-Path -Path $projectDir -ChildPath $simCFile_json)) {
    $fileListData.files += @{ "path" = $simCFile_json; "type" = "source" }; $actualSourceFilesForCompilation.Add($simCFile_json)
} else {
    Write-Host "WARNING: Simulation C file '$simCFile_json' not found in $projectDir, will not be added to list." -ForegroundColor Yellow
}
$fileListData.files += @{ "path" = $initCFile_json; "type" = "source" }; $actualSourceFilesForCompilation.Add($initCFile_json)

$projectIncludeDir_json = Join-Path -Path $projectDir -ChildPath "include" 
if (Test-Path $projectIncludeDir_json) {
    Get-ChildItem -Path $projectIncludeDir_json -Recurse -Filter "*.h" | ForEach-Object { 
        $fileListData.files += @{ "path" = ($_.FullName.Substring($projectDir.Length + 1).Replace("\", "/")); "type" = "header" }
    }
}

$fileListData | ConvertTo-Json -Depth 5 | Out-File -FilePath $file_list_json -Encoding UTF8 -Force
Write-Host "Generated $file_list_json. Total source files for compilation: $($actualSourceFilesForCompilation.Count)"


# --- Stage 4: Generate MINIMAL xinu.h and xinu_init.c for structure ---
$customStandardHeadersToSkip_gen = @("stdio.h", "stdlib.h", "string.h", "stdarg.h", "ctype.h") 
$minimalXinuHContent = @"
#ifndef _XINU_SIMULATION_H_
#define _XINU_SIMULATION_H_

// Standard C headers - MSVC should provide these from system paths
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

// Windows API
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Include project's own non-standard headers (e.g., kernel.h, process.h)
// These are included AFTER system headers.
"@
if (Test-Path $projectIncludeDir_json) { 
    Get-ChildItem -Path $projectIncludeDir_json -Recurse -Filter "*.h" | ForEach-Object {
        $headerName_gen = $_.Name 
        if (-not ($customStandardHeadersToSkip_gen -contains $headerName_gen)) {
            $relativePath_gen = $_.FullName.Substring($projectDir.Length + 1).Replace("\", "/") 
            $minimalXinuHContent += "`n#include `"$relativePath_gen`" // Project header: $headerName_gen"
        } else {
            $minimalXinuHContent += "`n/* SKIPPING project's custom standard header: $headerName_gen (using MSVC system version) */"
        }
    }
}

$minimalXinuHContent += @"

// Basic fallbacks if not defined by project headers
#ifndef PNMLEN
    #define PNMLEN 16
#endif
#ifndef NPROC
    #define NPROC 64
#endif

// Forward declarations for xinu_init.c functions or core Xinu API used by simulation
void initialize_system(void);
void kprintf(const char *format, ...);

#endif // _XINU_SIMULATION_H_
"@
$minimalXinuHContent | Out-File -FilePath $includesFile -Encoding ASCII -Force
Write-Host "Generated MINIMAL $includesFile."

$minimalXinuInitCContent = @"
#include ""xinu.h"" 

#ifndef kprintf 
void kprintf(const char *format, ...) {
    va_list ap;
    char buffer[2048]; 
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap); 
    va_end(ap);
    printf(""%s"", buffer); 
    fflush(stdout);     
}
#endif

void initialize_system(void) {
    // kprintf(""Minimal Xinu system initialization for simulation...\\n"");
}
"@
$minimalXinuInitCContent | Out-File -FilePath $xinu_init_file -Encoding ASCII -Force
Write-Host "Generated MINIMAL $xinu_init_file."

Write-Host "--- generate_xinu_includes.ps1 finished ---"