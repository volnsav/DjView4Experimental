set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# build_git_info.h  – generated at every build from git describe
# ---------------------------------------------------------------------------
# Optional: set DJVIEW_DJVU_SOURCE_DIR to the DjVuLibreExperimental checkout;
# if not set we try the sibling directory ../DjVuLibreExperimental.
if(NOT DJVIEW_DJVU_SOURCE_DIR)
    get_filename_component(_djvu_sibling
        "${CMAKE_SOURCE_DIR}/../DjVuLibreExperimental" ABSOLUTE)
    if(IS_DIRECTORY "${_djvu_sibling}")
        set(DJVIEW_DJVU_SOURCE_DIR "${_djvu_sibling}")
    endif()
endif()

set(_git_info_out "${CMAKE_CURRENT_BINARY_DIR}/build_git_info.h")
set(_git_info_script "${CMAKE_SOURCE_DIR}/cmake/GenerateBuildGitInfo.cmake")

# Generate build_git_info.h at configure time.
# Re-run cmake --preset ... to refresh git refs after a branch switch.
execute_process(
    COMMAND "${CMAKE_COMMAND}"
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DDJVU_SOURCE_DIR=${DJVIEW_DJVU_SOURCE_DIR}
            -DOUTPUT_FILE=${_git_info_out}
            -P ${_git_info_script}
    RESULT_VARIABLE _git_info_result)
if(NOT _git_info_result EQUAL 0)
    # Fallback stub if the script fails (e.g. git not available)
    file(WRITE "${_git_info_out}"
        "#ifndef BUILD_GIT_INFO_H\n#define BUILD_GIT_INFO_H\n"
        "#define DJVIEW_APP_GIT_REF \"unknown\"\n"
        "#define DJVIEW_LIB_GIT_REF \"unknown\"\n"
        "#endif\n")
endif()

# Dummy target so add_dependencies(djview djview_git_info) still compiles.
add_custom_target(djview_git_info)
# Make the generated header visible to sources
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# ---------------------------------------------------------------------------
# Sources identical to those in djview.pro
# ---------------------------------------------------------------------------
set(DJVIEW_SOURCES
    # djvu engine
    qdjvu.cpp
    qdjvunet.cpp
    qdjvuwidget.cpp
    qdjvuwidget.qrc
    # main application
    qdjviewprefs.cpp
    qdjviewsidebar.cpp
    qdjviewdialogs.cpp
    qdjviewexporters.cpp
    qdjview.cpp
    djview.cpp
    qdjview.qrc
    # UI forms
    qdjviewerrordialog.ui
    qdjviewauthdialog.ui
    qdjviewinfodialog.ui
    qdjviewmetadialog.ui
    qdjviewsavedialog.ui
    qdjviewprintdialog.ui
    qdjviewexportdialog.ui
    qdjviewexportps1.ui
    qdjviewexportps2.ui
    qdjviewexportps3.ui
    qdjviewexporttiff.ui
    qdjviewexportprn.ui
    qdjviewexportpdf.ui
    qdjviewprefsdialog.ui
    # helper C file
    tiff2pdf.c
)

# Plugin source (enabled for Qt5+ which includes Qt6)
if(DJVIEW_ENABLE_PLUGIN)
    list(APPEND DJVIEW_SOURCES qdjviewplugin.cpp)
endif()

# Windows resource file
if(WIN32)
    list(APPEND DJVIEW_SOURCES djview.rc)
endif()

# ---------------------------------------------------------------------------
# Translation files (.ts -> .qm)
# ---------------------------------------------------------------------------
set(TS_FILES
    djview_fr.ts
    djview_uk.ts
    djview_de.ts
    djview_cs.ts
    djview_ru.ts
    djview_es.ts
    djview_zh_cn.ts
    djview_zh_tw.ts
)

if(DJVIEW_BUILD_TRANSLATIONS)
    if(QT_VERSION_MAJOR EQUAL 6)
        qt6_add_translation(QM_FILES ${TS_FILES})
    else()
        qt5_add_translation(QM_FILES ${TS_FILES})
    endif()
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(djview WIN32 ${DJVIEW_SOURCES})
add_dependencies(djview djview_git_info)

if(DJVIEW_BUILD_TRANSLATIONS AND QM_FILES)
    # Make sure translations are built as part of the main target.
    target_sources(djview PRIVATE ${QM_FILES})
endif()

