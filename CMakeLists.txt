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

# ---------------------------------------------------------------------------
# Optional: bundle DjVuLibre runtime into the install tree
# ---------------------------------------------------------------------------
# Set DJVIEW_BUNDLE_DJVU_DEPS=ON together with DJVIEW_DJVU_INSTALL_DIR when
# building a standalone installer so the DjVuLibre shared library (and any
# of its own shared dependencies: libjpeg, libtiff, zlib on Windows) are
# copied next to djview.exe / djview.
#
option(DJVIEW_BUNDLE_DJVU_DEPS
    "Copy DjVuLibre runtime (and its deps) from DJVIEW_DJVU_INSTALL_DIR into the install tree"
    OFF)

if(DJVIEW_BUNDLE_DJVU_DEPS AND DJVIEW_DJVU_INSTALL_DIR)
    if(WIN32)
        file(GLOB _djvu_runtime_dlls
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/libdjvulibre.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/libjpeg*.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/libtiff*.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/zlib*.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/zlibd*.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/lzma*.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/deflatd*.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/deflate*.dll"
             "${DJVIEW_DJVU_INSTALL_DIR}/bin/lerc*.dll")
        if(_djvu_runtime_dlls)
            install(FILES ${_djvu_runtime_dlls}
                    DESTINATION "${CMAKE_INSTALL_BINDIR}"
                    COMPONENT Runtime)
        endif()
    else()
        file(GLOB _djvu_runtime_libs
             "${DJVIEW_DJVU_INSTALL_DIR}/lib/libdjvulibre.so*"
             "${DJVIEW_DJVU_INSTALL_DIR}/lib/libdjvulibre*.dylib")
        if(_djvu_runtime_libs)
            install(FILES ${_djvu_runtime_libs}
                    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                    COMPONENT Runtime)
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# Qt deployment (Windows only): run windeployqt at install time so all
# required Qt DLLs / plugins end up next to djview.exe.
# This is invoked automatically when building the CPack package.
# ---------------------------------------------------------------------------
if(WIN32 AND QT_VERSION_MAJOR EQUAL 6)
    # Locate windeployqt – it lives under the active vcpkg triplet's tool dir.
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt6
        HINTS
            "$ENV{VCPKG_ROOT}/installed/x64-windows-release/tools/qt6/bin"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/tools/qt6/bin"
            "$ENV{VCPKG_ROOT}/installed/arm64-windows-release/tools/qt6/bin"
            "$ENV{VCPKG_ROOT}/installed/arm64-windows/tools/qt6/bin"
            "${Qt6_DIR}/../../../bin"
    )

    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "DjView4: found windeployqt at ${WINDEPLOYQT_EXECUTABLE}")
        # The install(CODE) block runs at 'cmake --install' / 'cpack' time.
        install(CODE "
            message(STATUS \"Running windeployqt...\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --no-translations
                    --no-compiler-runtime
                    \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/djview.exe\"
                RESULT_VARIABLE _wdqt_result
            )
            if(NOT _wdqt_result EQUAL 0)
                message(WARNING \"windeployqt returned \${_wdqt_result}\")
            endif()
        " COMPONENT Runtime)
    else()
        message(STATUS "DjView4: windeployqt not found – Qt DLLs will NOT be deployed automatically")
    endif()
endif()

# ---------------------------------------------------------------------------
# CPack – combined installer
# ---------------------------------------------------------------------------
# Windows: generates both a NSIS installer (.exe) and a ZIP archive.
# Linux:   generates a TGZ archive.
#
# Workflow to produce a combined DjView4 + DjVuLibre installer:
#
#   cmake -S . -B build \
#         -DDJVIEW_DJVU_INSTALL_DIR=<djvulibre-prefix> \
#         -DDJVIEW_BUNDLE_DJVU_DEPS=ON
#   cmake --build build --config Release
#   cmake --install build --config Release --prefix build/_pkg
#   cd build && cpack -G NSIS   (Windows)
#              cpack -G TGZ    (Linux)
#
set(CPACK_PACKAGE_NAME              "DjView4")
set(CPACK_PACKAGE_VENDOR            "DjVuLibre")
set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "DjView4 – DjVu document viewer")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "DjView4")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME           "DjView4 ${PROJECT_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME           "DjView4")
    set(CPACK_NSIS_MODIFY_PATH            ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    # Register .djvu / .djv file association
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS [[
      WriteRegStr HKCR ".djvu" "" "DjVu.Document"
      WriteRegStr HKCR ".djv"  "" "DjVu.Document"
      WriteRegStr HKCR "DjVu.Document" "" "DjVu Document"
      WriteRegStr HKCR "DjVu.Document\shell\open\command" "" '"$INSTDIR\bin\djview.exe" "%1"'
    ]])
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS [[
      DeleteRegKey HKCR ".djvu"
      DeleteRegKey HKCR ".djv"
      DeleteRegKey HKCR "DjVu.Document"
    ]])
else()
    set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_SOURCE_IGNORE_FILES
    "/build/;/build-.*/;/\\\\.git/;/autom4te\\\\.cache/;/vcpkg/")

include(CPack)
