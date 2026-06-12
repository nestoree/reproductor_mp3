#Requires -Version 5.1

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$msys2Root = "C:\msys64"
$bashExe = "$msys2Root\usr\bin\bash.exe"
$exitCode = 0
$srcDir = $PSScriptRoot
$logFile = "$srcDir\setup.log"

Remove-Item $logFile -ErrorAction SilentlyContinue
function Log { param([string]$M) $M | Out-File -FilePath $logFile -Encoding UTF8 -Append; Write-Host $M }

function Write-Step {
    param([string]$Message)
    $msg = "`n>> $Message"
    $msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $msg -ForegroundColor Cyan
}
function Write-Info {
    param([string]$Message)
    $msg = "   $Message"
    $msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $msg -ForegroundColor Gray
}
function Write-Success {
    param([string]$Message)
    $msg = "   $Message"
    $msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $msg -ForegroundColor Green
}
function Write-Warning {
    param([string]$Message)
    $msg = "   $Message"
    $msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $msg -ForegroundColor Yellow
}

function Invoke-Msys2 {
    param([string]$Command)
    $prevMsys = $env:MSYSTEM
    $prevChere = $env:CHERE_INVOKING
    $env:MSYSTEM = "MINGW64"
    $env:CHERE_INVOKING = "1"
    $output = & $bashExe --login -c $Command 2>&1
    $global:LAST_BASH_EXITCODE = $LASTEXITCODE
    if ($prevMsys -eq $null) { Remove-Item Env:\MSYSTEM -ErrorAction SilentlyContinue }
    else { $env:MSYSTEM = $prevMsys }
    if ($prevChere -eq $null) { Remove-Item Env:\CHERE_INVOKING -ErrorAction SilentlyContinue }
    else { $env:CHERE_INVOKING = $prevChere }
    return $output
}

function Exec-Msys2 {
    param([string]$Command)
    $output = Invoke-Msys2 $Command
    foreach ($line in $output) { $line | Out-File -FilePath $logFile -Encoding UTF8 -Append }
    foreach ($line in $output) { Write-Host $line }
    return $output
}

# --- Elevate to admin ---
$currentPrincipal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $msg = "Reiniciando con privilegios de administrador..."
    $msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $msg -ForegroundColor Yellow
    $psArgs = "-ExecutionPolicy Bypass -NoProfile -File `"$PSCommandPath`""
    try {
        $proc = Start-Process -FilePath powershell -Verb RunAs -ArgumentList $psArgs -PassThru -Wait -ErrorAction Stop
        exit $proc.ExitCode
    } catch {
        $msg = "`nERROR: Debe aceptar la ventana de UAC. Ejecute setup.bat como administrador manualmente."
        $msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
        Write-Host $msg -ForegroundColor Red
        Read-Host "Presione Enter para salir"
        exit 1
    }
}

