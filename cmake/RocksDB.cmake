

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
    # Windows：强制使用**静态库**（rocksdb.lib）
    add_library(RocksDB::rocksdb STATIC IMPORTED GLOBAL)

    set_target_properties(RocksDB::rocksdb PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_ROOT}/include"

            # 静态库只需要 .lib 文件（无 DLL、无 -shared 后缀）
            IMPORTED_LOCATION_DEBUG   "${ROCKSDB_ROOT}/debug/lib/rocksdbd.lib"
            IMPORTED_LOCATION_RELEASE "${ROCKSDB_ROOT}/lib/rocksdb.lib"
    )


    return()
endif()


#if(WIN32)
#    # === 关键修复：添加 RocksDB 静态库所需的 Windows 系统库 ===
#    target_link_libraries(RocksDB::rocksdb INTERFACE
#            rpcrt4.lib      # UUID 函数
#            shlwapi.lib     # Path* 函数
#    )
#endif()
