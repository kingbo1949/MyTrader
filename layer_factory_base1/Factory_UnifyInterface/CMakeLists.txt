

set(TARGETNAME "Factory_UnifyInterface")
set(SRC_LIST
        src/Factory_UnifyInterface.cpp
        src/Cmd_IBDaemon.cpp
        src/IBApi.cpp
        src/IBApi_IB.cpp
        src/IBApi_Real.cpp
        src/IBApi_Simulator.cpp
        src/IBSpi.cpp
        src/pch.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_UNIFYINTERFACE_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_UNIFYINTERFACE_EXPORTS   # 注意是 PRIVATE
)
set_target_properties(${TARGETNAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE: ${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)

if(WIN32)
    target_link_libraries(${TARGETNAME}
            PUBLIC
            IBSysDefine
            PRIVATE
            IBSourceLib
            Factory_Simulator
            Factory_Topics
            Factory_Callback
            Factory_IBGlobalShare
            ToolLib
            Factory_Log
            Factory_HashEnv
            Factory_IBJSon
            Factory_QDatabase
            Factory_TShareEnv
#            Intelrdfpmath::intelrdfpmath

    )

    return()
endif()


