# cmake/RunWinDeployQt.cmake
# Called from a POST_BUILD custom command.
# Runs windeployqt and ignores non-zero exit codes caused by warnings
# (e.g. "Translations will not be available").
#
# Variables expected (passed via -D on the command line):
#   WINDEPLOYQT   – full path to windeployqt.exe
#   TARGET_EXE    – full path to the target executable
#   QT_BINDIR     – directory where Qt DLLs live (release or debug/bin)
#   QT_PLUGINDIR  – directory where Qt plugins live

if(NOT WINDEPLOYQT)
    message(WARNING "RunWinDeployQt: WINDEPLOYQT not set – skipping")
    return()
endif()

if(NOT TARGET_EXE)
    message(WARNING "RunWinDeployQt: TARGET_EXE not set – skipping")
    return()
endif()

message(STATUS "Running windeployqt: ${TARGET_EXE}")

set(_args
    --no-translations
    --no-compiler-runtime
)

# --dir   = where to WRITE plugins/DLLs (same folder as exe)
get_filename_component(_exe_dir "${TARGET_EXE}" DIRECTORY)
list(APPEND _args --dir "${_exe_dir}")

if(QT_BINDIR)
    list(APPEND _args --libdir   "${QT_BINDIR}")
endif()
if(QT_PLUGINDIR)
    list(APPEND _args --plugindir "${_exe_dir}")
endif()

list(APPEND _args "${TARGET_EXE}")

execute_process(
    COMMAND "${WINDEPLOYQT}" ${_args}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE  _errout
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
)

if(_output)
    message(STATUS "windeployqt: ${_output}")
endif()
if(_errout)
    message(STATUS "windeployqt (stderr): ${_errout}")
endif()
if(NOT _result EQUAL 0)
    message(STATUS "windeployqt exited with code ${_result} (treated as non-fatal)")
endif()
