param(
  [Parameter(Mandatory = $true)]
  [string]$DjvuRoot
)

$ErrorActionPreference = "Stop"

function Download-WithRetry {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Uri,
    [Parameter(Mandatory = $true)]
    [string]$OutFile,
    [int]$Attempts = 3
  )

  for ($i = 1; $i -le $Attempts; $i++) {
    try {
      Invoke-WebRequest -Uri $Uri -OutFile $OutFile
      return
    } catch {
      if ($i -eq $Attempts) {
        throw
      }
      Start-Sleep -Seconds (2 * $i)
    }
  }
}

$winRoot = Join-Path $DjvuRoot "windows"
$tmpRoot = $env:RUNNER_TEMP
if ([string]::IsNullOrWhiteSpace($tmpRoot)) {
  $tmpRoot = [System.IO.Path]::GetTempPath()
}

$jpegDir = Join-Path $winRoot "jpeg\jpeg-6b"
if (-not (Test-Path $jpegDir)) {
  New-Item -ItemType Directory -Force -Path (Join-Path $winRoot "jpeg") | Out-Null
  $jpegTar = Join-Path $tmpRoot "jpegsrc.v6b.tar.gz"
  Download-WithRetry -Uri "https://www.ijg.org/files/jpegsrc.v6b.tar.gz" -OutFile $jpegTar
  tar -xzf $jpegTar -C (Join-Path $winRoot "jpeg")
}

$zlibDir = Join-Path $winRoot "zlib\zlib"
if (-not (Test-Path $zlibDir)) {
  New-Item -ItemType Directory -Force -Path (Join-Path $winRoot "zlib") | Out-Null
  $zlibTar = Join-Path $tmpRoot "zlib-1.2.13.tar.gz"
  Download-WithRetry -Uri "https://zlib.net/fossils/zlib-1.2.13.tar.gz" -OutFile $zlibTar
  tar -xzf $zlibTar -C (Join-Path $winRoot "zlib")
  $zlibExtractedDir = Join-Path $winRoot "zlib\zlib-1.2.13"
  if (Test-Path $zlibExtractedDir) {
    Move-Item -Force $zlibExtractedDir $zlibDir
  }
}

$tiffLibDir = Join-Path $winRoot "tiff\tiff\libtiff"
if (-not (Test-Path $tiffLibDir)) {
  New-Item -ItemType Directory -Force -Path (Join-Path $winRoot "tiff") | Out-Null
  $tiffTar = Join-Path $tmpRoot "tiff-4.0.10.tar.gz"
  Download-WithRetry -Uri "https://download.osgeo.org/libtiff/tiff-4.0.10.tar.gz" -OutFile $tiffTar
  tar -xzf $tiffTar -C (Join-Path $winRoot "tiff")
  $tiffExtractedDir = Join-Path $winRoot "tiff\tiff-4.0.10"
  if (Test-Path $tiffExtractedDir) {
    Move-Item -Force $tiffExtractedDir (Join-Path $winRoot "tiff\tiff")
  }
}
