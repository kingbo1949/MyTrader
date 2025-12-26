
set(TARGETNAME "Factory_QShareEnv")
set(SRC_LIST
        src/Factory_QShareEnv.cpp
        src/pch.cpp
        src/QEnvManager.cpp
        src/QEnvManager_Real.cpp
        src/QEnvOneCode.cpp
        src/QEnvOneCode_Real.cpp


)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_QSHAREENV_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_QSHAREENV_EXPORTS   # 注意是 PRIVATE
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
        Factory_IBJSon
        Factory_HashEnv
)
