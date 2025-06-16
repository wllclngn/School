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

# --- Stage 1: Parse COMPS ---
$components = [System.Collections.Generic.List[string]]::new()
Write-Host "Attempting to parse COMPS variable using refined regex..."

# Regex explanation:
# (?sm)            - Single-line (dot matches newline) and Multi-line (^ matches start of line)
# ^\s*COMPS\s*=\s* - Matches "COMPS =" at the start of a line, tolerating whitespace.
# (                - Start of capturing group 1 (the value of COMPS)
#   (?:            -   Start of non-capturing group for a line of the value
#     (?:          -     Start of non-capturing group for content before optional line continuation
#       (?!\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)) # Negative lookahead: ensure current char is not start of end-delimiter
#       .          -       Match any character
#     )*?          -     End of content group, match zero or more times, non-greedily
#     \\\r?\n      -     Match line continuation (backslash, optional CR, newline)
#   )*             -   End of multi-line part, match zero or more continued lines
#   .*?            -   Match the last line of the value (or the only line if no continuations), non-greedily
# )                - End of capturing group 1
# (?=\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)) # Positive lookahead for the end delimiter:
#                    A newline followed by (another variable assignment OR a comment OR end of file/string),
#                    where the '=' in the next var assignment is not escaped.
$compsRegex = [regex]'(?sm)^\s*COMPS\s*=\s*((?:(?:(?!\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)).)*?\\\r?\n)*.*?(?=\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)))'
$match = $compsRegex.Match($makefileContent)