# ---------------------------------------------------------------------------
# Qt modules
# ---------------------------------------------------------------------------
target_link_libraries(djview PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# OpenGL acceleration (optional – renderer falls back to Raster if not found)
find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS OpenGL OpenGLWidgets)
if(Qt${QT_VERSION_MAJOR}OpenGLWidgets_FOUND)
    target_link_libraries(djview PRIVATE
        Qt${QT_VERSION_MAJOR}::OpenGL
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    )
elseif(Qt${QT_VERSION_MAJOR}OpenGL_FOUND)
    target_link_libraries(djview PRIVATE Qt${QT_VERSION_MAJOR}::OpenGL)
endif()

# ---------------------------------------------------------------------------
# DjVuLibre
# ---------------------------------------------------------------------------
# DjVuLibreExperimental CMake config exports DjVuLibre::djvulibre (lowercase).
# The pkg-config fallback in the root CMakeLists creates DjVuLibre::DjVuLibre.
# Handle both so the build works regardless of how DjVuLibre was found.
if(TARGET DjVuLibre::DjVuLibre)
    target_link_libraries(djview PRIVATE DjVuLibre::DjVuLibre)
elseif(TARGET DjVuLibre::djvulibre)
    target_link_libraries(djview PRIVATE DjVuLibre::djvulibre)
elseif(TARGET djvulibre)
    target_link_libraries(djview PRIVATE djvulibre)
endif()

# Internal DjVuLibre headers (DjVuImage.h, DjVuPalette.h, JB2Image.h, ...)
# needed for ddjvu_get_DjVuImage() backdoor and direct palette access.
if(DJVIEW_DJVU_SOURCE_DIR)
    target_include_directories(djview PRIVATE
        "${DJVIEW_DJVU_SOURCE_DIR}/libdjvu"
    )
    # DjVuLibre wraps classes in namespace DJVU when HAVE_NAMESPACES is set.
    # The DLL was built with it, so we must match.
    target_compile_definitions(djview PRIVATE HAVE_NAMESPACES=1)
endif()

# ---------------------------------------------------------------------------
# libtiff – multi-page TIFF export with G4/JPEG/Deflate compression.
# ---------------------------------------------------------------------------
find_package(TIFF REQUIRED)
target_link_libraries(djview PRIVATE TIFF::TIFF)
target_compile_definitions(djview PRIVATE HAVE_TIFF=1)

# ---------------------------------------------------------------------------
# libjpeg-turbo (direct grayscale JPEG encoding bypassing Qt plugin)
# ---------------------------------------------------------------------------
find_package(JPEG REQUIRED)
target_link_libraries(djview PRIVATE JPEG::JPEG)

