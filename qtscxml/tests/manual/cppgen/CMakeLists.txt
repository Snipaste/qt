
#####################################################################
## tst_cppgen Tool:
#####################################################################

qt_get_tool_target_name(target_name tst_cppgen)
qt_internal_add_tool(${target_name}
    SOURCES
        tst_cppgen.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Scxml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:cppgen.pro:<TRUE>:
# TEMPLATE = "app"
# _OPTION = "host_build"
