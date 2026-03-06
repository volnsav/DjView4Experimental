@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Optional overrides:
rem   %1         - configuration (Debug or Release, default: Release)
rem   %2         - platform (x64, x86, arm64, default: x64)
rem   VCVARS     - full path to vcvars64.bat
rem   VCVARS_VER - optional vcvars toolset version
rem   MSBUILD    - full path to MSBuild.exe
rem   MSBUILD_MAXCPU - optional max parallel nodes for MSBuild (/m:N)
rem   DJVU_PLATFORM_TOOLSET - optional MSVC platform toolset override (for example: v142, v143, v145)
rem   VCPKG_ROOT - path to vcpkg root (for lrelease)
rem   VCPKG_TRIPLET - vcpkg triplet (default: x64-windows)
rem   VCPKG_HOST_TRIPLET - host triplet for Qt tools (default: x64-windows)
rem   DJVU_ROOT  - path to DjVuLibreExperimental root

set "ROOT=%~dp0"
set "ROOT_DIR=%ROOT%"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
if not exist "%ROOT%build" mkdir "%ROOT%build" || exit /b 1
set "BUILD_CONFIG=%~1"
if not defined BUILD_CONFIG set "BUILD_CONFIG=Release"
set "BUILD_PLATFORM=%~2"
if not defined BUILD_PLATFORM set "BUILD_PLATFORM=x64"
set "BUILD_CONFIG_DIR=%BUILD_CONFIG%"
if /I "%BUILD_CONFIG%"=="Debug" (
  set "BUILD_CONFIG=Debug"
  set "BUILD_CONFIG_DIR=debug"
) else if /I "%BUILD_CONFIG%"=="Release" (
  set "BUILD_CONFIG=Release"
  set "BUILD_CONFIG_DIR=release"
) else (
  echo ERROR: invalid configuration "%BUILD_CONFIG%". Use Debug or Release.
  exit /b 1
)
if /I "%BUILD_PLATFORM%"=="x64" (
  set "BUILD_PLATFORM=x64"
) else if /I "%BUILD_PLATFORM%"=="x86" (
  set "BUILD_PLATFORM=Win32"
) else if /I "%BUILD_PLATFORM%"=="arm64" (
  set "BUILD_PLATFORM=ARM64"
) else (
  echo ERROR: invalid platform "%BUILD_PLATFORM%". Use x64, x86, or arm64.
  exit /b 1
)
if not defined VCPKG_TRIPLET (
  if /I "%BUILD_PLATFORM%"=="Win32" (
    set "VCPKG_TRIPLET=x86-windows"
  ) else if /I "%BUILD_PLATFORM%"=="ARM64" (
    set "VCPKG_TRIPLET=arm64-windows"
  ) else (
    set "VCPKG_TRIPLET=x64-windows"
  )
)
if not defined VCPKG_HOST_TRIPLET set "VCPKG_HOST_TRIPLET=x64-windows"
if not defined DJVU_ROOT set "DJVU_ROOT=%ROOT%..\DjVuLibreExperimental"
for %%I in ("%DJVU_ROOT%") do set "DJVU_ROOT=%%~fI"
set "MSBUILD_PARALLEL=/m"
if defined MSBUILD_MAXCPU set "MSBUILD_PARALLEL=/m:%MSBUILD_MAXCPU%"
if not defined DJVU_PLATFORM_TOOLSET if defined VCVARS_VER (
  if /I "%VCVARS_VER:~0,4%"=="14.2" set "DJVU_PLATFORM_TOOLSET=v142"
  if /I "%VCVARS_VER:~0,4%"=="14.3" set "DJVU_PLATFORM_TOOLSET=v143"
  if /I "%VCVARS_VER:~0,4%"=="14.5" set "DJVU_PLATFORM_TOOLSET=v145"
)
set "MSBUILD_TOOLSET_ARGS="
if defined DJVU_PLATFORM_TOOLSET (
  set "MSBUILD_TOOLSET_ARGS=/p:PlatformToolset=%DJVU_PLATFORM_TOOLSET% /p:DjvuPlatformToolset=%DJVU_PLATFORM_TOOLSET%"
)

