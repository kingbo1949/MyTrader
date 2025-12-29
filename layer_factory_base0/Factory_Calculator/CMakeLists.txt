

set(TARGETNAME "Factory_Calculator")
set(SRC_LIST
        src/Calculator_Average.cpp
        src/Calculator_Ema.cpp
        src/Calculator_Ma.cpp
        src/Calculator_Macd.cpp
        src/Calculator_Macd_Real.cpp
        src/Calculator_MacdDiv.cpp
        src/Calculator_MacdDiv_Real.cpp
        src/Container_Average.cpp
        src/Container_Ema.cpp
        src/Container_Ma.cpp
        src/Container_Macd.cpp
        src/Container_Macd_Real.cpp
        src/Container_MacdDiv.cpp
        src/Container_MacdDiv_Real.cpp
        src/Factory_Calculator.cpp
        src/pch.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

# 关键：只在构建本动态库时定义 FACTORY_CALCULATOR_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_CALCULATOR_EXPORTS   # 注意是 PRIVATE
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
        Factory_IBGlobalShare
        ToolLib
        Factory_Log
)
