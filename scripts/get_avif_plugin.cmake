# Download and integrate the AVIF plugin for Qt6 (Windows).

if(NOT DEFINED TARGET_DIR OR TARGET_DIR STREQUAL "")
  message(FATAL_ERROR "TARGET_DIR is required")
endif()

set(AVIF_PLUGIN_URL
    "https://github.com/novomesk/qt-avif-image-plugin/releases/download/v0.10.1/qavif6.dll")
set(AVIF_PLUGIN_PATH "${TARGET_DIR}/qavif6.dll")

file(MAKE_DIRECTORY "${TARGET_DIR}")

if(EXISTS "${AVIF_PLUGIN_PATH}")
  message(STATUS "qavif6.dll already exists in ${TARGET_DIR}")
  return()
endif()

file(DOWNLOAD
     "${AVIF_PLUGIN_URL}"
     "${AVIF_PLUGIN_PATH}"
     STATUS download_status
     SHOW_PROGRESS
     TLS_VERIFY ON)

list(GET download_status 0 download_code)
list(GET download_status 1 download_message)
if(NOT download_code EQUAL 0)
  file(REMOVE "${AVIF_PLUGIN_PATH}")
  message(FATAL_ERROR "Failed to download qavif6.dll: ${download_message}")
endif()

message(STATUS "qavif6.dll downloaded to ${TARGET_DIR}")
