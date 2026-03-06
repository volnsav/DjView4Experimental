# DjView4

DjView4 is a standalone DjVu document viewer built on the DjVuLibre library
and the Qt toolkit.

**Highlights:**
- entirely based on the public DjVuLibre API
- written in portable C++ with Qt 5 / Qt 6
- works natively under Linux/X11, Windows, and macOS
- continuous and side-by-side page scrolling
- tabbed document window
- thumbnail grid and outline sidebar
- page names, metadata dialog, annotations
- built-in export (image, PDF/PS, TIFF), print support
- implemented as reusable Qt widgets

**Prerequisites:**
- DjVuLibreExperimental (or any DjVuLibre >= 3.5.28)
- Qt 6 (recommended) or Qt 5 >= 5.15
- CMake >= 3.21
- C++14-capable compiler (GCC 9+, Clang 10+, MSVC 2019+)

---

## 1. Building from source

### 1.1 Quick start (all platforms)

Clone both repositories side by side:

```sh
git clone https://github.com/volnsav/DjVuLibreExperimental  -b develop
git clone https://github.com/volnsav/DjView4Experimental    -b develop
```

**Linux / macOS:**

```sh
cmake -S DjVuLibreExperimental -B djvulibre-build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DDJVULIBRE_USE_BUNDLED_DEPS=OFF \
      -DDJVULIBRE_ENABLE_GTEST=OFF \
      -DCMAKE_INSTALL_PREFIX=$PWD/djvulibre-prefix
cmake --build djvulibre-build --parallel
cmake --install djvulibre-build

cmake -S DjView4Experimental -B djview-build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DDJVIEW_DJVU_INSTALL_DIR=$PWD/djvulibre-prefix
cmake --build djview-build --parallel
```

**Windows (PowerShell — requires `VCPKG_ROOT` set):**

```powershell
cmake -S DjVuLibreExperimental -B djvulibre-build `
      -DDJVULIBRE_USE_BUNDLED_DEPS=ON `
      -DDJVULIBRE_ENABLE_GTEST=OFF
cmake --build djvulibre-build --config Release --parallel
cmake --install djvulibre-build --config Release --prefix djvulibre-prefix

cmake -S DjView4Experimental -B djview-build `
      -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
      -DVCPKG_TARGET_TRIPLET=x64-windows `
      -DDJVIEW_DJVU_INSTALL_DIR="$PWD\djvulibre-prefix"
cmake --build djview-build --config Release --parallel
```

### 1.2 CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `DJVIEW_DJVU_INSTALL_DIR` | _(empty)_ | Path to a pre-built DjVuLibre install prefix. If empty, CMake searches the system / vcpkg. |
| `DJVIEW_BUILD_TRANSLATIONS` | `ON` | Build and install `.qm` translation files (requires Qt LinguistTools). |
| `DJVIEW_ENABLE_PLUGIN` | `ON` | Build with DDE/X11 viewer-protocol plugin support. |
| `DJVIEW_BUNDLE_DJVU_DEPS` | `OFF` | Copy DjVuLibre runtime libs into the install tree alongside djview. Use with CPack for a self-contained installer. |
| `DJVIEW_DJVU_SOURCE_DIR` | _(auto)_ | Path to the DjVuLibreExperimental source checkout (git-describe info for the About dialog). Auto-detected as `../DjVuLibreExperimental`. |

### 1.3 CMake presets

Ready-made presets are defined in `CMakePresets.json`. `VCPKG_ROOT` must be
set (or overridden in a local `CMakeUserPresets.json`) for the Windows presets.

**Windows** — uses the Visual Studio 18 2026 multi-config generator.
Configure once, build Debug or Release separately:

```powershell
cmake --preset windows-x64                  # configure
cmake --build --preset windows-x64-release  # Release
cmake --build --preset windows-x64-debug    # Debug
```

**Linux** — single-config Ninja presets:

```sh
cmake --preset linux-x64-release
cmake --build --preset linux-x64-release
```

Available configure presets: `windows-x64`, `windows-arm64`, `linux-x64-release`, `linux-x64-debug`

Available build presets: `windows-x64-release`, `windows-x64-debug`, `windows-arm64-release`, `linux-x64-release`, `linux-x64-debug`

### 1.4 Self-contained installer (Windows)

The CMake build integrates with CPack to generate a NSIS installer (`.exe`)
and a ZIP archive bundling `djview.exe`, all Qt DLLs, and the DjVuLibre
runtime DLL.

```powershell
cmake -S . -B build -DDJVIEW_BUNDLE_DJVU_DEPS=ON ...
cmake --build build --config Release --parallel
cmake --install build --config Release --prefix build/_install
cd build; cpack -G "ZIP;NSIS" -C Release
```

The resulting `DjView4-<version>-win64.exe` / `.zip` are placed in `build/`.

### 1.5 Linux system install

If DjVuLibre is already installed system-wide (`libdjvulibre-dev`):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
sudo cmake --install build
```

### 1.6 Building on macOS

Install Qt 6 and DjVuLibre via Homebrew:

```sh
brew install qt cmake ninja djvulibre
```

Then:

```sh
cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build --parallel
```

For a macOS app bundle, set `CMAKE_OSX_DEPLOYMENT_TARGET` and use
`CMAKE_INSTALL_PREFIX` to place djview into an `.app` skeleton, then run
`macdeployqt`.

---

## 2. Continuous integration

GitHub Actions workflows are in `.github/workflows/`:

| Workflow | Trigger | Description |
|----------|---------|-------------|
| `ci.yml` | push to `develop`/`master`, PRs | Builds Linux x64, Linux ARM64, Windows x64, Windows ARM64 in Debug + Release. Produces installers on Release builds. |
| `ci-manual.yml` | `workflow_dispatch` | Manual trigger — specify platform, build type, and a custom DjVuLibreExperimental ref to test against unreleased library branches. |
