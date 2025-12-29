
set(TARGETNAME "Factory_Simulator")
set(SRC_LIST
        src/Factory_Simulator.cpp
        src/MeetDeal.cpp
        src/MeetDeal_Loose.cpp
        src/MeetDeal_Rigorous.cpp
        src/pch.cpp
        src/Simulator.cpp
        src/Simulator_Real.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_SIMULATOR_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_SIMULATOR_EXPORTS   # 注意是 PRIVATE
)
set_target_properties(${TARGETNAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
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
        Factory_Callback
        Factory_QShareEnv
        Factory_IBGlobalShare
        ToolLib
        Factory_Log
        Factory_HashEnv
        Factory_IBJSon
        Factory_QDatabase
        Factory_TShareEnv
)
