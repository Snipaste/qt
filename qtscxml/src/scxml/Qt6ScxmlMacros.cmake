#
# Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
# Contact: https://www.qt.io/licensing/
#
# This file is part of the QtScxml module of the Qt Toolkit.
#
# $QT_BEGIN_LICENSE:LGPL$
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU Lesser General Public License Usage
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 3 as published by the Free Software
# Foundation and appearing in the file LICENSE.LGPL3 included in the
# packaging of this file. Please review the following information to
# ensure the GNU Lesser General Public License version 3 requirements
# will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 2.0 or (at your option) the GNU General
# Public license version 3 or any later version approved by the KDE Free
# Qt Foundation. The licenses are as published by the Free Software
# Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-2.0.html and
# https://www.gnu.org/licenses/gpl-3.0.html.
#
# $QT_END_LICENSE$

# qt6_add_statecharts(target_or_outfiles inputfile ... )

function(qt6_add_statecharts target_or_outfiles)
    set(options)
    set(oneValueArgs OUTPUT_DIR OUTPUT_DIRECTORY NAMESPACE)
    set(multiValueArgs QSCXMLC_ARGUMENTS OPTIONS)

    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(scxml_files ${ARGS_UNPARSED_ARGUMENTS})
    set(outfiles)

    if (ARGS_NAMESPACE)
        set(namespace "--namespace" ${ARGS_NAMESPACE})
    endif()

    if (ARGS_OUTPUT_DIR)
        message(AUTHOR_WARNING
            "OUTPUT_DIR is deprecated. Please use OUTPUT_DIRECTORY instead.")
        set(ARGS_OUTPUT_DIRECTORY ${ARGS_OUTPUT_DIR})
    endif()

    if (ARGS_QSCXMLC_ARGUMENTS)
        message(AUTHOR_WARNING
            "QSCXMLC_ARGUMENTS is deprecated. Please use OPTIONS instead.")
        set(ARGS_OPTIONS ${ARGS_QSCXMLC_ARGUMENTS})
    endif()

    set(qscxmlcOutputDir ${CMAKE_CURRENT_BINARY_DIR})
    if (ARGS_OUTPUT_DIRECTORY)
        set(qscxmlcOutputDir ${ARGS_OUTPUT_DIRECTORY})
        if (NOT EXISTS "${qscxmlcOutputDir}" OR NOT IS_DIRECTORY "${qscxmlcOutputDir}")
            message(WARNING
                "qt6_add_statecharts: output dir does not exist: \"" ${qscxmlcOutputDir} "\". "
                "Statechart code generation may fail on some platforms." )
        endif()
    endif()

    foreach(it ${scxml_files})
        get_filename_component(outfilename ${it} NAME_WE)
        get_filename_component(infile ${it} ABSOLUTE)
        set(outfile ${qscxmlcOutputDir}/${outfilename})
        set(outfile_cpp ${qscxmlcOutputDir}/${outfilename}.cpp)
        set(outfile_h ${qscxmlcOutputDir}/${outfilename}.h)

        add_custom_command(OUTPUT ${outfile_cpp} ${outfile_h}
                           ${QT_TOOL_PATH_SETUP_COMMAND}
                           COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qscxmlc
                           ARGS ${namespace} ${ARGS_OPTIONS} --output ${outfile} ${infile}
                           MAIN_DEPENDENCY ${infile}
                           VERBATIM)
        list(APPEND outfiles ${outfile_cpp})
    endforeach()
    set_source_files_properties(${outfiles} PROPERTIES
        SKIP_AUTOMOC TRUE
        SKIP_AUTOUIC TRUE
    )
    if (TARGET ${target_or_outfiles})
        target_include_directories(${target_or_outfiles} PRIVATE ${qscxmlcOutputDir})
        target_sources(${target_or_outfiles} PRIVATE ${outfiles})
    else()
        set(${target_or_outfiles} ${outfiles} PARENT_SCOPE)
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_statecharts outfiles)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_add_statecharts("${outfiles}" ${ARGN})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_statecharts("${outfiles}" ${ARGN})
        endif()
        set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
    endfunction()
endif()
