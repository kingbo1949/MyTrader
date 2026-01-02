
set(TARGETNAME "Factory_Strategys")
set(SRC_LIST
        src/Factory_Strategys.cpp
        src/pch.cpp
        src/Strategy_Cover.cpp
        src/Strategy_Open.cpp
        src/Strategy_Real.cpp
        src/StrategyBase.cpp
        src/StrategySuit.cpp


)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_STRATEGYS_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_STRATEGYS_EXPORTS   # 注意是 PRIVATE
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
        Factory_IBJSon
        Factory_HashEnv
        Factory_QShareEnv
        Factory_TShareEnv
        Factory_Log
        Factory_StOperator
        Factory_QDatabase
        Factory_Topics
        Factory_UnifyInterface
)
