
set(TARGETNAME "Factory_Monitor")
set(SRC_LIST
        src/Factory_Monitor.cpp
        src/Monitor.cpp
        src/Monitor_Real.cpp
        src/pch.cpp
)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_MONITOR_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_MONITOR_EXPORTS   # 注意是 PRIVATE
)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)
target_link_libraries(${TARGETNAME}
        PUBLIC
        IBSysDefine
        PRIVATE
        Factory_IBGlobalShare
        ToolLib
        Factory_Log
)
