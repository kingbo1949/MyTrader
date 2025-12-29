
set(TARGETNAME "Factory_HashEnv")
set(SRC_LIST
        src/Factory_HashEnv.cpp
        src/ChoiceHelper.cpp
        src/CodeIdHashEnv.cpp
        src/CodeIdHashEnv_Real.cpp
        src/pch.cpp
        src/StrategyParamHashEnv.cpp
        src/StrategyParamHashEnv_Real.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

# 关键：只在构建本动态库时定义 FACTORY_HASHENV_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_HASHENV_EXPORTS   # 注意是 PRIVATE
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
        ToolLib
        Factory_Log
        Boost::all


)

