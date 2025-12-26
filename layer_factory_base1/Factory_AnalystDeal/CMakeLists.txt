set(TARGETNAME "Factory_AnalystDeal")

set(SRC_LIST
        src/AnalystResult_Real.cpp
        src/AnalystResultMonth.cpp
        src/AnalystResultMonth_Real.cpp
        src/AnalystSimpleMatch.cpp
        src/AnalystSimpleMatch_Real.cpp
        src/Factory_AnalystDeal.cpp
        src/pch.cpp
        src/ScanDealsForSimpleMatch.cpp
        src/ScanDealsForSimpleMatch_Real.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

# 关键：只在构建本动态库时定义 FACTORY_ANALYSTDEAL_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_ANALYSTDEAL_EXPORTS   # 注意是 PRIVATE
)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)
target_link_libraries(${TARGETNAME} PRIVATE
        IBSysDefine
        Factory_IBGlobalShare
        ToolLib
        Factory_Log
        Factory_HashEnv
        Factory_IBJSon
        Factory_QDatabase
        Factory_TShareEnv
)
