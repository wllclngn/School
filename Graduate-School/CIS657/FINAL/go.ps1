# PowerShell Script for Compiling XINU Kernel Simulation (Full Project)

# Configuration Variables
$projectDir = $PSScriptRoot
$sim_output_dir = Join-Path -Path $projectDir -ChildPath "sim_output"
$sourceFile = Join-Path -Path $projectDir -ChildPath "xinu_main.c"  # Path to the xinu_main.c file
$executable_name = Join-Path -Path $sim_output_dir -ChildPath "xinu_simulation.exe"
$xinuIncludesHeader = Join-Path -Path $projectDir -ChildPath "xinu_includes.h" # Path to generated includes file

# Path to the script that generates xinu_includes.h (replace with your script)
$generateIncludesScript = Join-Path -Path $projectDir -ChildPath "generate_xinu_includes.ps1"

# Generate a unique identifier
$uniqueId = New-Guid

# Print the unique identifier
Write-Host "Script execution ID: $uniqueId" -ForegroundColor Green

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

    # Static variable to track execution
    static [bool]$alreadyExecuted = $false

    # Check if already executed
    if ($alreadyExecuted) {
        Write-Host "Build-XINUSimulation already executed. Skipping." -ForegroundColor Yellow
        return $false
    }

    # Set the flag to indicate execution
    $alreadyExecuted = $true

    Write-Host "`nBuilding XINU kernel simulation..." -ForegroundColor Cyan

    # Initialize error flag
    $buildError = $false

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
        $buildError = $true
    }

    # Create output directory if it doesn't exist
    if (-not $buildError) {
        if (-not (Test-Path $sim_output_dir)) {
            New-Item -Path $sim_output_dir -ItemType Directory -Force | Out-Null
        } else {
            # Verify that the output directory is not the root directory or a directory containing important files
            if ($sim_output_dir -eq $PSScriptRoot -or $sim_output_dir -eq "\") {
                Write-Host "ERROR: Output directory cannot be the root directory or the project directory." -ForegroundColor Red
                $buildError = $true
            } else {
                # Clean the directory (commented out to prevent accidental deletion)
                # Remove-Item -Path "$sim_output_dir\*" -Force -ErrorAction SilentlyContinue
                Write-Host "Cleaning output directory is disabled to prevent accidental data loss." -ForegroundColor Yellow
            }
        }
    }

    # Generate xinu_includes.h
    if (-not $buildError) {
        Write-Host "Generating xinu_includes.h using $generateIncludesScript..." -ForegroundColor Green
        try {
            & $generateIncludesScript
            Write-Host "Successfully generated xinu_includes.h" -ForegroundColor Green
        } catch {
            Write-Host "ERROR: Failed to generate xinu_includes.h: $($_.Exception.Message)" -ForegroundColor Red
            $buildError = $true
        }
    }

    # Get all source files from file_list.json
    if (-not $buildError) {
        try {
            $jsonContent = Get-Content -Path "$projectDir\file_list.json" -Raw | ConvertFrom-Json
            $sourceFiles = $jsonContent.files | Where-Object { $_.type -eq "source" } | ForEach-Object { $_.path }
            if (-not $sourceFiles) {
                Write-Host "ERROR: No source files found in file_list.json" -ForegroundColor Red
                $buildError = $true
            }
        } catch {
            Write-Host "ERROR: Failed to load or parse file_list.json: $($_.Exception.Message)" -ForegroundColor Red
            $buildError = $true
        }
    }

    # Construct compile commands for each source file
    if (-not $buildError) {
        $compileCommands = foreach ($sourceFile in $sourceFiles) {
            $objectFile = Join-Path -Path $sim_output_dir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($sourceFile)).obj"
            "cl.exe /nologo /W4 /EHsc /c `"$projectDir\$sourceFile`" /Fo`"$objectFile`" /I.`" /Iinclude /D_CRT_SECURE_NO_WARNINGS"
        }

        # Construct the link command
        $objectFiles = Get-ChildItem -Path $sim_output_dir -Filter "*.obj" | ForEach-Object { $_.FullName }
        $link_command = "link.exe /nologo /SUBSYSTEM:CONSOLE /OUT:`"$executable_name`" $($objectFiles -join ' ') kernel32.lib user32.lib"

        $compileBatch = Join-Path $env:TEMP "xinu_compile.bat"

        # Batch file content
        $batchContent = @"
@echo off
echo Setting up Visual Studio environment...
call `"$vsDevCmdPath`"

echo Compiling XINU kernel files...
cd `"$projectDir`"
$($compileCommands -join "`r`n")
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
        } else {
            Write-Host "ERROR: Failed to build XINU kernel simulation" -ForegroundColor Red
            $buildError = $true
        }
    }

    if ($buildError) {
        Write-Host "Build failed. Aborting." -ForegroundColor Red
        return $false
    }

    return $executable_name
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
    $currentDate = Get-Date -Format "yyyy-MM-DD HH:MM:SS"
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