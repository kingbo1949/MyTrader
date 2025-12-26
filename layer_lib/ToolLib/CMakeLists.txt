
set(TARGETNAME "ToolLib")
# 定义 Ice 路径（从父作用域继承）

set(SRC_LIST
        src/Global.cpp
        src/Log.cpp
        src/MyMath.cpp

)

set(INCLUDE_LIST
        include/Global.h
        include/Log.h
        include/MyMath.h
        include/MyThreadPool.h
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
#        Qt6::Core
#        Qt6::Widgets
        PRIVATE
        Boost::all
)