# PowerShell script to recursively list files in a directory and output to JSON

# Configuration
$rootDir = $PSScriptRoot # You can change this to any directory
$jsonFile = Join-Path -Path $PSScriptRoot -ChildPath "file_list.json"
$currentDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$currentUser = $env:USERNAME

Write-Host "Current Date and Time (UTC - YYYY-MM-DD HH:MM:SS formatted): $currentDate"
Write-Host "Current User's Login: $currentUser"

# Function to determine file type
function Get-FileType {
    param (
        [string]$filePath
    )
    $extension = [System.IO.Path]::GetExtension($filePath)
    switch ($extension) {
        ".c" { return "source" }
        ".h" { return "header" }
        ".ps1" { return "powershell" }
        ".txt" { return "text" }
        ".md" { return "markdown" }
        default { return "file" }
    }
}

# Get all files recursively
$files = Get-ChildItem -Path $rootDir -Recurse -File

# Create an array to store file information
$fileList = @()

# Iterate through each file and create an object
foreach ($file in $files) {
    $filePath = $file.FullName.Substring($rootDir.Length + 1) # Relative path
    $fileType = Get-FileType -filePath $file.FullName
    $fileList += [PSCustomObject]@{
        path = $filePath
        type = $fileType
    }
}

# Create the JSON object
$jsonObject = @{
    files = $fileList
}

# Convert to JSON and save to file
$jsonObject | ConvertTo-Json -Depth 10 | Out-File -FilePath $jsonFile -Encoding UTF8

Write-Host "File list generated and saved to: $jsonFile" -ForegroundColor Green