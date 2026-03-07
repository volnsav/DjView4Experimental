# GenerateBuildGitInfo.cmake
# Called at build time (add_custom_command) to produce build_git_info.h
# in the current binary directory.
#
# Required variables (passed via -D):
#   SOURCE_DIR       - repository root of DjView4Experimental
#   DJVU_SOURCE_DIR  - path to DjVuLibreExperimental (may be empty / missing)
#   OUTPUT_FILE      - absolute path to the generated build_git_info.h

cmake_minimum_required(VERSION 3.21)

function(get_git_ref repo_dir out_var)
    if(NOT IS_DIRECTORY "${repo_dir}/.git")
        set(${out_var} "unknown" PARENT_SCOPE)
        return()
    endif()
    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        set(${out_var} "unknown" PARENT_SCOPE)
        return()
    endif()
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${repo_dir}" rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE _branch
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${repo_dir}" rev-parse --short HEAD
        OUTPUT_VARIABLE _sha
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(_branch AND _sha)
        set(${out_var} "${_branch}+${_sha}" PARENT_SCOPE)
    elseif(_sha)
        set(${out_var} "${_sha}" PARENT_SCOPE)
    else()
        set(${out_var} "unknown" PARENT_SCOPE)
    endif()
endfunction()

get_git_ref("${SOURCE_DIR}"      _app_ref)
get_git_ref("${DJVU_SOURCE_DIR}" _lib_ref)

file(WRITE "${OUTPUT_FILE}"
"#ifndef BUILD_GIT_INFO_H
#define BUILD_GIT_INFO_H
#define DJVIEW_APP_GIT_REF \"${_app_ref}\"
#define DJVIEW_LIB_GIT_REF \"${_lib_ref}\"
#endif
")
message(STATUS "Generated ${OUTPUT_FILE}: app=${_app_ref} lib=${_lib_ref}")
