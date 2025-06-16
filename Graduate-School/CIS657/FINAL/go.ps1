# PowerShell Script for Compiling XINU Kernel Simulation
# This script builds and runs the XINU simulation using actual XINU source files

# Configuration Variables
$projectDir = $PSScriptRoot
$sim_output_dir = Join-Path -Path $projectDir -ChildPath "sim_output"
$executable_name = Join-Path -Path $sim_output_dir -ChildPath "xinu_simulation.exe"
$generateIncludesScript = Join-Path -Path $projectDir -ChildPath "generate_xinu_includes.ps1"

# Current date and username - get them dynamically
$currentDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$currentUser = $env:USERNAME

# Function to execute a command and capture its output with error limit
function Invoke-CommandLine {
    param (
        [string]$command,
        [string]$workingDirectory = $PSScriptRoot,
        [switch]$noOutput,
        [int]$errorLimit = 10
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
        # Read output without infinite loop problems
        $errorCount = 0
        while (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            Write-Host $line
        }

        while (-not $process.StandardError.EndOfStream) {
            $line = $process.StandardError.ReadLine()
            Write-Host $line -ForegroundColor Red
            # Count errors and stop after limit
            if ($line -match "error") {
                $errorCount++
                if ($errorCount -ge $errorLimit) {
                    Write-Host "Reached error limit of $errorLimit. Stopping output..." -ForegroundColor Yellow
                    break
                }
            }
        }
    }

    $process.WaitForExit()
    return $process.ExitCode
}

# Clean the simulation output directory
function Clean-SimulationOutput {
    param (
        [string]$outputDir
    )
    
    if (Test-Path $outputDir) {
        Write-Host "Cleaning simulation output directory..." -ForegroundColor Yellow
        Remove-Item -Path "$outputDir\*" -Force -ErrorAction SilentlyContinue
        Write-Host "Cleaned $outputDir" -ForegroundColor Green
    } else {
        Write-Host "Creating simulation output directory..." -ForegroundColor Yellow
        New-Item -Path $outputDir -ItemType Directory -Force | Out-Null
        Write-Host "Created $outputDir" -ForegroundColor Green
    }
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

    # Clean the simulation output directory
    Clean-SimulationOutput -outputDir $sim_output_dir

    # Generate xinu_includes.h
    Write-Host "Generating xinu_includes.h using $generateIncludesScript..." -ForegroundColor Green
    try {
        & $generateIncludesScript
        Write-Host "Successfully generated xinu_includes.h" -ForegroundColor Green
    } catch {
        Write-Host "ERROR: Failed to generate xinu_includes.h: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }

    # Source files to compile
    $sourceFiles = @(
        "xinu_simulation.c",
        "system\starvation_prevention.c"
    )
    
    # Check if source files exist
    $missingFiles = @()
    foreach ($file in $sourceFiles) {
        if (-not (Test-Path (Join-Path -Path $projectDir -ChildPath $file))) {
            $missingFiles += $file
        }
    }
    
    if ($missingFiles.Count -gt 0) {
        Write-Host "ERROR: Required source files not found:" -ForegroundColor Red
        foreach ($file in $missingFiles) {
            Write-Host "  - $file" -ForegroundColor Red
        }
        return $false
    }
    
    # Construct compile commands for each source file
    $compileCommands = @()
    foreach ($sourceFile in $sourceFiles) {
        $objectFile = Join-Path -Path $sim_output_dir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($sourceFile)).obj"
        $compileCommands += "cl.exe /nologo /W3 /EHsc /c `"$projectDir\$sourceFile`" /Fo`"$objectFile`" /I`"$projectDir`" /I`"$projectDir\include`" /D_CRT_SECURE_NO_WARNINGS /DSIMULATION"
    }

    # Construct the link command
    $objectFilePaths = @()
    foreach ($sourceFile in $sourceFiles) {
        $objName = [System.IO.Path]::GetFileNameWithoutExtension($sourceFile)
        $objectFilePaths += "`"$(Join-Path -Path $sim_output_dir -ChildPath "$objName.obj")`""
    }
    $link_command = "link.exe /nologo /SUBSYSTEM:CONSOLE /OUT:`"$executable_name`" $($objectFilePaths -join ' ') kernel32.lib user32.lib"

    # Create batch file for compilation
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
    $result = Invoke-CommandLine -command $compileBatch -errorLimit 10
    Remove-Item $compileBatch -ErrorAction SilentlyContinue

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

    # Run the simulation with the current username as parameter
    Write-Host "Starting XINU kernel simulation..." -ForegroundColor Yellow
    Invoke-CommandLine -command "`"$executable`" $currentUser"
    Write-Host "`nSimulation completed!" -ForegroundColor Green
    return $true
}

# Main script execution
try {
    # Welcome message
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "=== XINU Kernel Simulation Build Script ===" -ForegroundColor Cyan
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "Current Date and Time (UTC): $currentDate" -ForegroundColor White
    Write-Host "Current User's Login: $currentUser" -ForegroundColor White
    Write-Host "====================================================" -ForegroundColor Cyan
    
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
    Write-Host "`"$executable`" $currentUser" -ForegroundColor White
}
catch {
    Write-Host "An error occurred:" -ForegroundColor Red
    Write-Host $_ -ForegroundColor Red
    exit 1
}