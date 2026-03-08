find_package(Leptonica CONFIG QUIET)
if(Leptonica_FOUND)
  if(TARGET leptonica AND NOT TARGET Leptonica::Leptonica)
    add_library(Leptonica::Leptonica ALIAS leptonica)
  endif()
  return()
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(LEPTONICA QUIET lept)
endif()

if(LEPTONICA_FOUND)
  set(Leptonica_FOUND TRUE)
  set(Leptonica_VERSION "${LEPTONICA_VERSION}")
  set(Leptonica_INCLUDE_DIRS ${LEPTONICA_INCLUDE_DIRS})
  set(Leptonica_LIBRARY_DIRS ${LEPTONICA_LIBRARY_DIRS})
  set(Leptonica_LIBRARIES ${LEPTONICA_LINK_LIBRARIES})

  if(NOT TARGET Leptonica::Leptonica)
    add_library(Leptonica::Leptonica INTERFACE IMPORTED)
    set_target_properties(Leptonica::Leptonica PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LEPTONICA_INCLUDE_DIRS}"
      INTERFACE_LINK_DIRECTORIES "${LEPTONICA_LIBRARY_DIRS}"
      INTERFACE_LINK_LIBRARIES "${LEPTONICA_LINK_LIBRARIES}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Leptonica
  REQUIRED_VARS Leptonica_INCLUDE_DIRS Leptonica_LIBRARIES
  VERSION_VAR Leptonica_VERSION)
