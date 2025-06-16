# PowerShell script to find vcvars64.bat dynamically

# Define potential Visual Studio installation directories
$VSInstallPaths = @(
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2017\BuildTools"
)

# Define the relative path to vcvars64.bat
$vcvarsPath = "VC\Auxiliary\Build\vcvars64.bat"

# Function to test if a path exists
function Test-PathSafe {
  param (
    [string]$Path
  )
  try {
    return Test-Path -Path $Path -PathType Leaf -ErrorAction Stop
  }
  catch {
    return $false
  }
}

# Iterate through potential installation paths and find vcvars64.bat
foreach ($InstallPath in $VSInstallPaths) {
    $FullPath = Join-Path -Path $InstallPath -ChildPath $vcvarsPath
    if (Test-PathSafe -Path $FullPath) {
        Write-Host "Found vcvars64.bat at: $FullPath"
        # You can now use $FullPath in your script
        # For example, to execute the batch file:
        # & "$FullPath"
        break
    }
}

if (-not $FullPath) {
    Write-Host "vcvars64.bat not found in any of the expected locations."
    # Attempt to find vcvarsall.bat as a fallback
    $vcvarsAllPath = "VC\Auxiliary\Build\vcvarsall.bat"
    foreach ($InstallPath in $VSInstallPaths) {
        $FullPath = Join-Path -Path $InstallPath -ChildPath $vcvarsAllPath
        if (Test-PathSafe -Path $FullPath) {
            Write-Host "Found vcvarsall.bat at: $FullPath"
            # Consider using vcvarsall.bat with appropriate arguments
            Write-Host "Please use vcvarsall.bat with the appropriate architecture argument (e.g., vcvarsall.bat x64)"
            break
        }
    }
    if (-not $FullPath) {
        Write-Host "vcvarsall.bat also not found. Please ensure Visual Studio Build Tools are installed correctly."
    }
}