try {

$msg = "============================================"
$msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
Write-Host $msg -ForegroundColor Cyan
$msg = " Reproductor App - Instalador"
$msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
Write-Host $msg -ForegroundColor Cyan
$msg = "============================================"
$msg | Out-File -FilePath $logFile -Encoding UTF8 -Append
Write-Host $msg -ForegroundColor Cyan

# --- Step 1: Detect or install MSYS2 ---
Write-Step "Paso 1/4: MSYS2"

if (Test-Path $bashExe) {
    Write-Success "MSYS2 encontrado en $msys2Root"
} else {
    Write-Info "Instalando MSYS2..."
    $wingetOk = $false
    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if ($winget) {
        Write-Info "Instalando MSYS2 via winget..."
        winget install "MSYS2.MSYS2" --accept-package-agreements --accept-source-agreements 2>&1 | ForEach-Object { $_ | Out-File -FilePath $logFile -Encoding UTF8 -Append; Write-Host $_ }
        if ($LASTEXITCODE -eq 0 -and (Test-Path $bashExe)) {
            $wingetOk = $true
        } else {
            Write-Warning "winget fallo, intentando descarga directa..."
        }
    }

    if (-not $wingetOk) {
        $installerUrl = "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64.exe"
        try {
            $release = Invoke-RestMethod -Uri "https://api.github.com/repos/msys2/msys2-installer/releases/latest" -ErrorAction Stop
            $asset = $release.assets | Where-Object { $_.name -like "msys2-x86_64-*.exe" } | Select-Object -First 1
            if ($asset) { $installerUrl = $asset.browser_download_url }
        } catch { Write-Warning "Usando URL por defecto para MSYS2" }
        $installer = "$env:TEMP\msys2-installer.exe"
        Write-Info "Descargando MSYS2 desde $installerUrl..."
        Invoke-WebRequest -Uri $installerUrl -OutFile $installer
        Write-Info "Ejecutando instalador..."
        $p = Start-Process -Wait -FilePath $installer -ArgumentList "--confirm-command", "--accept-messages", "--root", $msys2Root -PassThru
        if ($p.ExitCode -ne 0) {
            throw "Instalador de MSYS2 fallo (codigo $($p.ExitCode))"
        }
        if (-not (Test-Path $bashExe)) {
            throw "MSYS2 instalado pero bash.exe no encontrado"
        }
    }
    Write-Success "MSYS2 instalado"
}

# --- Step 2: Update MSYS2 ---
Write-Step "Paso 2/4: Actualizando MSYS2"

Write-Info "Pacman: llavero de paquetes..."
Exec-Msys2 "pacman -Sy --noconfirm msys2-keyring 2>&1"
if ($global:LAST_BASH_EXITCODE -ne 0) { throw "Fallo al actualizar llavero de MSYS2" }

Write-Info "Pacman: actualizacion (paso 1)..."
Exec-Msys2 "pacman -Su --noconfirm 2>&1"

Write-Info "Pacman: actualizacion (paso 2)..."
Exec-Msys2 "pacman -Su --noconfirm 2>&1"

# --- Step 3: Install dependencies ---
Write-Step "Paso 3/4: Dependencias"

$packages = @(
    "mingw-w64-x86_64-gcc",
    "mingw-w64-x86_64-pkg-config",
    "mingw-w64-x86_64-gtk3",
    "mingw-w64-x86_64-gstreamer",
    "mingw-w64-x86_64-gst-plugins-base",
    "mingw-w64-x86_64-gst-plugins-good",
    "mingw-w64-x86_64-gst-libav",
    "mingw-w64-x86_64-gst-plugins-ugly",
    "mingw-w64-x86_64-librsvg"
)

$pkgStr = $packages -join ' '
Write-Info "Instalando paquetes: $pkgStr"
Exec-Msys2 "pacman -S --noconfirm --needed $pkgStr 2>&1"
if ($global:LAST_BASH_EXITCODE -ne 0) { throw "Fallo al instalar paquetes" }

# --- Step 4: Compile ---
Write-Step "Paso 4/4: Compilando"

$unixSrc = & "$msys2Root\usr\bin\cygpath.exe" -u "$srcDir"
"unixSrc = $unixSrc" | Out-File -FilePath $logFile -Encoding UTF8 -Append

if (-not (Test-Path "$srcDir\player.c")) {
    throw "No se encontro player.c en $srcDir"
}

Write-Info "Compilando player.c..."

$compileCmd = "cd '$unixSrc' && gcc -Wall -O2 -mwindows -o reproductor.exe player.c `$(pkg-config --cflags --libs gtk+-3.0 gstreamer-1.0 gstreamer-pbutils-1.0) -lm 2>&1"
"Compile command: $compileCmd" | Out-File -FilePath $logFile -Encoding UTF8 -Append

$output = Invoke-Msys2 $compileCmd
$output | Out-File -FilePath $logFile -Encoding UTF8 -Append
"Bash exit code: $global:LAST_BASH_EXITCODE" | Out-File -FilePath $logFile -Encoding UTF8 -Append

if ($global:LAST_BASH_EXITCODE -ne 0) {
    Write-Host "`nERROR DE COMPILACION:" -ForegroundColor Red
    Write-Host $output -ForegroundColor Red
    Write-Host "`nRevise '$logFile' para la salida completa." -ForegroundColor Yellow
    throw "Compilacion fallida"
}

if (-not (Test-Path "$srcDir\reproductor.exe")) {
    throw "No se encontro reproductor.exe despues de compilar"
}

$size = (Get-Item "$srcDir\reproductor.exe").Length
Write-Success "Compilado! $([math]::Round($size/1KB)) KB"

# --- Create run.bat ---
$runBat = @"
@echo off
title Reproductor App
set "MSYS2_ROOT=$msys2Root"
set "GST_PLUGIN_PATH=%MSYS2_ROOT%\mingw64\lib\gstreamer-1.0"
set "GSETTINGS_SCHEMA_DIR=%MSYS2_ROOT%\mingw64\share\glib-2.0\schemas"
set "PATH=%MSYS2_ROOT%\mingw64\bin;%MSYS2_ROOT%\usr\bin;%PATH%"
start "" "%~dp0reproductor.exe"
"@
[System.IO.File]::WriteAllText("$srcDir\run.bat", $runBat, [System.Text.Encoding]::ASCII)
Write-Success "run.bat creado"

Write-Host "`n============================================" -ForegroundColor Green
Write-Host " INSTALACION COMPLETADA!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host "`nIniciando Reproductor App..." -ForegroundColor White

# --- Run ---
$env:GST_PLUGIN_PATH = "$msys2Root\mingw64\lib\gstreamer-1.0"
$env:GSETTINGS_SCHEMA_DIR = "$msys2Root\mingw64\share\glib-2.0\schemas"
$env:PATH = "$msys2Root\mingw64\bin;$msys2Root\usr\bin;$env:PATH"
Start-Process -FilePath "$srcDir\reproductor.exe" -WorkingDirectory "$srcDir"

} catch {
    $err = "`n============================================"
    $err | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $err -ForegroundColor Red
    $err = " ERROR"
    $err | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $err -ForegroundColor Red
    $err = "============================================"
    $err | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $err -ForegroundColor Red
    $errMsg = "`n$_"
    $errMsg | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $errMsg -ForegroundColor Red
    $logMsg = "`nRevise el archivo de log: $logFile"
    $logMsg | Out-File -FilePath $logFile -Encoding UTF8 -Append
    Write-Host $logMsg -ForegroundColor Yellow
    $exitCode = 1
}

Write-Host "`nPresione Enter para salir." -ForegroundColor Gray
Read-Host
exit $exitCode
