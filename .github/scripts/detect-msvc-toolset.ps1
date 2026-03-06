param(
  [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

if ($env:DJVU_PLATFORM_TOOLSET) {
  "toolset=$env:DJVU_PLATFORM_TOOLSET" >> $env:GITHUB_OUTPUT
  Write-Host "Using preconfigured DJVU_PLATFORM_TOOLSET=$env:DJVU_PLATFORM_TOOLSET"
  exit 0
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
  throw "vswhere.exe not found"
}

$candidates = @("v143", "v142", "v145")
$vcTrees = @("v170", "v160", "v180")
$toolset = $null
$selectedInstallPath = $null
$installPaths = [System.Collections.Generic.List[string]]::new()

# Prefer the Visual Studio instance that provides the msbuild on PATH.
$msbuildCmd = Get-Command msbuild.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if ($msbuildCmd) {
  $msbuildPath = $msbuildCmd.Source
  if ($msbuildPath -match "^(?<root>.+)\\MSBuild\\Current\\Bin(?:\\amd64)?\\MSBuild\.exe$") {
    $installPaths.Add($Matches.root)
  }
}

$vswherePaths = & $vswhere -all -products * -requires Microsoft.Component.MSBuild -property installationPath
if ($LASTEXITCODE -eq 0 -and $vswherePaths) {
  foreach ($path in ($vswherePaths -split "`r?`n")) {
    if (-not [string]::IsNullOrWhiteSpace($path)) {
      $installPaths.Add($path.Trim())
    }
  }
}

if ($installPaths.Count -eq 0) {
  throw "Failed to resolve Visual Studio installation path via msbuild/vswhere"
}

foreach ($v in $candidates) {
  foreach ($installPath in $installPaths | Select-Object -Unique) {
    foreach ($vcTree in $vcTrees) {
      $probe = Join-Path $installPath "MSBuild\Microsoft\VC\$vcTree\Platforms\$Platform\PlatformToolsets\$v\Toolset.props"
      if (Test-Path $probe) {
        $toolset = $v
        $selectedInstallPath = $installPath
        break
      }
    }
    if ($toolset) { break }
  }
  if ($toolset) { break }
}

if (-not $toolset -and $Platform -ieq "x64") {
  foreach ($v in $candidates) {
    foreach ($installPath in $installPaths | Select-Object -Unique) {
      foreach ($vcTree in $vcTrees) {
        $probe = Join-Path $installPath "MSBuild\Microsoft\VC\$vcTree\Platforms\x86_amd64\PlatformToolsets\$v\Toolset.props"
        if (Test-Path $probe) {
          $toolset = $v
          $selectedInstallPath = $installPath
          break
        }
      }
      if ($toolset) { break }
    }
    if ($toolset) { break }
  }
}

if (-not $toolset) {
  throw "No supported MSVC toolset found (v143/v142/v145) for platform '$Platform'"
}

"toolset=$toolset" >> $env:GITHUB_OUTPUT
if ($selectedInstallPath) {
  Write-Host "Detected DJVU_PLATFORM_TOOLSET=$toolset from '$selectedInstallPath'"
} else {
  Write-Host "Detected DJVU_PLATFORM_TOOLSET=$toolset"
}
