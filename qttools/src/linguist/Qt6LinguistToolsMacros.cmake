#=============================================================================
# Copyright (C) 2020 The Qt Company Ltd.
# Copyright 2005-2011 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

include(CMakeParseArguments)

function(qt6_create_translation _qm_files)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_LUPDATE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(_lupdate_files ${_LUPDATE_UNPARSED_ARGUMENTS})
    set(_lupdate_options ${_LUPDATE_OPTIONS})

    list(FIND _lupdate_options "-extensions" _extensions_index)
    if(_extensions_index GREATER -1)
        math(EXPR _extensions_index "${_extensions_index} + 1")
        list(GET _lupdate_options ${_extensions_index} _extensions_list)
        string(REPLACE "," ";" _extensions_list "${_extensions_list}")
        list(TRANSFORM _extensions_list STRIP)
        list(TRANSFORM _extensions_list REPLACE "^\\." "")
        list(TRANSFORM _extensions_list PREPEND "*.")
    else()
        set(_extensions_list "*.java;*.jui;*.ui;*.c;*.c++;*.cc;*.cpp;*.cxx;*.ch;*.h;*.h++;*.hh;*.hpp;*.hxx;*.js;*.qs;*.qml;*.qrc")
    endif()
    set(_my_sources)
    set(_my_tsfiles)
    foreach(_file ${_lupdate_files})
        get_filename_component(_ext ${_file} EXT)
        get_filename_component(_abs_FILE ${_file} ABSOLUTE)
        if(_ext MATCHES "ts")
            list(APPEND _my_tsfiles ${_abs_FILE})
        else()
            list(APPEND _my_sources ${_abs_FILE})
        endif()
    endforeach()
    set(stamp_file_dir "${CMAKE_CURRENT_BINARY_DIR}/.lupdate")
    if(NOT EXISTS "${stamp_file_dir}")
        file(MAKE_DIRECTORY "${stamp_file_dir}")
    endif()
    foreach(_ts_file ${_my_tsfiles})
        get_filename_component(_ts_name ${_ts_file} NAME)
        if(_my_sources)
          # make a list file to call lupdate on, so we don't make our commands too
          # long for some systems
          set(_ts_lst_file "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_ts_name}_lst_file")
          set(_lst_file_srcs)
          set(_dependencies)
          foreach(_lst_file_src ${_my_sources})
              set(_lst_file_srcs "${_lst_file_src}\n${_lst_file_srcs}")
              if(IS_DIRECTORY ${_lst_file_src})
                  list(TRANSFORM _extensions_list PREPEND "${_lst_file_src}/" OUTPUT_VARIABLE _directory_glob)
                  file(GLOB_RECURSE _directory_contents CONFIGURE_DEPENDS ${_directory_glob})
                  list(APPEND _dependencies ${_directory_contents})
              else()
                  list(APPEND _dependencies "${_lst_file_src}")
              endif()
          endforeach()

          get_directory_property(_inc_DIRS INCLUDE_DIRECTORIES)
          foreach(_pro_include ${_inc_DIRS})
              get_filename_component(_abs_include "${_pro_include}" ABSOLUTE)
              set(_lst_file_srcs "-I${_pro_include}\n${_lst_file_srcs}")
          endforeach()

          file(WRITE ${_ts_lst_file} "${_lst_file_srcs}")
        endif()
        set(stamp_file "${stamp_file_dir}/${_ts_name}.stamp")
        add_custom_command(OUTPUT ${stamp_file}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate
            ARGS ${_lupdate_options} "@${_ts_lst_file}" -ts ${_ts_file}
            COMMAND ${CMAKE_COMMAND} -E touch "${stamp_file}"
            DEPENDS ${_dependencies}
            VERBATIM)
    endforeach()
    qt6_add_translation(${_qm_files} __QT_INTERNAL_DEPEND_ON_TIMESTAMP_FILE ${_my_tsfiles})
    set(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
endfunction()

function(qt6_add_translation _qm_files)
    set(options __QT_INTERNAL_DEPEND_ON_TIMESTAMP_FILE)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_LRELEASE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(_lrelease_files ${_LRELEASE_UNPARSED_ARGUMENTS})

    foreach(_current_FILE ${_lrelease_files})
        get_filename_component(_abs_FILE ${_current_FILE} ABSOLUTE)
        get_filename_component(qm ${_abs_FILE} NAME)
        set(ts_stamp_file "${CMAKE_CURRENT_BINARY_DIR}/.lupdate/${qm}.stamp")
        # everything before the last dot has to be considered the file name (including other dots)
        string(REGEX REPLACE "\\.[^.]*$" "" FILE_NAME ${qm})
        get_source_file_property(output_location ${_abs_FILE} OUTPUT_LOCATION)
        if(output_location)
            file(MAKE_DIRECTORY "${output_location}")
            set(qm "${output_location}/${FILE_NAME}.qm")
        else()
            set(qm "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.qm")
        endif()

        if(_LRELEASE___QT_INTERNAL_DEPEND_ON_TIMESTAMP_FILE)
            set(qm_dep "${ts_stamp_file}")
        else()
            set(qm_dep "${_abs_FILE}")
        endif()

        add_custom_command(OUTPUT ${qm}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lrelease
            ARGS ${_LRELEASE_OPTIONS} ${_abs_FILE} -qm ${qm}
            DEPENDS ${qm_dep} VERBATIM
        )
        list(APPEND ${_qm_files} ${qm})
    endforeach()
    set(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
endfunction()

# Makes the paths in the unparsed arguments absolute and stores them in out_var.
function(qt_internal_make_paths_absolute out_var)
    set(result "")
    foreach(path IN LISTS ARGN)
        get_filename_component(abs_path "${path}" ABSOLUTE)
        list(APPEND result "${abs_path}")
    endforeach()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

# Needed to locate Qt6LupdateProject.json.in file inside functions
set(_Qt6_LINGUIST_TOOLS_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

function(qt6_add_lupdate target)
    set(options
        NO_GLOBAL_TARGET)
    set(oneValueArgs)
    set(multiValueArgs
        TS_FILES
        SOURCES
        INCLUDE_DIRECTORIES
        OPTIONS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(arg_INCLUDE_DIRECTORIES)
        qt_internal_make_paths_absolute(includePaths "${arg_INCLUDE_DIRECTORIES}")
    else()
        set(includePaths "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
    endif()
    if(arg_SOURCES)
        qt_internal_make_paths_absolute(sources "${arg_SOURCES}")
    else()
        set(sources "$<TARGET_PROPERTY:${target},SOURCES>")
    endif()

    qt_internal_make_paths_absolute(ts_files "${arg_TS_FILES}")

    set(lupdate_project_base "${CMAKE_CURRENT_BINARY_DIR}/.lupdate/${target}_project")
    set(lupdate_project_cmake "${lupdate_project_base}")
    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(multi_config)
        string(APPEND lupdate_project_cmake ".$<CONFIG>")
    endif()
    string(APPEND lupdate_project_cmake ".cmake")
    set(lupdate_project_json "${lupdate_project_base}.json")
    file(GENERATE OUTPUT "${lupdate_project_cmake}"
        CONTENT "set(lupdate_project_file \"${CMAKE_CURRENT_LIST_FILE}\")
set(lupdate_include_paths \"${includePaths}\")
set(lupdate_sources \"${sources}\")
set(lupdate_translations \"${ts_files}\")
")

    add_custom_target(${target}_lupdate
        COMMAND "${CMAKE_COMMAND}" "-DIN_FILE=${lupdate_project_cmake}"
                "-DOUT_FILE=${lupdate_project_json}"
                -P "${_Qt6_LINGUIST_TOOLS_DIR}/GenerateLUpdateProject.cmake"
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate -project "${lupdate_project_json}"
                ${arg_OPTIONS}
        DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate
        VERBATIM)

    if(NOT DEFINED QT_GLOBAL_LUPDATE_TARGET)
        set(QT_GLOBAL_LUPDATE_TARGET update_translations)
    endif()

    if(NOT arg_NO_GLOBAL_TARGET)
        if(NOT TARGET ${QT_GLOBAL_LUPDATE_TARGET})
            add_custom_target(${QT_GLOBAL_LUPDATE_TARGET})
        endif()
        add_dependencies(${QT_GLOBAL_LUPDATE_TARGET} ${target}_lupdate)
    endif()
endfunction()

function(qt6_add_lrelease target)
    set(options
        NO_TARGET_DEPENDENCY
        NO_GLOBAL_TARGET)
    set(oneValueArgs
        QM_FILES_OUTPUT_VARIABLE)
    set(multiValueArgs
        TS_FILES
        OPTIONS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    qt_internal_make_paths_absolute(ts_files "${arg_TS_FILES}")

    set(qm_files "")
    foreach(ts_file ${ts_files})
        if(NOT EXISTS "${ts_file}")
            message(WARNING "Translation file '${ts_file}' does not exist. "
                "Consider building the target '${target}_lupdate' to create an initial "
                "version of that file.")

            # Provide a command that creates an initial .ts file with the right language set.
            # The language is guessed by lupdate from the file name.
            add_custom_command(OUTPUT ${ts_file}
                COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate -ts ${ts_file}
                VERBATIM)
        endif()
        get_filename_component(qm ${ts_file} NAME_WLE)
        string(APPEND qm ".qm")
        get_source_file_property(output_location ${ts_file} OUTPUT_LOCATION)
        if(output_location)
            if(NOT IS_ABSOLUTE "${output_location}")
                get_filename_component(output_location "${output_location}" ABSOLUTE
                    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
            endif()
            file(MAKE_DIRECTORY "${output_location}")
            string(PREPEND qm "${output_location}/")
        else()
            string(PREPEND qm "${CMAKE_CURRENT_BINARY_DIR}/")
        endif()
        add_custom_command(OUTPUT ${qm}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lrelease
            ${arg_OPTIONS} ${ts_file} -qm ${qm}
            DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::lrelease ${ts_file}
            VERBATIM)
        list(APPEND qm_files "${qm}")
    endforeach()

    add_custom_target(${target}_lrelease DEPENDS ${qm_files})
    if(NOT arg_NO_TARGET_DEPENDENCY)
        add_dependencies(${target} ${target}_lrelease)
    endif()

    if(NOT DEFINED QT_GLOBAL_LRELEASE_TARGET)
        set(QT_GLOBAL_LRELEASE_TARGET release_translations)
    endif()

    if(NOT arg_NO_GLOBAL_TARGET)
        if(NOT TARGET ${QT_GLOBAL_LRELEASE_TARGET})
            add_custom_target(${QT_GLOBAL_LRELEASE_TARGET})
        endif()
        add_dependencies(${QT_GLOBAL_LRELEASE_TARGET} ${target}_lrelease)
    endif()

    if(NOT "${arg_QM_FILES_OUTPUT_VARIABLE}" STREQUAL "")
        set("${arg_QM_FILES_OUTPUT_VARIABLE}" "${qm_files}" PARENT_SCOPE)
    endif()
endfunction()

# This function is currently in Technical Preview.
# It's signature and behavior might change.
function(qt6_add_translations target)
    set(options)
    set(oneValueArgs
        QM_FILES_OUTPUT_VARIABLE
        RESOURCE_PREFIX
        OUTPUT_TARGETS)
    set(multiValueArgs
        TS_FILES
        SOURCES
        INCLUDE_DIRECTORIES
        LUPDATE_OPTIONS
        LRELEASE_OPTIONS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(DEFINED arg_RESOURCE_PREFIX AND DEFINED arg_QM_FILES_OUTPUT_VARIABLE)
        message(FATAL_ERROR "QM_FILES_OUTPUT_VARIABLE cannot be specified "
            "together with RESOURCE_PREFIX.")
    endif()
    if(DEFINED arg_QM_FILES_OUTPUT_VARIABLE AND DEFINED arg_OUTPUT_TARGETS)
        message(FATAL_ERROR "OUTPUT_TARGETS cannot be specified "
            "together with QM_FILES_OUTPUT_VARIABLE.")
    endif()
    if(NOT DEFINED arg_RESOURCE_PREFIX AND NOT DEFINED arg_QM_FILES_OUTPUT_VARIABLE)
        set(arg_RESOURCE_PREFIX "/i18n")
    endif()

    qt6_add_lupdate(${target}
        TS_FILES "${arg_TS_FILES}"
        SOURCES "${arg_SOURCES}"
        INCLUDE_DIRECTORIES "${arg_INCLUDE_DIRECTORIES}"
        OPTIONS "${arg_LUPDATE_OPTIONS}")
    qt6_add_lrelease(${target}
        TS_FILES "${arg_TS_FILES}"
        QM_FILES_OUTPUT_VARIABLE qm_files
        OPTIONS "${arg_LRELEASE_OPTIONS}")
    if(NOT "${arg_RESOURCE_PREFIX}" STREQUAL "")
        qt6_add_resources(${target} "translations"
            PREFIX "${arg_RESOURCE_PREFIX}"
            BASE "${CMAKE_CURRENT_BINARY_DIR}"
            OUTPUT_TARGETS out_targets
            FILES ${qm_files})
        if(DEFINED arg_OUTPUT_TARGETS)
            set("${arg_OUTPUT_TARGETS}" "${out_targets}" PARENT_SCOPE)
        endif()
    endif()
    if(NOT "${arg_QM_FILES_OUTPUT_VARIABLE}" STREQUAL "")
        set("${arg_QM_FILES_OUTPUT_VARIABLE}" "${qm_files}" PARENT_SCOPE)
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_create_translation _qm_files)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_create_translation("${_qm_files}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_create_translation("${_qm_files}" ${ARGN})
        endif()
        set("${_qm_files}" "${${_qm_files}}" PARENT_SCOPE)
    endfunction()
    function(qt_add_translation _qm_files)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_translation("${_qm_files}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_translation("${_qm_files}" ${ARGN})
        endif()
        set("${_qm_files}" "${${_qm_files}}" PARENT_SCOPE)
    endfunction()
    function(qt_add_lupdate)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_lupdate(${ARGN})
        else()
            message(FATAL_ERROR "qt_add_lupdate() is only available in Qt 6.")
        endif()
    endfunction()
    function(qt_add_lrelease)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_lrelease(${ARGN})
            cmake_parse_arguments(PARSE_ARGV 1 arg "" "QM_FILES_OUTPUT_VARIABLE" "")
            if(arg_QM_FILES_OUTPUT_VARIABLE)
                set(${arg_QM_FILES_OUTPUT_VARIABLE} ${${arg_QM_FILES_OUTPUT_VARIABLE}} PARENT_SCOPE)
            endif()
        else()
            message(FATAL_ERROR "qt_add_lrelease() is only available in Qt 6.")
        endif()
    endfunction()
    function(qt_add_translations)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_translations(${ARGN})
            cmake_parse_arguments(PARSE_ARGV 1 arg "" "OUTPUT_TARGETS;QM_FILES_OUTPUT_VARIABLE" "")
            if(arg_OUTPUT_TARGETS)
                set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
            endif()
            if(arg_QM_FILES_OUTPUT_VARIABLE)
                set(${arg_QM_FILES_OUTPUT_VARIABLE} ${${arg_QM_FILES_OUTPUT_VARIABLE}} PARENT_SCOPE)
            endif()
        else()
            message(FATAL_ERROR "qt_add_translations() is only available in Qt 6.")
        endif()
    endfunction()
endif()
