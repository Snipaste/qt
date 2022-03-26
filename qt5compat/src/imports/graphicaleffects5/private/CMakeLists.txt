#####################################################################
## qtgraphicaleffectsprivate Plugin:
#####################################################################

set(qml_files
    "FastGlow.qml"
)

qt_internal_add_qml_module(qtgraphicaleffectsprivate
    URI "Qt5Compat.GraphicalEffects.private"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtGraphicalEffectsPrivatePlugin
    PLUGIN_TARGET qtgraphicaleffectsprivate
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_QMLTYPES
    PAST_MAJOR_VERSIONS 1
    SOURCES
        plugin.cpp
        qgfxsourceproxy.cpp qgfxsourceproxy_p.h
    QML_FILES
        ${qml_files}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)
