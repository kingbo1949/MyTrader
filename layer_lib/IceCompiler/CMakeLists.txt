
set(TARGETNAME "IceCompiler")

set(SRC_LIST
        src/QDatabase.cpp
        src/QDatabaseI.cpp
        src/QStruc.cpp
        src/QStrucI.cpp

)
set(INCLUDE_LIST
        include/QDatabase.h
        include/QDatabaseI.h
        include/QStruc.h
        include/QStrucI.h
)

add_library(${TARGETNAME})

target_sources(${TARGETNAME}
        PRIVATE ${SRC_LIST}
        PUBLIC ${INCLUDE_LIST}
)


target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)

target_link_libraries(${TARGETNAME}
        PUBLIC
        ZeroIce::all
)