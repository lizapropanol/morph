param(
    [string]$QtPath = "C:/Qt/6.8.0/msvc2022_64",
    [string]$OpenSSLPath = "C:/Qt/Tools/OpenSSLv3/Win_x64",
    [string]$BuildDir = "build",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  morph - Windows Installer Builder      " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$ISCC = Get-Command iscc -ErrorAction SilentlyContinue
if (-not $ISCC) {
    $PotentialPaths = @(
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    )
    foreach ($p in $PotentialPaths) {
        if (Test-Path $p) {
            $ISCC = $p
            break
        }
    }
}

if (-not $ISCC) {
    Write-Warning "Inno Setup (ISCC.exe) was not found."
    Write-Host "You can install it easily with: winget install JRSoftware.InnoSetup" -ForegroundColor Yellow
}

Write-Host "`n[1/2] Configuring CMake..." -ForegroundColor Green
$CMakeArgs = @(
    "-B", $BuildDir,
    "-S", $ProjectRoot,
    "-DCMAKE_BUILD_TYPE=$Config"
)

if (Test-Path $QtPath) {
    $PrefixPaths = @($QtPath)
    if (Test-Path $OpenSSLPath) {
        $PrefixPaths += $OpenSSLPath
    }
    $CMakeArgs += "-DCMAKE_PREFIX_PATH=$($PrefixPaths -join ';')"
}

& cmake @CMakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
}

Write-Host "`n[2/2] Building project and creating installer..." -ForegroundColor Green
& cmake --build $BuildDir --config $Config

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
}

$SetupFile = Get-ChildItem -Path "$ProjectRoot\$BuildDir" -Filter "morph*setup*.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($SetupFile) {
    Write-Host "`n=========================================" -ForegroundColor Green
    Write-Host " SUCCESS! Installer generated:" -ForegroundColor Green
    Write-Host " $($SetupFile.FullName)" -ForegroundColor White
    Write-Host " Size: $([math]::Round($SetupFile.Length / 1MB, 2)) MB" -ForegroundColor Gray
    Write-Host "=========================================" -ForegroundColor Green
} else {
    Write-Host "`nBuild complete." -ForegroundColor Yellow
}
