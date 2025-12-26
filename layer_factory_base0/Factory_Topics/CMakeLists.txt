
set(TARGETNAME "Factory_Topics")
set(SRC_LIST
        src/Factory_Topics.cpp
        src/pch.cpp
        src/Topic_OnTick.cpp
        src/Topic_OnTick_Real.cpp
        src/Topic_OnTrade.cpp
        src/Topic_OnTrade_Real.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})


target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_TOPIC_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_TOPIC_EXPORTS   # 注意是 PRIVATE
)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)

target_link_libraries(${TARGETNAME}
        PUBLIC
        IBSysDefine

)