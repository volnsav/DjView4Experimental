@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Optional overrides:
rem   %1             - configuration (Debug or Release, default: Release)
rem   %2             - platform (x64, x86, arm64, default: x64)
rem   DJVU_ROOT      - path to DjVuLibreExperimental root
rem   VCPKG_ROOT     - path to vcpkg root
rem   VCPKG_TRIPLET  - vcpkg triplet (default: x64-windows)
rem   VCPKG_HOST_TRIPLET - host triplet for Qt tools (default: x64-windows)
rem   VCVARS         - full path to vcvars64.bat
rem   VCVARS_VER     - optional vcvars toolset version (example: 14.44.35207)
rem   MSBUILD        - full path to MSBuild.exe
rem   MSBUILD_MAXCPU - optional max parallel nodes for MSBuild (/m:N)
rem   DJVU_PLATFORM_TOOLSET - optional MSVC platform toolset override (for example: v142, v143, v145)

set "ROOT=%~dp0"
for %%I in ("%ROOT%.") do set "ROOT=%%~fI\"
set "ROOT_DIR=%ROOT%"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
if not exist "%ROOT%build" mkdir "%ROOT%build" || exit /b 1
set "BUILD_CONFIG=%~1"
if not defined BUILD_CONFIG set "BUILD_CONFIG=Release"
set "BUILD_PLATFORM=%~2"
if not defined BUILD_PLATFORM set "BUILD_PLATFORM=x64"
set "BUILD_CONFIG_DIR=%BUILD_CONFIG%"
set "BUILD_PLATFORM_DIR=%BUILD_PLATFORM%"
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
  set "BUILD_PLATFORM_DIR=x64"
) else if /I "%BUILD_PLATFORM%"=="x86" (
  set "BUILD_PLATFORM=Win32"
  set "BUILD_PLATFORM_DIR=Win32"
) else if /I "%BUILD_PLATFORM%"=="arm64" (
  set "BUILD_PLATFORM=ARM64"
  set "BUILD_PLATFORM_DIR=ARM64"
) else (
  echo ERROR: invalid platform "%BUILD_PLATFORM%". Use x64, x86, or arm64.
  exit /b 1
)
set "OUTDIR=%ROOT%build\%BUILD_CONFIG%_%BUILD_PLATFORM_DIR%_strict"
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

