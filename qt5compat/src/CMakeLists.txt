# Generated from src.pro.

add_subdirectory(core5)
if(TARGET Qt::Gui AND TARGET Qt::Quick)
    add_subdirectory(imports)
endif()
