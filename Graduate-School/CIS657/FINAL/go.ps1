# PowerShell Script to Compile the XINU C Project
# Requires Visual Studio Build Tools

# Function to check if a command exists
function Is-CommandAvailable {
    param ([string]$Command)
    try { $null = & where.exe $Command 2>$null; return $true } catch { return $false }
}

# Function to execute a command within the Visual Studio Developer Command Prompt
function Execute-Command-In-DevCmdPrompt {
    param (
        [string]$DevCmdPromptPath,
        [string]$Command
    )
    Write-Host "Executing (in Dev Cmd Prompt): $Command" -ForegroundColor Green

    # Invoke VsDevCmd.bat directly in PowerShell
    Write-Host "Invoking VsDevCmd.bat..." -ForegroundColor DarkYellow
    Invoke-Expression "& `"$DevCmdPromptPath`""

    # Execute the compilation command directly in PowerShell
    Write-Host "Executing compilation command: $Command" -ForegroundColor DarkYellow
    try {
        # Split the command into executable and arguments
        $commandParts = $Command -split " ", 2
        $executable = $commandParts[0]
        $arguments = $commandParts[1] -split " "

        # Execute the command
        & $executable @arguments
        $exitCode = $LASTEXITCODE
    }
    catch {
        Write-Host "ERROR: Compilation failed: $($_.Exception.Message)" -ForegroundColor Red
        $exitCode = 1
    }

    if ($exitCode -ne 0) {
        Write-Host "ERROR: Command failed with exit code $exitCode" -ForegroundColor Red
    } else {
        Write-Host "Command executed successfully." -ForegroundColor Green
    }
    return $exitCode
}

# --- Script Start ---
Write-Host "Starting the build process for XINU C project..." -ForegroundColor Cyan

# --- MSVC Detection ---
$devCmdPromptSearchPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    # Add more common paths if needed
)
$selectedDevCmdPrompt = ""
foreach ($path in $devCmdPromptSearchPaths) {
    if (Test-Path $path) {
        $selectedDevCmdPrompt = $path
        Write-Host "Found VsDevCmd.bat at: $selectedDevCmdPrompt" -ForegroundColor Green
        break
    }
}

$useMSVC = $false
if ($selectedDevCmdPrompt) {
    # Check if cl.exe is available AFTER running VsDevCmd.bat
    $testCommand = "cl.exe"
    # We are now invoking VsDevCmd.bat directly, so we don't need to wrap cl.exe in another cmd /c call
    $testResult = Execute-Command-In-DevCmdPrompt -DevCmdPromptPath $selectedDevCmdPrompt -Command $testCommand
    if ($testResult -eq 0) {
        $useMSVC = $true
        Write-Host "cl.exe found in environment after running VsDevCmd.bat." -ForegroundColor Green
    } else {
        Write-Host "WARNING: cl.exe not found even after running VsDevCmd.bat. MSVC build will be skipped." -ForegroundColor Yellow
    }
}

if ($useMSVC) {
    Write-Host "Attempting to build with MSVC (cl.exe)..." -ForegroundColor Cyan

    # --- Compilation ---
    $sourceFiles = "pstarv_test.c system\resched.c system\insert.c shell\starvation_shell.c"  # List your C source files
    $outputExe = "pstarv_test.exe"
    # Add include directory for pthread if needed
    $compileCommand = "cl.exe /EHsc /MD $sourceFiles /I. /Fepstarv_test.exe" # Your compilation command

    Write-Host "Compiling with command: $compileCommand"
    $result = Execute-Command-In-DevCmdPrompt -DevCmdPromptPath $selectedDevCmdPrompt -Command $compileCommand

    if ($result -eq 0) {
        Write-Host "Compilation successful!" -ForegroundColor Green
    } else {
        Write-Host "Compilation failed!" -ForegroundColor Red
        exit 1
    }
    # --- End Compilation ---
} else {
    Write-Host "MSVC (cl.exe) not found. Please install Visual Studio Build Tools." -ForegroundColor Red
    exit 1
}

Write-Host "Build process completed." -ForegroundColor Cyan
exit 0