if not defined VCVARS (
  for /d %%D in ("C:\Program Files\Microsoft Visual Studio\*") do (
    for %%E in (Community Professional Enterprise BuildTools Preview) do (
      if not defined VCVARS if exist "%%~fD\%%E\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%~fD\%%E\VC\Auxiliary\Build\vcvars64.bat"
    )
  )
)
if not defined VCVARS (
  for %%I in (
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
  ) do (
    if exist "%%~I" set "VCVARS=%%~I"
  )
)

if not defined MSBUILD (
  for /f "delims=" %%I in ('where msbuild.exe 2^>nul') do (
    if not defined MSBUILD set "MSBUILD=%%I"
  )
)
if not defined MSBUILD (
  for /d %%D in ("C:\Program Files\Microsoft Visual Studio\*") do (
    for %%E in (Community Professional Enterprise BuildTools Preview) do (
      if not defined MSBUILD if exist "%%~fD\%%E\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%~fD\%%E\MSBuild\Current\Bin\MSBuild.exe"
    )
  )
)

if not exist "%VCVARS%" (
  echo ERROR: vcvars64.bat not found. Set VCVARS env var or install VS C++ tools.
  exit /b 1
)
if not exist "%MSBUILD%" (
  echo ERROR: MSBuild.exe not found. Set MSBUILD env var or install VS MSBuild component.
  exit /b 1
)

if defined VCVARS_VER (
  call "%VCVARS%" -vcvars_ver=%VCVARS_VER%
) else (
  call "%VCVARS%"
)
if errorlevel 1 exit /b 1

call :resolve_git_ref "%ROOT_DIR%" APP_GIT_REF
call :resolve_git_ref "%DJVU_ROOT%" LIB_GIT_REF
> "%ROOT%src\build_git_info.h" (
  echo #ifndef BUILD_GIT_INFO_H
  echo #define BUILD_GIT_INFO_H
  echo #define DJVIEW_APP_GIT_REF "!APP_GIT_REF!"
  echo #define DJVIEW_LIB_GIT_REF "!LIB_GIT_REF!"
  echo #endif
)

call :prepare_cbt_stamps "%ROOT%src\%BUILD_CONFIG_DIR%" || exit /b 1
"%MSBUILD%" "%ROOT%src\djview.vcxproj" %MSBUILD_PARALLEL% %MSBUILD_TOOLSET_ARGS% /p:Configuration=%BUILD_CONFIG% /p:Platform=%BUILD_PLATFORM% /p:VcpkgHostTriplet=%VCPKG_HOST_TRIPLET% /p:TrackFileAccess=false
if errorlevel 1 exit /b %errorlevel%

if defined VCPKG_ROOT (
  if not exist "%VCPKG_ROOT%\vcpkg.exe" set "VCPKG_ROOT="
)
if not defined VCPKG_ROOT (
  if exist "%ROOT%vcpkg\vcpkg.exe" set "VCPKG_ROOT=%ROOT%vcpkg"
)
if not defined VCPKG_ROOT (
  if exist "%ROOT%..\vcpkg\vcpkg.exe" set "VCPKG_ROOT=%ROOT%..\vcpkg"
)
if not defined VCPKG_ROOT (
  if exist "%USERPROFILE%\vcpkg\vcpkg.exe" set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
)
if not defined VCPKG_ROOT (
  for /f "delims=" %%I in ('where vcpkg.exe 2^>nul') do (
    for %%J in ("%%~dpI.") do set "VCPKG_ROOT=%%~fJ"
    goto :have_vcpkg_root
  )
)
:have_vcpkg_root
if defined VCPKG_ROOT for %%I in ("%VCPKG_ROOT%") do set "VCPKG_ROOT=%%~fI"
if not defined VCPKG_ROOT (
  echo ERROR: vcpkg root not found. Set VCPKG_ROOT or clone vcpkg to "%ROOT%vcpkg".
  exit /b 1
)

