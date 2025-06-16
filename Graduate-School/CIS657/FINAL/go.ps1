# PowerShell Script for Compiling XINU Kernel Simulation from JSON file list

# Configuration Variables
$projectDir = $PSScriptRoot
$sim_output_dir = Join-Path -Path $projectDir -ChildPath "sim_output"
$jsonFile = Join-Path -Path $projectDir -ChildPath "file_list.json"  # Path to your JSON file

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

    # Load source files from JSON
    try {
        $jsonContent = Get-Content -Path $jsonFile -Raw | ConvertFrom-Json
        $source_files = $jsonContent.files | Where-Object { $_.path -notlike "*include\\*" -and $_.type -eq "source" -and $_.path -notlike "pstarv_test.c" } | ForEach-Object { $_.path }
        if (-not $source_files) {
            Write-Host "ERROR: No source files found in JSON or all files were filtered out." -ForegroundColor Red
            return $false
        }
    }
    catch {
        Write-Host "ERROR: Failed to load or parse JSON file: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }

    # Construct the paths to the source files relative to the project directory
    $full_source_paths = $source_files | ForEach-Object { Join-Path -Path $projectDir -ChildPath $_ }

    # Construct compile commands for each source file
    $compile_commands = foreach ($source_file in $full_source_paths) {
        $object_file = Join-Path -Path $sim_output_dir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($source_file)).obj"
        "cl.exe /nologo /W4 /EHsc /c `"$source_file`" /Fo`"$object_file`" /I.`" /I.\system`" /I.\shell`" /I.\config`" /I.\device\tty`" /I.\lib\libxc`" /D_CRT_SECURE_NO_WARNINGS"
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