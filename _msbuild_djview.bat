@echo off
setlocal EnableExtensions

rem Optional overrides:
rem   VCVARS     - full path to vcvars64.bat
rem   VCVARS_VER - optional vcvars toolset version
rem   MSBUILD    - full path to MSBuild.exe
rem   VCPKG_ROOT - path to vcpkg root (for lrelease)
rem   VCPKG_TRIPLET - vcpkg triplet (default: x64-windows)

set "ROOT=%~dp0"
if not defined VCPKG_TRIPLET set "VCPKG_TRIPLET=x64-windows"

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

"%MSBUILD%" "%ROOT%src\djview.vcxproj" /m /p:Configuration=Release /p:Platform=x64 /p:TrackFileAccess=false
if errorlevel 1 exit /b %errorlevel%

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

set "LRELEASE_EXE=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\tools\Qt6\bin\lrelease.exe"
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
echo Translations are ready in: %LANG_OUT%
exit /b 0
