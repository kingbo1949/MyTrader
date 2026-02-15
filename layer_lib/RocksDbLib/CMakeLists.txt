set(TARGETNAME "RocksDbLib")

set(SRC_LIST
        src/RocksEnv.cpp
        src/RocksDb.cpp
        src/RocksWriteBatch.cpp
)

add_library(${TARGETNAME} STATIC ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
)

target_link_libraries(${TARGETNAME}
        PUBLIC
        RocksDB::rocksdb
        ZeroIce::all
        PRIVATE
        ToolLib
        Boost::all
)

add_subdirectory(demo)
