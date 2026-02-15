

include_guard(GLOBAL)

# 避免重复定义
if(TARGET RocksDB::rocksdb)
    return()
endif()

if(NOT DEFINED ROCKSDB_ROOT)
    message(FATAL_ERROR "ROCKSDB_ROOT is not set. Define it via CMakePresets.json or -DROCKSDB_ROOT=...")
endif()

if(APPLE)
    add_library(RocksDB::rocksdb UNKNOWN IMPORTED GLOBAL)
    set_target_properties(RocksDB::rocksdb PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_ROOT}/release/include"
            IMPORTED_LOCATION_DEBUG   "${ROCKSDB_ROOT}/debug/lib/librocksdb.dylib"
            IMPORTED_LOCATION_RELEASE "${ROCKSDB_ROOT}/release/lib/librocksdb.dylib"
    )
else()
    message(FATAL_ERROR "Unsupported platform for RocksDB (only APPLE is supported currently).")
endif()
