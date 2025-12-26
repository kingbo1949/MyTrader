
set(TARGETNAME "Factory_Callback")
set(SRC_LIST
        src/Factory_Callback.cpp
        src/OnRtn.cpp
        src/OnRtn_Deal.cpp
        src/OnRtn_ErrCancel.cpp
        src/OnRtn_Order.cpp
        src/OnRtn_Real.cpp
        src/OnRtnQueue.cpp
        src/OnRtnQueue_Real.cpp
        src/pch.cpp
)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

# 关键：只在构建本动态库时定义 FACTORY_CALLBACK_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_CALLBACK_EXPORTS   # 注意是 PRIVATE
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
        Factory_QDatabase
        Factory_TShareEnv
        Factory_Topics
        Factory_Log
        Factory_IBJSon
)
