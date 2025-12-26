
set(TARGETNAME "Factory_Setup")
set(SRC_LIST
        src/Factory_Setup.cpp
        src/CmdUpdateDbFromIB.cpp
        src/DaemonByTick.cpp
        src/DaemonByTick_QGenerator.cpp
        src/DaemonByTick_Real.cpp
        src/DaemonByTick_Trade.cpp
        src/pch.cpp
        src/QCallbackObject.cpp
        src/QCallbackObject_QGenerator.cpp
        src/QCallbackObject_RealTime.cpp
        src/QCallbackObject_Simulator.cpp
        src/QCallbackObject_Trade.cpp
        src/Setup.cpp
        src/Setup_QGenerator.cpp
        src/Setup_Real.cpp
        src/Setup_TRunTime.cpp
        src/Setup_TSimulator.cpp



)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_SETUP_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_SETUP_EXPORTS   # 注意是 PRIVATE
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
        ToolLib
        Factory_IBGlobalShare
        Factory_QDatabase
        Factory_QShareEnv
        Factory_TShareEnv
        Factory_UnifyInterface
        Factory_IBJSon
        Factory_HashEnv
        Factory_Strategys
        Factory_Topics
        Factory_Callback
        Factory_Simulator
)
