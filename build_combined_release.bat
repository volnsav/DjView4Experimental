@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Optional overrides:
rem   DJVU_ROOT      - path to DjVuLibreExperimental root
rem   VCPKG_ROOT     - path to vcpkg root
rem   VCPKG_TRIPLET  - vcpkg triplet (default: x64-windows)
rem   VCVARS         - full path to vcvars64.bat
rem   VCVARS_VER     - optional vcvars toolset version (example: 14.44.35207)
rem   MSBUILD        - full path to MSBuild.exe
rem   MSBUILD_MAXCPU - optional max parallel nodes for MSBuild (/m:N)

set "ROOT=%~dp0"
for %%I in ("%ROOT%.") do set "ROOT=%%~fI\"
set "ROOT_DIR=%ROOT%"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
if not exist "%ROOT%build" mkdir "%ROOT%build" || exit /b 1
set "OUTDIR=%ROOT%build\Release_strict"
set "MSBUILD_PARALLEL=/m"
if defined MSBUILD_MAXCPU set "MSBUILD_PARALLEL=/m:%MSBUILD_MAXCPU%"

if not defined DJVU_ROOT set "DJVU_ROOT=%ROOT%..\DjVuLibreExperimental"
for %%I in ("%DJVU_ROOT%") do set "DJVU_ROOT=%%~fI"

if not defined VCPKG_TRIPLET set "VCPKG_TRIPLET=x64-windows"

if defined VCPKG_ROOT (
  if not exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\tools\Qt6\bin\lrelease.exe" set "VCPKG_ROOT="
)
if not defined VCPKG_ROOT (
  if exist "%ROOT%..\vcpkg\vcpkg.exe" set "VCPKG_ROOT=%ROOT%..\vcpkg"
)
if not defined VCPKG_ROOT (
  if exist "%USERPROFILE%\vcpkg\vcpkg.exe" set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
)
if not defined VCPKG_ROOT (
  for /f "delims=" %%I in ('where vcpkg.exe 2^>nul') do (
    for %%J in ("%%~dpI..") do set "VCPKG_ROOT=%%~fJ"
    goto :have_vcpkg_root
  )
)
:have_vcpkg_root
if defined VCPKG_ROOT for %%I in ("%VCPKG_ROOT%") do set "VCPKG_ROOT=%%~fI"
set "VCPKG_BIN=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\bin"
set "LRELEASE_EXE=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\tools\Qt6\bin\lrelease.exe"
set "QT_TRANSLATIONS_DIR=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\translations\Qt6"
set "QT_PLUGIN_DIR=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\Qt6\plugins"

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
  echo ERROR: vcpkg bin dir not found: "%VCPKG_BIN%"
  echo        Override with: set VCPKG_ROOT=... and optional set VCPKG_TRIPLET=...
  exit /b 1
)
if not exist "%QT_PLUGIN_DIR%\platforms\qwindows.dll" (
  echo ERROR: Qt platforms plugin not found under "%QT_PLUGIN_DIR%"
  echo        Check VCPKG_ROOT/VCPKG_TRIPLET and that Qt6 is installed in vcpkg.
  exit /b 1
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
call :prepare_cbt_stamps "%ROOT%src\release" || exit /b 1
"%MSBUILD%" "%ROOT%src\djview.vcxproj" %MSBUILD_PARALLEL% /p:Configuration=Release /p:Platform=x64 /p:TrackFileAccess=false
if errorlevel 1 exit /b 1

if exist "%OUTDIR%" rmdir /S /Q "%OUTDIR%"
mkdir "%OUTDIR%" || exit /b 1
mkdir "%OUTDIR%\platforms" || exit /b 1
copy /Y "%ROOT%src\release\djview.exe" "%OUTDIR%\djview.exe" >nul
if errorlevel 1 exit /b 1

rem Runtime DLLs from DjVuLibre build output.
for %%F in (libdjvulibre.dll libjpeg.dll) do (
  copy /Y "%DJVU_ROOT%\windows\build\Release\x64\%%F" "%OUTDIR%\%%F" >nul || exit /b 1
)

rem Runtime DLLs from Qt/vcpkg (minimal verified set).
for %%F in (
  Qt6Core.dll
  Qt6Gui.dll
  Qt6Network.dll
  Qt6OpenGL.dll
  Qt6OpenGLWidgets.dll
  Qt6PrintSupport.dll
  Qt6Widgets.dll
  icudt78.dll
  icuin78.dll
  icuuc78.dll
  brotlicommon.dll
  brotlidec.dll
  bz2.dll
  double-conversion.dll
  freetype.dll
  harfbuzz.dll
  libcrypto-3-x64.dll
  libpng16.dll
  md4c.dll
  pcre2-16.dll
  zlib1.dll
  zstd.dll
) do (
  copy /Y "%VCPKG_BIN%\%%F" "%OUTDIR%\%%F" >nul || exit /b 1
)

rem Required Qt platform plugin.
copy /Y "%QT_PLUGIN_DIR%\platforms\qwindows.dll" "%OUTDIR%\platforms\qwindows.dll" >nul || exit /b 1

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
echo Combined runtime is ready in: %OUTDIR%
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
