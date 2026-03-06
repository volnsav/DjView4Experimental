set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

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
if(TARGET DjVuLibre::DjVuLibre)
    target_link_libraries(djview PRIVATE DjVuLibre::DjVuLibre)
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

if(DJVIEW_BUILD_TRANSLATIONS AND QM_FILES)
    install(FILES ${QM_FILES}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/djview4/translations"
    )
endif()
