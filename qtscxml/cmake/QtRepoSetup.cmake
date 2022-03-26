function(add_qt_statecharts target)
    # Don't try to add statecharts when cross compiling, and the target is actually a host target
    # (like a tool).
    qt_is_imported_target("${target}" is_imported)
    if(is_imported)
        return()
    endif()

    qt_parse_all_arguments(arg "add_qt_statecharts" "" "" "FILES" ${ARGN})

    qt6_add_statecharts(${target} ${arg_FILES})
endfunction()
