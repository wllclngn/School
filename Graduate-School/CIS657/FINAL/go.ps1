# PowerShell Script for Compiling XINU Kernel Simulation using Process Isolation
# This script builds both the Windows host and the isolated XINU process

# Configuration Variables
$projectDir = $PSScriptRoot
$sim_output_dir = Join-Path -Path $projectDir -ChildPath "sim_output"
$host_executable = Join-Path -Path $sim_output_dir -ChildPath "xinu_host.exe"
$xinu_executable = Join-Path -Path $sim_output_dir -ChildPath "xinu_core.exe"
$compilationLog = Join-Path -Path $projectDir -ChildPath "compilation.txt"

# Current date and username - dynamically generated
$currentDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$currentUser = $env:USERNAME  # Get the Windows username dynamically

# Create or clear the compilation log file
if (Test-Path $compilationLog) {
    Remove-Item -Path $compilationLog -Force
}
"===== XINU Simulation Compilation Log =====" | Out-File -FilePath $compilationLog -Encoding UTF8
"Date: $currentDate" | Out-File -FilePath $compilationLog -Append -Encoding UTF8
"User: $currentUser" | Out-File -FilePath $compilationLog -Append -Encoding UTF8
"=======================================" | Out-File -FilePath $compilationLog -Append -Encoding UTF8
"" | Out-File -FilePath $compilationLog -Append -Encoding UTF8

# Function to log messages to both console and log file
function Write-Log {
    param (
        [string]$message,
        [string]$color = "White"
    )
    
    Write-Host $message -ForegroundColor $color
    $message | Out-File -FilePath $compilationLog -Append -Encoding UTF8
}

# Function to execute a command and capture its output
function Invoke-CommandLine {
    param (
        [string]$command,
        [string]$workingDirectory = $PSScriptRoot,
        [switch]$noOutput,
        [int]$errorLimit = 10,
        [switch]$interactive
    )

    Write-Log "Executing: $command" -color Yellow
    
    # For interactive programs, run directly
    if ($interactive) {
        # Log the command
        "Running interactive command: $command" | Out-File -FilePath $compilationLog -Append -Encoding UTF8
        
        # Execute directly without capturing output
        $exitCode = (Start-Process -FilePath "cmd.exe" -ArgumentList "/c $command" -NoNewWindow -Wait -WorkingDirectory $workingDirectory -PassThru).ExitCode
        return $exitCode
    }
    
    # For non-interactive programs, capture output for logging
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
        $output = @()
        
        while (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            $output += $line
            Write-Host $line
        }

        while (-not $process.StandardError.EndOfStream) {
            $line = $process.StandardError.ReadLine()
            $output += $line
            Write-Host $line -ForegroundColor Red
            if ($line -match "error") {
                $errorCount++
                if ($errorCount -ge $errorLimit) {
                    $message = "Reached error limit of $errorLimit. Stopping output..."
                    Write-Host $message -ForegroundColor Yellow
                    $output += $message
                    break
                }
            }
        }
        
        # Log all output to the file
        $output | Out-File -FilePath $compilationLog -Append -Encoding UTF8
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
        Write-Log "Cleaning simulation output directory..." -color Yellow
        Remove-Item -Path "$outputDir\*" -Force -ErrorAction SilentlyContinue
        Write-Log "Cleaned $outputDir" -color Green
    } else {
        Write-Log "Creating simulation output directory..." -color Yellow
        New-Item -Path $outputDir -ItemType Directory -Force | Out-Null
        Write-Log "Created $outputDir" -color Green
    }
}

# Build the Windows host process
function Build-HostProcess {
    param (
        [string]$sim_output_dir
    )

    Write-Log "`nBuilding Windows host process..." -color Cyan

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
        Write-Log "ERROR: Visual Studio Developer Command Prompt not found." -color Red
        Write-Log "Please install Visual Studio with C++ development tools." -color Yellow
        return $false
    }

    # Source file to compile
    $sourceFile = "xinu_host.c"
    
    # Check if source file exists
    if (-not (Test-Path (Join-Path -Path $projectDir -ChildPath $sourceFile))) {
        Write-Log "ERROR: Required source file not found: $sourceFile" -color Red
        return $false
    }
    
    # Construct compile command
    $objectFile = Join-Path -Path $sim_output_dir -ChildPath "xinu_host.obj"
    $compileCommand = "cl.exe /nologo /W3 /EHsc /c `"$projectDir\$sourceFile`" /Fo`"$objectFile`" /D_CRT_SECURE_NO_WARNINGS"

    # Construct the link command
    $link_command = "link.exe /nologo /SUBSYSTEM:CONSOLE /OUT:`"$host_executable`" `"$objectFile`" kernel32.lib user32.lib"

    # Create batch file for compilation
    $compileBatch = Join-Path $env:TEMP "xinu_host_compile.bat"

    # Batch file content
    $batchContent = @"
@echo off
echo Setting up Visual Studio environment...
call `"$vsDevCmdPath`"

echo Compiling Windows host process...
cd `"$projectDir`"
$compileCommand

echo Linking Windows host process...
$link_command

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo Compilation of host process successful!
"@

    # Write the batch file and execute it
    $batchContent | Out-File -FilePath $compileBatch -Encoding ASCII

    Write-Log "Executing compiler for host process..." -color Yellow
    $result = Invoke-CommandLine -command $compileBatch -errorLimit 10
    Remove-Item $compileBatch -ErrorAction SilentlyContinue

    # Check if compilation was successful
    if (Test-Path $host_executable) {
        Write-Log "Successfully built Windows host process at $host_executable" -color Green
        return $host_executable
    } else {
        Write-Log "ERROR: Failed to build Windows host process" -color Red
        return $false
    }
}

