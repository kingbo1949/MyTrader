
set(TARGETNAME "Factory_StOperator")
set(SRC_LIST
        src/Factory_StOperator.cpp
        src/KLineAndMaTool.cpp
        src/MakePrice.cpp
        src/MakePrice_Manual.cpp
        src/MakePrice_Real.cpp
        src/NeedCancel.cpp
        src/NeedCancel_Manual.cpp
        src/NeedCancel_Real.cpp
        src/OpenPrice.cpp
        src/OpenPrice_Real.cpp
        src/pch.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_STOPERATOR_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_STOPERATOR_EXPORTS   # 注意是 PRIVATE
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
        Factory_QShareEnv
        Factory_IBGlobalShare
        ToolLib
        Factory_HashEnv
        Factory_IBJSon
        Factory_QDatabase
        Factory_TShareEnv
        Factory_UnifyInterface
)
