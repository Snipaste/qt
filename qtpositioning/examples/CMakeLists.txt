# Generated from examples.pro.

qt_examples_build_begin(EXTERNAL_BUILD)

if(TARGET Qt::Positioning)
    add_subdirectory(positioning)
endif()

qt_examples_build_end()