# ---------------------------------------------------------------------------
# jbig2enc – JBIG2 encoding for 1bpp B&W pages (much better than JPEG/G4).
# Fetched from upstream at configure time; Leptonica supplied via vcpkg.
# ---------------------------------------------------------------------------
include(FetchContent)
find_package(Leptonica REQUIRED)   # vcpkg supplies this (leptonica port)
FetchContent_Declare(jbig2enc
    GIT_REPOSITORY https://github.com/agl/jbig2enc
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(jbig2enc)
target_link_libraries(djview PRIVATE libjbig2enc ${Leptonica_LIBRARIES})
target_include_directories(djview PRIVATE
    "${jbig2enc_SOURCE_DIR}/src"
    ${Leptonica_INCLUDE_DIRS}
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(djview PRIVATE
    $<$<CONFIG:Release>:NDEBUG QT_NO_DEBUG QT_NO_DEBUG_STREAM>
)

if(DJVIEW_ENABLE_PLUGIN)
    target_compile_definitions(djview PRIVATE WITH_DJVIEWPLUGIN)
endif()

# Include own source dir for generated UI headers.
target_include_directories(djview PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Post-build deployment (Windows only)
# Runs automatically after every build so the output dir is self-contained.
# ---------------------------------------------------------------------------
if(WIN32)
    # Disable vcpkg MSBuild applocal.ps1 – we handle all DLL deployment
    # ourselves via TARGET_RUNTIME_DLLS + RunWinDeployQt.cmake below.
    # Without this, applocal.ps1 fails on libdjvulibre.dll (not in vcpkg)
    # and aborts the build before our own POST_BUILD steps run.
    set_property(TARGET djview PROPERTY VS_GLOBAL_VcpkgApplocalDeps "false")

    # 1. Copy all transitive runtime DLLs (libdjvulibre, vcpkg Qt DLLs, etc.)
    #    $<TARGET_RUNTIME_DLLS:djview> requires CMake >= 3.21 (our minimum).
    add_custom_command(TARGET djview POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:djview>
            $<TARGET_FILE_DIR:djview>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs -> $<TARGET_FILE_DIR:djview>"
    )

    # 2. windeployqt – adds platforms/, imageformats/, styles/, etc.
    #    Invoked via a cmake -P script so that windeployqt's non-zero exit code
    #    for non-fatal warnings (e.g. missing translations catalog in vcpkg Qt)
    #    does NOT abort the build.
    if(WINDEPLOYQT_EXECUTABLE)
        # Derive the vcpkg Qt6/plugins directory from windeployqt location.
        # windeployqt lives at:  <vcpkg>/tools/Qt6/bin/windeployqt.exe
        # plugins live at:       <vcpkg>/Qt6/plugins/
        get_filename_component(_vcpkg_root "${WINDEPLOYQT_EXECUTABLE}" DIRECTORY) # tools/Qt6/bin
        get_filename_component(_vcpkg_root "${_vcpkg_root}/../../.." ABSOLUTE)    # vcpkg triplet root
        # Release plugins:  <vcpkg>/Qt6/plugins/
        # Debug plugins:    <vcpkg>/debug/Qt6/plugins/
        # Qt debug runtime verifies that loaded plugins are also debug builds,
        # so we must select the right directory based on build config.
        set(_qt_plugins_rel "${_vcpkg_root}/Qt6/plugins")
        set(_qt_plugins_dbg "${_vcpkg_root}/debug/Qt6/plugins")

        # Copy only the plugin subdirs that djview actually needs.
        foreach(_plugin_subdir
            platforms      # mandatory: qwindows(d).dll
            imageformats   # JPEG, PNG, GIF support
            styles         # QModernWindowsStyle
            tls            # HTTPS / network TLS
            networkinformation
        )
            # Choose debug or release plugin source dir at build time.
            # IS_DIRECTORY is configure-time; we check the release dir as proxy
            # (both exist if Qt is installed).
            if(IS_DIRECTORY "${_qt_plugins_rel}/${_plugin_subdir}")
                add_custom_command(TARGET djview POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E copy_directory
                        "$<IF:$<CONFIG:Debug>,${_qt_plugins_dbg},${_qt_plugins_rel}>/${_plugin_subdir}"
                        "$<TARGET_FILE_DIR:djview>/${_plugin_subdir}"
                    COMMENT "Deploying Qt plugin ($<CONFIG>): ${_plugin_subdir}"
                )
            endif()
        endforeach()
        # jpeg62.dll is a transitive dependency of qjpeg(d).dll but is NOT
        # picked up by TARGET_RUNTIME_DLLS (loaded at runtime by the plugin).
        # Copy it explicitly so the JPEG imageformat plugin can load.
        add_custom_command(TARGET djview POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${_vcpkg_root}/debug/bin,${_vcpkg_root}/bin>/jpeg62.dll"
                "$<TARGET_FILE_DIR:djview>/jpeg62.dll"
            COMMENT "Deploying jpeg62.dll ($<CONFIG>)"
        )

        # qwebp loads extra WebP codec DLLs at runtime; copy them explicitly
        # because they are plugin dependencies, not direct djview dependencies.
        add_custom_command(TARGET djview POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${_vcpkg_root}/debug/bin,${_vcpkg_root}/bin>/libwebpdecoder.dll"
                "$<IF:$<CONFIG:Debug>,${_vcpkg_root}/debug/bin,${_vcpkg_root}/bin>/libwebpdemux.dll"
                "$<TARGET_FILE_DIR:djview>"
            COMMENT "Deploying WebP plugin runtime DLLs ($<CONFIG>)"
        )

        # Download qavif6.dll after building djview (Windows only)
        if(WIN32)
            add_custom_command(TARGET djview POST_BUILD
                COMMAND "${CMAKE_COMMAND}"
                    -DTARGET_DIR="$<TARGET_FILE_DIR:djview>/imageformats"
                    -P "${CMAKE_SOURCE_DIR}/cmake/GetAvifPlugin.cmake"
                COMMENT "Downloading qavif6.dll (AVIF plugin)"
            )
        endif()

        unset(_vcpkg_root)
        unset(_qt_plugins_rel)
        unset(_qt_plugins_dbg)
    else()
        message(WARNING "windeployqt not found – Qt plugins will NOT be deployed to the build dir")
    endif()
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS djview
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/djview.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
)

if(DJVIEW_BUILD_TRANSLATIONS AND QM_FILES)
    install(FILES ${QM_FILES}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/djview4/translations"
    )
endif()
