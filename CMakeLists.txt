cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# vcpkg auto-detection (must be set BEFORE project())
# ---------------------------------------------------------------------------
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file (auto-detected from VCPKG_ROOT)")
    message(STATUS "DjView4: auto-set CMAKE_TOOLCHAIN_FILE from VCPKG_ROOT")
endif()

project(DjView4 VERSION 4.12.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(DJVIEW_ENABLE_PLUGIN
    "Build with the DjView plugin (DDE/X11 viewer protocol) support" ON)
option(DJVIEW_BUILD_TRANSLATIONS
    "Compile Qt translation files (.ts -> .qm)" ON)

# Where to look for a prebuilt DjVuLibre install (adds to CMAKE_PREFIX_PATH).
# If empty, CMake will use the standard search paths (system install or vcpkg).
set(DJVIEW_DJVU_INSTALL_DIR ""
    CACHE PATH "Optional path to a prebuilt DjVuLibreExperimental install prefix")

# ---------------------------------------------------------------------------
# Propagate DjVuLibre prefix
# ---------------------------------------------------------------------------
if(DJVIEW_DJVU_INSTALL_DIR AND NOT DJVIEW_DJVU_INSTALL_DIR STREQUAL "")
    list(PREPEND CMAKE_PREFIX_PATH "${DJVIEW_DJVU_INSTALL_DIR}")
endif()

# ---------------------------------------------------------------------------
# Find Qt6 (or Qt5 as fallback)
# ---------------------------------------------------------------------------
find_package(Qt6 QUIET COMPONENTS Core)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets PrintSupport)
    if(DJVIEW_BUILD_TRANSLATIONS)
        find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
    endif()
    message(STATUS "DjView4: using Qt ${Qt6_VERSION}")
else()
    set(QT_VERSION_MAJOR 5)
    find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets PrintSupport)
    if(DJVIEW_BUILD_TRANSLATIONS)
        find_package(Qt5 REQUIRED COMPONENTS LinguistTools)
    endif()
    message(STATUS "DjView4: using Qt ${Qt5_VERSION}")
endif()

# ---------------------------------------------------------------------------
# Find DjVuLibre
# ---------------------------------------------------------------------------
# Try the installed CMake config first (DjVuLibreExperimental installs one).
find_package(DjVuLibre CONFIG QUIET)
if(NOT DjVuLibre_FOUND)
    # Fall back to pkg-config (system DjVuLibre on Linux).
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(djvulibre IMPORTED_TARGET djvulibre)
        if(djvulibre_FOUND)
            # Expose a proper target alias so src/CMakeLists.txt is uniform.
            add_library(DjVuLibre::DjVuLibre INTERFACE IMPORTED)
            target_link_libraries(DjVuLibre::DjVuLibre
                INTERFACE PkgConfig::djvulibre)
            set(DjVuLibre_FOUND TRUE)
        endif()
    endif()
endif()

if(NOT DjVuLibre_FOUND)
    message(FATAL_ERROR
        "DjVuLibre not found.\n"
        "Options:\n"
        "  1. Install libdjvulibre-dev (Linux) or build DjVuLibreExperimental.\n"
        "  2. Pass -DDJVIEW_DJVU_INSTALL_DIR=<prefix> to point at the install.\n"
        "  3. Add the install prefix to CMAKE_PREFIX_PATH.")
endif()

message(STATUS "DjView4: DjVuLibre found")

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(src)
