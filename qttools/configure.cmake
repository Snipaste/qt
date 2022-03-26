

#### Inputs



#### Libraries



#### Tests

# libclang
# special case begin
# Even though Qt builds with qmake and libclang 6.0, it fails with CMake.
# Presumably because 6.0 ClangConfig.cmake files are not good enough?
# In any case explicitly request a minimum version of 8.x for now, otherwise
# building with CMake will fail at compilation time.
qt_find_package(WrapLibClang 8 PROVIDED_TARGETS WrapLibClang::WrapLibClang)
# special case end

if(TARGET WrapLibClang::WrapLibClang)
    set(TEST_libclang "ON" CACHE BOOL "Required libclang version found." FORCE)
endif()



#### Features

# Check whether the sqlite plugin is available.
set(sqlite_plugin_available FALSE)
if(NOT QT_CONFIGURE_RUNNING)
    if(TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::Sql)
        get_target_property(sql_plugins ${QT_CMAKE_EXPORT_NAMESPACE}::Sql QT_PLUGINS)
        if(QSQLiteDriverPlugin IN_LIST sql_plugins)
            set(sqlite_plugin_available TRUE)
        endif()
    endif()
endif()

qt_feature("assistant" PRIVATE
    LABEL "Qt Assistant"
    PURPOSE "Qt Assistant is a tool for viewing on-line documentation in Qt help file format."
    CONDITION TARGET Qt::Widgets AND QT_FEATURE_png AND QT_FEATURE_pushbutton AND QT_FEATURE_toolbutton AND (sqlite_plugin_available OR QT_BUILD_SHARED_LIBS)
)
qt_feature("clang" PRIVATE
    LABEL "QDoc"
    CONDITION TEST_libclang
)
qt_feature("clangcpp" PRIVATE
    LABEL "Clang-based lupdate parser"
    CONDITION QT_FEATURE_clang AND TEST_libclang
)
qt_feature("designer" PRIVATE
    LABEL "Qt Designer"
    PURPOSE "Qt Designer is the Qt tool for designing and building graphical user interfaces (GUIs) with Qt Widgets. You can compose and customize your windows or dialogs in a what-you-see-is-what-you-get (WYSIWYG) manner, and test them using different styles and resolutions."
    CONDITION TARGET Qt::Widgets AND QT_FEATURE_png AND QT_FEATURE_pushbutton AND QT_FEATURE_toolbutton
)
qt_feature("distancefieldgenerator" PRIVATE
    LABEL "Qt Distance Field Generator"
    PURPOSE "The Qt Distance Field Generator tool can be used to pregenerate the font cache in order to optimize startup performance."
    CONDITION TARGET Qt::Widgets AND QT_FEATURE_png AND QT_FEATURE_thread AND QT_FEATURE_toolbutton AND TARGET Qt::Quick
)
qt_feature("kmap2qmap" PRIVATE
    LABEL "kmap2qmap"
    PURPOSE "kmap2qmap is a tool to generate keymaps for use on Embedded Linux. The source files have to be in standard Linux kmap format that is e.g. understood by the kernel's loadkeys command."
)
qt_feature("linguist" PRIVATE
    LABEL "Qt Linguist"
    PURPOSE "Qt Linguist can be used by translator to translate text in Qt applications."
)
qt_feature("macdeployqt" PRIVATE
    LABEL "Mac Deployment Tool"
    PURPOSE "The Mac deployment tool automates the process of creating a deployable application bundle that contains the Qt libraries as private frameworks."
    CONDITION MACOS
)
qt_feature("pixeltool" PRIVATE
    LABEL "pixeltool"
    PURPOSE "The Qt Pixel Zooming Tool is a graphical application that magnifies the screen around the mouse pointer so you can look more closely at individual pixels."
    CONDITION TARGET Qt::Widgets AND QT_FEATURE_png AND QT_FEATURE_pushbutton AND QT_FEATURE_toolbutton
)
qt_feature("qdbus" PRIVATE
    LABEL "qdbus"
    PURPOSE "qdbus is a communication interface for Qt-based applications."
    CONDITION TARGET Qt::DBus
)
qt_feature("qev" PRIVATE
    LABEL "qev"
    PURPOSE "qev allows introspection of incoming events for a QWidget, similar to the X11 xev tool."
)
qt_feature("qtattributionsscanner" PRIVATE
    LABEL "Qt Attributions Scanner"
    PURPOSE "Qt Attributions Scanner generates attribution documents for third-party code in Qt."
    CONDITION QT_FEATURE_commandlineparser
)
qt_feature("qtdiag" PRIVATE
    LABEL "qtdiag"
    PURPOSE "qtdiag outputs information about the Qt installation it was built with."
    CONDITION QT_FEATURE_commandlineparser AND TARGET Qt::Gui AND NOT ANDROID AND NOT QNX AND NOT UIKIT AND NOT WASM
)
qt_feature("qtplugininfo" PRIVATE
    LABEL "qtplugininfo"
    PURPOSE "qtplugininfo dumps metadata about Qt plugins in JSON format."
    CONDITION QT_FEATURE_commandlineparser AND QT_FEATURE_library AND (android_app OR NOT ANDROID)
)
qt_feature("windeployqt" PRIVATE
    LABEL "Windows deployment tool"
    PURPOSE "The Windows deployment tool is designed to automate the process of creating a deployable folder containing the Qt-related dependencies (libraries, QML imports, plugins, and translations) required to run the application from that folder. It creates a sandbox for Universal Windows Platform (UWP) or an installation tree for Windows desktop applications, which can be easily bundled into an installation package."
    CONDITION WIN32
)
qt_configure_add_summary_section(NAME "Qt Tools")
qt_configure_add_summary_entry(ARGS "assistant")
qt_configure_add_summary_entry(ARGS "clang")
qt_configure_add_summary_entry(ARGS "clangcpp")
qt_configure_add_summary_entry(ARGS "designer")
qt_configure_add_summary_entry(ARGS "distancefieldgenerator")
#qt_configure_add_summary_entry(ARGS "kmap2qmap")
qt_configure_add_summary_entry(ARGS "linguist")
qt_configure_add_summary_entry(ARGS "macdeployqt")
qt_configure_add_summary_entry(ARGS "pixeltool")
qt_configure_add_summary_entry(ARGS "qdbus")
#qt_configure_add_summary_entry(ARGS "qev")
qt_configure_add_summary_entry(ARGS "qtattributionsscanner")
qt_configure_add_summary_entry(ARGS "qtdiag")
qt_configure_add_summary_entry(ARGS "qtplugininfo")
qt_configure_add_summary_entry(ARGS "windeployqt")
qt_configure_end_summary_section() # end of "Qt Tools" section
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "QDoc will not be compiled, probably because libclang could not be located. This means that you cannot build the Qt documentation.
Either set CMAKE_PREFIX_PATH or LLVM_INSTALL_DIR to the location of your llvm installation.
On Linux systems, you may be able to install libclang by installing the libclang-dev or libclang-devel package, depending on your distribution.
On macOS, you can use Homebrew's llvm package.
You will also need to set the FEATURE_clang CMake variable to ON to re-evaluate this check."
    CONDITION NOT QT_FEATURE_clang
)
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "Clang-based lupdate parser will not be available. LLVM and Clang C++ libraries have not been found.
You will need to set the FEATURE_clangcpp CMake variable to ON to re-evaluate this check."
    CONDITION NOT QT_FEATURE_clangcpp
)
