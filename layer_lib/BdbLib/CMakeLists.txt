set(TARGETNAME "BdbLib")

set(SRC_LIST
        src/DbMapType/DbMapType.cpp
        src/DbMapType/DbMapType_Dup.cpp
        src/DbMapType/DbMapType_NoDup.cpp
        src/DbTable.cpp
        src/Env.cpp

)

add_library(${TARGETNAME} STATIC ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>
)

target_link_libraries(${TARGETNAME}
        PUBLIC
        BerkeleyDB::db
        ZeroIce::all
        PRIVATE
        ToolLib
        Boost::all
)