if ($match.Success) {
    $compsValueRaw = $match.Groups[1].Value.Trim()
    $cleanedCompsString = ($compsValueRaw -replace '\\\r?\n', ' ' -replace '\s+', ' ').Trim() # Replace '\' at EOL then all whitespace
    
    if ($cleanedCompsString) {
        [string[]]$componentNamesArray = $cleanedCompsString.Split(' ') | Where-Object { $_ -ne "" -and -not $_.StartsWith("#") }
        if ($componentNamesArray.Length -gt 0) {
            $components.AddRange($componentNamesArray)
            Write-Host "SUCCESS: Found components: $($components -join ', ')" -ForegroundColor Green
        } else {
             Write-Host "WARNING: COMPS regex matched, but yielded no components after filtering. Cleaned value: '$cleanedCompsString'" -ForegroundColor Yellow
        }
    } else {
        Write-Host "WARNING: COMPS regex matched, but the captured value was empty or whitespace. Raw was: `"$($compsValueRaw.Replace("`r","\r").Replace("`n","\n"))`"" -ForegroundColor Yellow
    }
} else {
    Write-Host "ERROR: Could not parse COMPS variable from Makefile using current regex." -ForegroundColor Red
}


# --- Stage 2: Parse _CFILES and _SFILES for each component ---
$sourceFilePaths = [System.Collections.Generic.List[string]]::new()

if ($components.Count -gt 0) {
    Write-Host "Attempting to parse source files for each component..."
    foreach ($componentName_outer in $components) { 
        $compMakefileVar = $componentName_outer.ToUpper() -replace '[\\/]', '_'
        Write-Host "  Component: $componentName_outer (Variable base: ${compMakefileVar})"

        foreach ($fileType_outer in @("CFILES", "SFILES")) { 
            $fileExt_outer = if ($fileType_outer -eq "CFILES") { ".c" } else { ".S" }
            $varName_outer = "${compMakefileVar}_${fileType_outer}"
            
            $filesValueString_outer = ""
            # Regex for VAR = value (captures 'content') and VAR += value (captures 'append_content')
            # This pattern is similar to the COMPS one for handling multi-line values.
            $varBlockRegexPattern = "(?sm)(?:^\s*${varName_outer}\s*=\s*(?<content>(?:(?:(?!\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)).)*?\\\r?\n)*.*?(?=\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$))))|(?:^\s*${varName_outer}\s*\+=\s*(?<append_content>(?:(?:(?!\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$)).)*?\\\r?\n)*.*?(?=\r?\n\s*(?:[A-Z_]+\s*(?<!\\)=\s*|[#]|$))))"
            $varBlockMatches = [regex]::Matches($makefileContent, $varBlockRegexPattern)
            
            $accumulatedFileValues = ""
            foreach ($varMatch in $varBlockMatches) {
                $currentBlockContent = ""
                if ($varMatch.Groups["content"].Success) {
                    $currentBlockContent = $varMatch.Groups["content"].Value.Trim()
                } elseif ($varMatch.Groups["append_content"].Success) {
                    $currentBlockContent = $varMatch.Groups["append_content"].Value.Trim()
                }
                
                if ($currentBlockContent) {
                    if ($accumulatedFileValues -ne "") { $accumulatedFileValues += " " } # Add space before appending next block
                    $accumulatedFileValues += ($currentBlockContent -replace '\\\r?\n', ' ' -replace '\s+', ' ').Trim()
                }
            }
            $filesValueString_outer = $accumulatedFileValues # Already cleaned and trimmed by parts

            if ($filesValueString_outer) {
                $foundFiles_outer = $filesValueString_outer.Split(' ') | Where-Object { $_ -ne "" -and $_.EndsWith($fileExt_outer) }
                if ($foundFiles_outer.Length -gt 0) { 
                    Write-Host "    $($fileType_outer): $($foundFiles_outer.Length) files found." -ForegroundColor Green
                    foreach ($file_inner in $foundFiles_outer) { 
                        $pathToAdd_inner = "$componentName_outer/$file_inner" 
                        if (-not $sourceFilePaths.Contains($pathToAdd_inner)) {
                            $sourceFilePaths.Add($pathToAdd_inner)
                        }
                    }
                } else {
                     Write-Host "    $($fileType_outer): No files matching extension '$($fileExt_outer)' found. (Value: '$($filesValueString_outer)')" -ForegroundColor Yellow
                }
            } else {
                Write-Host "    $($fileType_outer): Not defined or empty for $($componentName_outer)."
            }
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
            $minimalXinuHContent += "`n#include `"$relativePath_gen`" // Project header: $headerName_gen" # Added specific header name
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
// Add other essential Xinu constants/types definitions as fallbacks if they are commonly needed
// and not reliably defined in your project's core headers (like kernel.h) for the simulation.
// For example:
// #ifndef SYSERR
//     #define SYSERR (-1)
// #endif
// #ifndef OK
//     #define OK (1)
// #endif

// Forward declarations for xinu_init.c functions or core Xinu API used by simulation
void initialize_system(void);
void kprintf(const char *format, ...);
// Add other prototypes for functions defined in your Xinu C code that xinu_simulation.c might call.

#endif // _XINU_SIMULATION_H_
"@
$minimalXinuHContent | Out-File -FilePath $includesFile -Encoding ASCII -Force
Write-Host "Generated MINIMAL $includesFile."

$minimalXinuInitCContent = @"
#include ""xinu.h"" 
// Includes the simulation's master header

// Dummy kprintf if not fully available from Xinu sources yet, or for basic simulation output
#ifndef kprintf 
void kprintf(const char *format, ...) {
    va_list ap;
    char buffer[2048]; // Increased buffer size
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap); // Use vsnprintf for safety
    va_end(ap);
    printf(""%s"", buffer); // Use standard printf for simulation output
    fflush(stdout);     // Ensure output is flushed, important for debugging
}
#endif

void initialize_system(void) {
    // This is a placeholder for your Xinu's main initialization sequence.
    // For the simulation, you might call specific init functions from your Xinu code here.
    // kprintf(""Minimal Xinu system initialization for simulation...\\n"");

    // Example: Initialize process table, ready list, clock, etc., as needed by the simulation logic.
    // Ensure any global variables used here (like proctab, readylist) are declared (typically in xinu.h via kernel.h)
    // and defined (in one of your C files or in this xinu_init.c for simulation purposes).
}

// Add other minimal stubs or simulation-specific implementations if needed by xinu_simulation.c to link
// For example, if xinu_simulation.c calls create(), ready(), kill(), etc., you might need
// minimal stubs for them here if the full Xinu source isn't compiled/linked yet.
"@
$minimalXinuInitCContent | Out-File -FilePath $xinu_init_file -Encoding ASCII -Force
Write-Host "Generated MINIMAL $xinu_init_file."

Write-Host "--- generate_xinu_includes.ps1 finished ---"