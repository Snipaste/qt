file(ARCHIVE_CREATE
    OUTPUT cmake_zstd.zstd
    PATHS "${CMAKE_CURRENT_LIST_FILE}"
    FORMAT raw
    COMPRESSION Zstd)
