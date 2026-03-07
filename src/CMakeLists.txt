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