# Build the XINU core process
function Build-XINUProcess {
    param (
        [string]$sim_output_dir
    )

    Write-Log "`nBuilding XINU core process..." -color Cyan

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
        Write-Log "ERROR: Visual Studio Developer Command Prompt not found." -color Red
        Write-Log "Please install Visual Studio with C++ development tools." -color Yellow
        return $false
    }

    # Source file to compile
    $sourceFile = "xinu_core.c"
    
    # Check if source file exists
    if (-not (Test-Path (Join-Path -Path $projectDir -ChildPath $sourceFile))) {
        Write-Log "ERROR: Required source file not found: $sourceFile" -color Red
        return $false
    }
    
    # Source files for XINU core
    $xinuSourceFiles = @(
        "xinu_core.c",
        "system\resched.c",
        "system\ready.c",
        "system\starvation_prevention.c"
    )
    
    # Object files for linking
    $objectFiles = @()
    foreach ($file in $xinuSourceFiles) {
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($file)
        $objectFiles += "`"$sim_output_dir\$baseName.obj`""
    }
    
    # Construct compilation commands for each source file
    $compileCommands = @()
    foreach ($file in $xinuSourceFiles) {
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($file)
        $objectFile = "$sim_output_dir\$baseName.obj"
        $compileCommands += "cl.exe /nologo /W3 /EHsc /c `"$projectDir\$file`" /Fo`"$objectFile`" /D_CRT_SECURE_NO_WARNINGS /DXINU_CORE_PROCESS"
    }

    # Construct the link command
    $link_command = "link.exe /nologo /SUBSYSTEM:CONSOLE /OUT:`"$xinu_executable`" $($objectFiles -join ' ') kernel32.lib user32.lib"

    # Create batch file for compilation
    $compileBatch = Join-Path $env:TEMP "xinu_core_compile.bat"

    # Batch file content
    $batchContent = @"
@echo off
echo Setting up Visual Studio environment...
call `"$vsDevCmdPath`"

echo Compiling XINU core files...
cd `"$projectDir`"
$($compileCommands -join "`r`n")

echo Linking XINU core process...
$link_command

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo Compilation of XINU core process successful!
"@

    # Write the batch file and execute it
    $batchContent | Out-File -FilePath $compileBatch -Encoding ASCII

    Write-Log "Executing compiler for XINU core..." -color Yellow
    $result = Invoke-CommandLine -command $compileBatch -errorLimit 10
    Remove-Item $compileBatch -ErrorAction SilentlyContinue

    # Check if compilation was successful
    if (Test-Path $xinu_executable) {
        Write-Log "Successfully built XINU core process at $xinu_executable" -color Green
        return $xinu_executable
    } else {
        Write-Log "ERROR: Failed to build XINU core process" -color Red
        return $false
    }
}

# Run the simulation
function Run-XINUSimulation {
    param (
        [string]$hostExecutable,
        [string]$xinuExecutable
    )

    Write-Log "`nRunning XINU kernel simulation..." -color Cyan

    if (-not (Test-Path $hostExecutable)) {
        Write-Log "ERROR: Host executable not found at $hostExecutable" -color Red
        return $false
    }

    if (-not (Test-Path $xinuExecutable)) {
        Write-Log "ERROR: XINU core executable not found at $xinuExecutable" -color Red
        return $false
    }

    # Run the host application with the current username as parameter
    # and the path to the XINU core executable
    Write-Log "Starting XINU simulation..." -color Yellow
    Write-Log "Command: `"$hostExecutable`" `"$xinuExecutable`" $currentUser" -color Yellow
    
    # Use interactive mode for the simulation to allow direct console interaction
    Invoke-CommandLine -command "`"$hostExecutable`" `"$xinuExecutable`" $currentUser" -interactive
    
    Write-Log "`nSimulation completed!" -color Green
    return $true
}

# Main script execution
try {
    # Welcome message
    Write-Log "====================================================" -color Cyan
    Write-Log "=== XINU Kernel Simulation Build Script ===" -color Cyan
    Write-Log "====================================================" -color Cyan
    Write-Log "Current Date and Time (UTC): $currentDate" -color White
    Write-Log "Current User's Login: $currentUser" -color White
    Write-Log "====================================================" -color Cyan
    
    # Step 1: Clean output directory
    Clean-SimulationOutput -outputDir $sim_output_dir
    
    # Step 2: Build host process
    $hostExe = Build-HostProcess -sim_output_dir $sim_output_dir
    if ($hostExe -eq $false) {
        Write-Log "Failed to build host process. Exiting." -color Red
        exit 1
    }
    
    # Step 3: Build XINU process
    $xinuExe = Build-XINUProcess -sim_output_dir $sim_output_dir
    if ($xinuExe -eq $false) {
        Write-Log "Failed to build XINU core process. Exiting." -color Red
        exit 1
    }

    # Step 4: Run the simulation
    $success = Run-XINUSimulation -hostExecutable $hostExe -xinuExecutable $xinuExe
    if (-not $success) {
        Write-Log "Failed to run the simulation. Exiting." -color Red
        exit 1
    }

    # Final message
    Write-Log "`nXINU kernel simulation completed successfully!" -color Green
    Write-Log "`nTo run the simulation again:" -color Cyan
    Write-Log "`"$hostExe`" `"$xinuExe`" $currentUser" -color White
    Write-Log "`nCompilation log saved to: $compilationLog" -color White
}
catch {
    Write-Log "An error occurred:" -color Red
    Write-Log $_ -color Red
    "ERROR: $($_.Exception.Message)" | Out-File -FilePath $compilationLog -Append -Encoding UTF8
    exit 1
}