if not defined DJVU_ROOT set "DJVU_ROOT=%ROOT%..\DjVuLibreExperimental"
for %%I in ("%DJVU_ROOT%") do set "DJVU_ROOT=%%~fI"

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
set "VCPKG_BIN_REL=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\bin"
set "VCPKG_BIN_DBG=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\debug\bin"
set "VCPKG_BIN=%VCPKG_BIN_REL%"
set "VCPKG_BIN_FALLBACK="
if /I "%BUILD_CONFIG%"=="Debug" (
  set "VCPKG_BIN=%VCPKG_BIN_DBG%"
  set "VCPKG_BIN_FALLBACK=%VCPKG_BIN_REL%"
)
set "LRELEASE_EXE=%VCPKG_ROOT%\installed\%VCPKG_HOST_TRIPLET%\tools\Qt6\bin\lrelease.exe"
set "QT_TRANSLATIONS_DIR=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\translations\Qt6"
set "QT_PLUGIN_DIR_REL=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\Qt6\plugins"
set "QT_PLUGIN_DIR_DBG=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\debug\Qt6\plugins"
set "QT_PLUGIN_DIR=%QT_PLUGIN_DIR_REL%"
set "QT_DLL_SUFFIX="
set "QT_PLATFORM_PLUGIN=qwindows.dll"
set "QT_PLATFORM_PLUGIN_FALLBACK="
set "ICUDT_DLL=icudt78.dll"
set "ICUIN_DLL=icuin78.dll"
set "ICUUC_DLL=icuuc78.dll"
set "LIBCRYPTO_DLL=libcrypto-3-x64.dll"
set "LIBSSL_DLL=libssl-3-x64.dll"
set "BZIP_DLL=bz2.dll"
set "FREETYPE_DLL=freetype.dll"
set "PNG_DLL=libpng16.dll"
set "PCRE16_DLL=pcre2-16.dll"
set "ZLIB_DLL=zlib1.dll"
if /I "%BUILD_PLATFORM%"=="Win32" (
  set "LIBCRYPTO_DLL=libcrypto-3.dll"
  set "LIBSSL_DLL=libssl-3.dll"
) else if /I "%BUILD_PLATFORM%"=="ARM64" (
  set "LIBCRYPTO_DLL=libcrypto-3-arm64.dll"
  set "LIBSSL_DLL=libssl-3-arm64.dll"
)
if /I "%BUILD_CONFIG%"=="Debug" (
  set "QT_DLL_SUFFIX=d"
  set "QT_PLATFORM_PLUGIN=qwindowsd.dll"
  set "QT_PLATFORM_PLUGIN_FALLBACK=qwindows.dll"
  set "QT_PLUGIN_DIR=%QT_PLUGIN_DIR_DBG%"
  set "ICUDT_DLL=icudtd78.dll"
  set "ICUIN_DLL=icuind78.dll"
  set "ICUUC_DLL=icuucd78.dll"
  set "BZIP_DLL=bz2d.dll"
  set "FREETYPE_DLL=freetyped.dll"
  set "PNG_DLL=libpng16d.dll"
  set "PCRE16_DLL=pcre2-16d.dll"
  set "ZLIB_DLL=zlibd1.dll"
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
if not exist "%DJVU_ROOT%\windows\djvulibre\djvulibre.sln" (
  echo ERROR: DjVuLibre solution not found under "%DJVU_ROOT%".
  echo        Override with: set DJVU_ROOT=...
  exit /b 1
)
if not exist "%VCPKG_BIN%" (
  echo ERROR: vcpkg runtime bin dir not found: "%VCPKG_BIN%"
  echo        Override with: set VCPKG_ROOT=... and optional set VCPKG_TRIPLET=...
  exit /b 1
)
if not exist "%QT_PLUGIN_DIR%\platforms\%QT_PLATFORM_PLUGIN%" (
  if defined QT_PLATFORM_PLUGIN_FALLBACK if exist "%QT_PLUGIN_DIR_REL%\platforms\%QT_PLATFORM_PLUGIN_FALLBACK%" (
    set "QT_PLUGIN_DIR=%QT_PLUGIN_DIR_REL%"
    set "QT_PLATFORM_PLUGIN=%QT_PLATFORM_PLUGIN_FALLBACK%"
  ) else (
    echo ERROR: Qt platform plugin not found: "%QT_PLUGIN_DIR%\platforms\%QT_PLATFORM_PLUGIN%"
    if defined QT_PLATFORM_PLUGIN_FALLBACK echo        fallback not found: "%QT_PLUGIN_DIR_REL%\platforms\%QT_PLATFORM_PLUGIN_FALLBACK%"
    echo        Check VCPKG_ROOT/VCPKG_TRIPLET and that Qt6 is installed in vcpkg.
    exit /b 1
  )
)
if not exist "%LRELEASE_EXE%" (
  echo ERROR: lrelease.exe not found: "%LRELEASE_EXE%"
  echo        Check VCPKG_ROOT/VCPKG_TRIPLET and that Qt6 tools are installed in vcpkg.
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

rem djview pre-build already triggers libdjvulibre build from DjVuLibreExperimental.
call :prepare_cbt_stamps "%ROOT%src\%BUILD_CONFIG_DIR%" || exit /b 1
"%MSBUILD%" "%ROOT%src\djview.vcxproj" %MSBUILD_PARALLEL% %MSBUILD_TOOLSET_ARGS% /p:Configuration=%BUILD_CONFIG% /p:Platform=%BUILD_PLATFORM% /p:VcpkgHostTriplet=%VCPKG_HOST_TRIPLET% /p:TrackFileAccess=false
if errorlevel 1 exit /b 1

if exist "%OUTDIR%" rmdir /S /Q "%OUTDIR%"
mkdir "%OUTDIR%" || exit /b 1
mkdir "%OUTDIR%\platforms" || exit /b 1
mkdir "%OUTDIR%\styles" || exit /b 1
copy /Y "%ROOT%src\%BUILD_CONFIG_DIR%\djview.exe" "%OUTDIR%\djview.exe" >nul
if errorlevel 1 exit /b 1

rem Runtime DLLs from DjVuLibre build output.
for %%F in (libdjvulibre.dll libjpeg.dll) do (
  copy /Y "%DJVU_ROOT%\windows\build\%BUILD_CONFIG%\%BUILD_PLATFORM_DIR%\%%F" "%OUTDIR%\%%F" >nul || exit /b 1
)

rem Runtime DLLs from Qt/vcpkg (minimal verified set).
for %%F in (
  Qt6Core%QT_DLL_SUFFIX%.dll
  Qt6Gui%QT_DLL_SUFFIX%.dll
  Qt6Network%QT_DLL_SUFFIX%.dll
  Qt6OpenGL%QT_DLL_SUFFIX%.dll
  Qt6OpenGLWidgets%QT_DLL_SUFFIX%.dll
  Qt6PrintSupport%QT_DLL_SUFFIX%.dll
  Qt6Widgets%QT_DLL_SUFFIX%.dll
  %ICUDT_DLL%
  %ICUIN_DLL%
  %ICUUC_DLL%
  brotlicommon.dll
  brotlidec.dll
  %BZIP_DLL%
  double-conversion.dll
  %FREETYPE_DLL%
  harfbuzz.dll
  %LIBCRYPTO_DLL%
  %LIBSSL_DLL%
  %PNG_DLL%
  md4c.dll
  %PCRE16_DLL%
  %ZLIB_DLL%
  zstd.dll
) do (
  call :copy_from_vcpkg_bin "%%F" "%OUTDIR%\%%F" || exit /b 1
)

rem Required Qt platform plugin.
copy /Y "%QT_PLUGIN_DIR%\platforms\%QT_PLATFORM_PLUGIN%" "%OUTDIR%\platforms\%QT_PLATFORM_PLUGIN%" >nul || exit /b 1

rem Optional Qt style plugins (needed when using -style with plugin styles).
call :copy_plugin_dir_dlls "%QT_PLUGIN_DIR%\styles" "%OUTDIR%\styles" "%QT_PLUGIN_DIR_REL%\styles" || exit /b 1

rem Compile and deploy translations.
mkdir "%OUTDIR%\share\djvu\djview4" || exit /b 1
for %%F in ("%ROOT%src\*.ts") do (
  "%LRELEASE_EXE%" "%%~fF" -qm "%OUTDIR%\share\djvu\djview4\%%~nF.qm" >nul || exit /b 1
)
if exist "%QT_TRANSLATIONS_DIR%" (
  for %%F in ("%ROOT%src\djview_*.ts") do (
    set "lang=%%~nF"
    set "lang=!lang:djview_=!"
    if /I "!lang!"=="zh_cn" set "lang=zh_CN"
    if /I "!lang!"=="zh_tw" set "lang=zh_TW"
    if exist "%QT_TRANSLATIONS_DIR%\qt_!lang!.qm" copy /Y "%QT_TRANSLATIONS_DIR%\qt_!lang!.qm" "%OUTDIR%\share\djvu\djview4\qt_!lang!.qm" >nul || exit /b 1
    if exist "%QT_TRANSLATIONS_DIR%\qtbase_!lang!.qm" copy /Y "%QT_TRANSLATIONS_DIR%\qtbase_!lang!.qm" "%OUTDIR%\share\djvu\djview4\qtbase_!lang!.qm" >nul || exit /b 1
  )
)

echo.
echo Combined build: Configuration=%BUILD_CONFIG% Platform=%BUILD_PLATFORM%
echo Combined runtime is ready in: %OUTDIR%
exit /b 0

:copy_plugin_dir_dlls
set "PLUGIN_SRC=%~1"
set "PLUGIN_DST=%~2"
set "PLUGIN_FALLBACK=%~3"
if exist "%PLUGIN_SRC%\*.dll" (
  copy /Y "%PLUGIN_SRC%\*.dll" "%PLUGIN_DST%\" >nul || exit /b 1
  exit /b 0
)
if not "%PLUGIN_FALLBACK%"=="" if exist "%PLUGIN_FALLBACK%\*.dll" (
  copy /Y "%PLUGIN_FALLBACK%\*.dll" "%PLUGIN_DST%\" >nul || exit /b 1
  exit /b 0
)
echo WARNING: no style plugins were found in "%PLUGIN_SRC%"
if not "%PLUGIN_FALLBACK%"=="" echo          fallback checked: "%PLUGIN_FALLBACK%"
exit /b 0

:copy_from_vcpkg_bin
set "SRC_NAME=%~1"
set "DST_PATH=%~2"
if exist "%VCPKG_BIN%\%SRC_NAME%" (
  copy /Y "%VCPKG_BIN%\%SRC_NAME%" "%DST_PATH%" >nul
  exit /b %errorlevel%
)
if defined VCPKG_BIN_FALLBACK if exist "%VCPKG_BIN_FALLBACK%\%SRC_NAME%" (
  copy /Y "%VCPKG_BIN_FALLBACK%\%SRC_NAME%" "%DST_PATH%" >nul
  exit /b %errorlevel%
)
echo ERROR: runtime DLL not found: %SRC_NAME%
echo        looked in: "%VCPKG_BIN%"
if defined VCPKG_BIN_FALLBACK echo        and fallback: "%VCPKG_BIN_FALLBACK%"
exit /b 1

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
