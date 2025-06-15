# PowerShell script to compile pstarv_test.c with Microsoft C++

# Set the path to your cl.exe compiler (adjust as needed)
# This assumes you have the Visual Studio Build Tools installed
# and the environment variables are set up correctly.
$clPath = "cl.exe"

# Set the output executable name
$outputName = "pstarv_test.exe"

# Set the source files to compile
$sourceFiles = @(
    "pstarv_test.c",
    "Graduate-School\CIS657\FINAL\system\resched.c",
    "Graduate-School\CIS657\FINAL\system\insert.c",
    "Graduate-School\CIS657\FINAL\shell\starvation_shell.c"
)

# Build the compilation command
# /EHsc enables exception handling
# /MD uses the multithreaded DLL version of the runtime library
$command = "$clPath /EHsc /MD $($sourceFiles -join ' ') /Fe$outputName"

# Output the command for debugging
Write-Host "Compiling with command: $command"

# Execute the compilation command
try {
    Invoke-Expression $command
    Write-Host "Compilation successful! Executable: $outputName"
}
catch {
    Write-Host "Compilation failed!"
    Write-Host $_.Exception.Message
    exit 1
}

# Link with the pthread library (if needed)
# if (-not (Test-Path "pthreadVC2.lib")) {
#     Write-Host "pthreadVC2.lib not found.  You may need to download and place it in the project directory."
# } else {
#     Write-Host "Linking with pthreadVC2.lib"
#     $linkCommand = "link.exe /OUT:$outputName $outputName pthreadVC2.lib"
#     Invoke-Expression $linkCommand
# }

# Optional: Run the executable after successful compilation
# Write-Host "Running the executable..."
# .\$outputName