set "LRELEASE_EXE=%VCPKG_ROOT%\installed\%VCPKG_HOST_TRIPLET%\tools\Qt6\bin\lrelease.exe"
set "QT_TRANSLATIONS_DIR=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\translations\Qt6"
if not exist "%LRELEASE_EXE%" (
  echo ERROR: lrelease.exe not found: %LRELEASE_EXE%
  echo        Set VCPKG_ROOT and optional VCPKG_TRIPLET.
  exit /b 1
)

set "LANG_OUT=%ROOT%share\djvu\djview4"
if not exist "%LANG_OUT%" mkdir "%LANG_OUT%" || exit /b 1

for %%F in ("%ROOT%src\*.ts") do (
  echo LRELEASE %%~nxF
  "%LRELEASE_EXE%" "%%~fF" -qm "%LANG_OUT%\%%~nF.qm" || exit /b 1
)

if exist "%QT_TRANSLATIONS_DIR%" (
  for %%F in ("%ROOT%src\djview_*.ts") do (
    set "lang=%%~nF"
    set "lang=!lang:djview_=!"
    if /I "!lang!"=="zh_cn" set "lang=zh_CN"
    if /I "!lang!"=="zh_tw" set "lang=zh_TW"
    if exist "%QT_TRANSLATIONS_DIR%\qt_!lang!.qm" copy /Y "%QT_TRANSLATIONS_DIR%\qt_!lang!.qm" "%LANG_OUT%\qt_!lang!.qm" >nul || exit /b 1
    if exist "%QT_TRANSLATIONS_DIR%\qtbase_!lang!.qm" copy /Y "%QT_TRANSLATIONS_DIR%\qtbase_!lang!.qm" "%LANG_OUT%\qtbase_!lang!.qm" >nul || exit /b 1
  )
)

echo.
echo Build completed: Configuration=%BUILD_CONFIG% Platform=%BUILD_PLATFORM%
echo Translations are ready in: %LANG_OUT%
exit /b 0

:prepare_cbt_stamps
set "CBT_DIR=%~1"
if not exist "%CBT_DIR%" mkdir "%CBT_DIR%" || exit /b 1
for %%F in (
  moc_predefs.h.cbt
  qdjview.moc.cbt
  qdjviewexporters.moc.cbt
  qdjviewplugin.moc.cbt
  qdjviewprefs.moc.cbt
  qdjviewsidebar.moc.cbt
  qdjvu.moc.cbt
  qdjvunet.moc.cbt
  qdjvuwidget.moc.cbt
) do (
  if not exist "%CBT_DIR%\%%F" type nul > "%CBT_DIR%\%%F"
)
exit /b 0

:resolve_git_ref
set "REPO=%~1"
set "OUTVAR=%~2"
set "REF=unknown"
if "%REPO:~-1%"=="\" set "REPO=%REPO:~0,-1%"
if exist "%REPO%\.git" (
  for /f "delims=" %%I in ('git -C "%REPO%" describe --tags --exact-match 2^>nul') do set "REF=%%I"
  if /I "!REF!"=="unknown" (
    set "BRANCH="
    set "HASH="
    for /f "delims=" %%I in ('git -C "%REPO%" rev-parse --abbrev-ref HEAD 2^>nul') do set "BRANCH=%%I"
    for /f "delims=" %%I in ('git -C "%REPO%" rev-parse --short HEAD 2^>nul') do set "HASH=%%I"
    if defined HASH (
      if defined BRANCH (
        if /I "!BRANCH!"=="HEAD" (
          set "REF=!HASH!"
        ) else (
          set "REF=!BRANCH!+!HASH!"
        )
      ) else (
        set "REF=!HASH!"
      )
    )
  )
)
set "%OUTVAR%=%REF%"
exit /b 0
