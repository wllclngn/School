# Use direct paths instead of Join-Path to avoid ChildPath issues
$projectDir = $PSScriptRoot
$sim_output_dir = "$projectDir\sim_output"
$executable_name = "$sim_output_dir\xinu_simulation.exe"

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

function Generate-XinuIncludes {
    $generateScript = "$projectDir\generate_xinu_includes.ps1"
    
    if (-not (Test-Path $generateScript)) {
        Write-Host "ERROR: generate_xinu_includes.ps1 not found at $generateScript" -ForegroundColor Red
        return $false
    }
    
    Write-Host "Generating XINU includes and initialization code..." -ForegroundColor Cyan
    & $generateScript
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to generate XINU includes and initialization code" -ForegroundColor Red
        return $false
    }
    
    return $true
}

function Build-XINUSimulation {
    param (
        [string]$sim_output_dir
    )

    Write-Host "`nBuilding XINU kernel simulation..." -ForegroundColor Cyan

    $vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsDevCmdPath = $null

    if (Test-Path $vsWherePath) {
        $vsPath = & $vsWherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            $vsDevCmdPath = "$vsPath\Common7\Tools\VsDevCmd.bat"
        }
    }

    if (-not $vsDevCmdPath -or -not (Test-Path $vsDevCmdPath)) {
        Write-Host "ERROR: Visual Studio Developer Command Prompt not found." -ForegroundColor Red
        Write-Host "Please install Visual Studio with C++ development tools." -ForegroundColor Yellow
        return $false
    }

    if (-not (Test-Path $sim_output_dir)) {
        New-Item -Path $sim_output_dir -ItemType Directory -Force | Out-Null
    }

    # Generate XINU includes and initialization code
    if (-not (Generate-XinuIncludes)) {
        return $false
    }

    # Get source files from file_list.json
    try {
        $jsonContent = Get-Content -Path "$projectDir\file_list.json" -Raw | ConvertFrom-Json
        $sourceFiles = $jsonContent.files | Where-Object { $_.type -eq "source" } | ForEach-Object { $_.path }
        if (-not $sourceFiles) {
            Write-Host "ERROR: No source files found in file_list.json" -ForegroundColor Red
            return $false
        }
        
        Write-Host "Found $($sourceFiles.Count) source files in file_list.json" -ForegroundColor Yellow
    } catch {
        Write-Host "ERROR: Failed to load or parse file_list.json: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }

    # Construct compile commands for each source file
    $compileCommands = foreach ($sourceFile in $sourceFiles) {
        $objectName = [System.IO.Path]::GetFileNameWithoutExtension($sourceFile) + ".obj"
        $objectFile = "$sim_output_dir\$objectName"
        "cl.exe /nologo /W4 /EHsc /c `"$projectDir\$sourceFile`" /Fo`"$objectFile`" /I`"$projectDir`" /I`"$projectDir\include`" /D_CRT_SECURE_NO_WARNINGS"
    }

    # Write each command to a temporary batch file
    $compileBatchName = "xinu_compile.bat"
    $compileBatch = "$env:TEMP\$compileBatchName"
    
    # Get all object files that will be created
    $objectFiles = $sourceFiles | ForEach-Object { 
        $objectName = [System.IO.Path]::GetFileNameWithoutExtension($_) + ".obj"
        "$sim_output_dir\$objectName"
    }

    # Construct the link command
    $link_command = "link.exe /nologo /SUBSYSTEM:CONSOLE /OUT:`"$executable_name`" $($objectFiles -join ' ') kernel32.lib user32.lib"

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

    Write-Host "Compiling XINU simulation..." -ForegroundColor Yellow
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

function Run-XINUSimulation {
    param (
        [string]$executable,
        [string]$username
    )

    Write-Host "`nRunning XINU kernel simulation..." -ForegroundColor Cyan

    if (-not (Test-Path $executable)) {
        Write-Host "ERROR: Executable not found at $executable" -ForegroundColor Red
        return $false
    }

    # Run the simulation with username parameter
    Write-Host "Starting XINU kernel simulation... (User: $username)" -ForegroundColor Yellow
    Invoke-CommandLine "$executable $username"
    Write-Host "`nSimulation completed!" -ForegroundColor Green
    return $true
}

# Main script execution
try {
    # Dynamically get the current Windows username
    $username = $env:USERNAME
    
    # Get current UTC date and time
    $current_time_utc = (Get-Date).ToUniversalTime() | Get-Date -Format "yyyy-MM-dd HH:mm:ss"

    # Welcome message
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "=== XINU Kernel Simulation Build Script ===" -ForegroundColor Cyan
    Write-Host "User: $username" -ForegroundColor White
    Write-Host "Date: $current_time_utc UTC" -ForegroundColor White
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "This script compiles and runs a simulation of the XINU kernel." -ForegroundColor Green

    # Step 1: Build the simulation
    $executable = Build-XINUSimulation -sim_output_dir $sim_output_dir
    if ($executable -eq $false) {
        exit 1
    }

    # Step 2: Run the simulation with username
    # The $username variable is already set from $env:USERNAME
    $success = Run-XINUSimulation -executable $executable -username $username
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