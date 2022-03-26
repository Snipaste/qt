# Generated from qregexp.pro.

#####################################################################
## tst_bench_qregexp Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qregexp
    EXCEPTIONS
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Core5Compat
        Qt::Test
)

# Resources:
set(qregexp_resource_files
    "main.cpp"
)

qt_internal_add_resource(tst_bench_qregexp "qregexp"
    PREFIX
        "/"
    FILES
        ${qregexp_resource_files}
)


#### Keys ignored in scope 1:.:.:qregexp.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_bench_qregexp CONDITION TARGET Qt::Script AND NOT pcre
    DEFINES
        HAVE_JSC
    PUBLIC_LIBRARIES
        Qt::Script
)

qt_internal_extend_target(tst_bench_qregexp CONDITION (NOT QNX) AND (EXISTS _ss_[QT_SYSROOT]/usr/include/boost/regex.hpp)
    DEFINES
        HAVE_BOOST
    PUBLIC_LIBRARIES
        boost_regex
)
