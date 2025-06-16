# PowerShell Script for Compiling and Running XINU Kernel Simulation

# Configuration Variables
$projectDir = $PSScriptRoot
$sim_output_dir = Join-Path -Path $projectDir -ChildPath "sim_output"

# Define XINU kernel source files - ADJUST THESE BASED ON YOUR REPO
$source_files = @(
    "system\main.c",          # Kernel entry point
    "system\initialize.c",    # System initialization
    "system\create.c",        # Process creation
    "system\kill.c",          # Process termination
    "system\resume.c",        # Process resume
    "system\sleep.c",         # Process sleep
    "system\kprintf.c",       # Kernel printf
    "system\queue.c",        # Queue management
    "system\ready.c",        # Ready list management
    "system\chprio.c",       # Change priority
    "system\getitem.c",      # Get item from queue
    "system\insert.c",       # Insert item into queue
    "system\receive.c",      # Message receive
    "system\recvclr.c",      # Clear message
    "shell\xinu_main.c",       # Simulation entry point
    "shell\starvation_shell.c",  # Starvation shell command
    "shell\pstarv_globals.c",   # Starvation globals
    "shell\pstarv_process.c",   # Starvation process
    "shell\p2_process.c",      # Process 2
    "shell\p1_process.c"       # Process 1
    # Add device driver files if needed (e.g., TTY)
    # Add other necessary XINU kernel files here
)

$executable_name = Join-Path -Path $sim_output_dir -ChildPath "xinu_simulation.exe"

# Function to execute a command and capture its output
function Invoke-CommandLine {
    param (
        [string]$command,
        [string]$workingDirectory = $PSScriptRoot,
        [switch]$noOutput
    )

    Write-Host "Executing: $command" -ForegroundColor Yellow

    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = "cmd.exe"
    $processInfo.Arguments = "/c $command"
    $processInfo.RedirectStandardError = $true
    $processInfo.RedirectStandardOutput = $true
    $processInfo.UseShellExecute = $false
    $processInfo.WorkingDirectory = $workingDirectory

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $processInfo
    $process.Start() | Out-Null

    if (-not $noOutput) {
        while (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            Write-Host $line
        }

        while (-not $process.StandardError.EndOfStream) {
            $line = $process.StandardError.ReadLine()
            Write-Host $line -ForegroundColor Red
        }
    }

    $process.WaitForExit()
    return $process.ExitCode
}

# Build the simulation
function Build-XINUSimulation {
    param (
        [string]$sim_output_dir
    )

    Write-Host "`nBuilding XINU kernel simulation..." -ForegroundColor Cyan

    # Find Visual Studio
    $vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsDevCmdPath = $null

    if (Test-Path $vsWherePath) {
        $vsPath = & $vsWherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            $vsDevCmdPath = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"
        }
    }

    if (-not $vsDevCmdPath -or -not (Test-Path $vsDevCmdPath)) {
        Write-Host "ERROR: Visual Studio Developer Command Prompt not found." -ForegroundColor Red
        Write-Host "Please install Visual Studio with C++ development tools." -ForegroundColor Yellow
        return $false
    }

    # Create output directory if it doesn't exist
    if (-not (Test-Path $sim_output_dir)) {
        New-Item -Path $sim_output_dir -ItemType Directory -Force | Out-Null
    } else {
        # Clean the directory
        Remove-Item -Path "$sim_output_dir\*" -Force -ErrorAction SilentlyContinue
    }

    # Construct the paths to the source files relative to the project directory
    $full_source_paths = $source_files | ForEach-Object { Join-Path -Path $projectDir -ChildPath $_ }

    # Define include paths
    $include_paths = @(
        "`"$projectDir`"",
        "`"$projectDir\system`"",
        "`"$projectDir\shell`""
    ) -join ";"

    # Set the INCLUDE environment variable
    $env:INCLUDE = $include_paths

    # Construct compile commands for each source file
    $compile_commands = foreach ($source_file in $full_source_paths) {
        $object_file = Join-Path -Path $sim_output_dir -ChildPath "$($_.BaseName).obj"
        "cl.exe /nologo /W4 /EHsc /c `"$source_file`" /Fo`"$object_file`""
    }

    # Construct the link command
    $object_files = Get-ChildItem -Path $sim_output_dir -Filter "*.obj" | ForEach-Object { $_.FullName }
    $link_command = "link.exe /nologo /SUBSYSTEM:CONSOLE /OUT:`"$executable_name`" $($object_files -join ' ') kernel32.lib user32.lib"

    $compileBatch = Join-Path $env:TEMP "xinu_compile.bat"

    # Batch file content
    $batchContent = @"
@echo off
echo Setting up Visual Studio environment...
call `"$vsDevCmdPath`"
echo Compiling XINU kernel files...
cd `"$projectDir`"
set INCLUDE=$include_paths
$($compile_commands -join "`r`n")
echo Linking XINU simulation...
$link_command
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo Compilation successful!
"@

    # Write the batch file and execute it
    $batchContent | Out-File -FilePath $compileBatch -Encoding ASCII

    Write-Host "Executing compiler with your files..." -ForegroundColor Yellow
    $result = Invoke-CommandLine $compileBatch
    Remove-Item $compileBatch

    # Check if compilation was successful
    if (Test-Path $executable_name) {
        Write-Host "Successfully built XINU kernel simulation at $executable_name" -ForegroundColor Green
        return $executable_name
    } else {
        Write-Host "ERROR: Failed to build XINU kernel simulation" -ForegroundColor Red
        return $false
    }
}

# Run the simulation
function Run-XINUSimulation {
    param (
        [string]$executable
    )

    Write-Host "`nRunning XINU kernel simulation..." -ForegroundColor Cyan

    if (-not (Test-Path $executable)) {
        Write-Host "ERROR: Executable not found at $executable" -ForegroundColor Red
        return $false
    }

    # Run the simulation
    Write-Host "Starting XINU kernel simulation..." -ForegroundColor Yellow
    Invoke-CommandLine $executable
    Write-Host "`nSimulation completed!" -ForegroundColor Green
    return $true
}

# Main script execution
try {
    $currentDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $currentUser = $env:USERNAME

    # Welcome message
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "=== XINU Kernel Simulation Build Script ===" -ForegroundColor Cyan
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "Current Date and Time (UTC): $currentDate" -ForegroundColor White
    Write-Host "Current User's Login: $currentUser" -ForegroundColor White
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "This script compiles and runs a simulation of the XINU kernel." -ForegroundColor Green

    # Step 1: Build the simulation
    $executable = Build-XINUSimulation -sim_output_dir $sim_output_dir
    if ($executable -eq $false) {
        exit 1
    }

    # Step 2: Run the simulation
    $success = Run-XINUSimulation -executable $executable
    if (-not $success) {
        exit 1
    }

    # Final message
    Write-Host "`nXINU kernel simulation completed successfully!" -ForegroundColor Green
    Write-Host "`nTo run the simulation again:" -ForegroundColor Cyan
    Write-Host "$executable" -ForegroundColor White
}
catch {
    Write-Host "An error occurred:" -ForegroundColor Red
    Write-Host $_ -ForegroundColor Red
    exit 1
}