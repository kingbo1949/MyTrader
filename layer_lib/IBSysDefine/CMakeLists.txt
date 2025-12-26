set(TARGETNAME "IBSysDefine")
set(SRC_LIST
        src/base_struc.cpp
        src/base_trade.cpp
        src/pch.cpp
)
set(INCLUDE_LIST
        include/base_struc.h
        include/base_trade.h
        include/CompileDefine.h.h
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
        PRIVATE
        Spdlog::log
)