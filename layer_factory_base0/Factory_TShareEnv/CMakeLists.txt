
set(TARGETNAME "Factory_TShareEnv")
set(SRC_LIST
        src/db_ActiveAction.cpp
        src/db_ActiveAction_Real.cpp
        src/db_Deal.cpp
        src/db_Deal_Real.cpp
        src/db_LastOrder.cpp
        src/db_LastOrder_Real.cpp
        src/db_Order.cpp
        src/db_Order_Real.cpp
        src/db_PositionSize.cpp
        src/db_PositionSize_Real.cpp
        src/db_PricePair.cpp
        src/db_PricePair_Real.cpp
        src/Factory_TShareEnv.cpp
        src/pch.cpp


)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_TSHAREENV_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_TSHAREENV_EXPORTS   # 注意是 PRIVATE
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
        Factory_IBJSon
        Factory_HashEnv
)
