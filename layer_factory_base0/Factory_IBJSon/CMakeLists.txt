
set(TARGETNAME "Factory_IBJSon")
set(SRC_LIST
        src/Factory_IBJSon.cpp
        src/JSon.cpp
        src/JSon_Broker.cpp
        src/JSon_Broker_Real.cpp
        src/JSon_Contracts.cpp
        src/JSon_Contracts_Real.cpp
        src/JSon_Deal.cpp
        src/JSon_Deal_Real.cpp
        src/JSon_LastOrder.cpp
        src/JSon_LastOrder_Real.cpp
        src/JSon_Order.cpp
        src/JSon_Order_Real.cpp
        src/JSon_PositionSize.cpp
        src/JSon_PositionSize_Real.cpp
        src/JSon_PricePair.cpp
        src/JSon_PricePair_Real.cpp
        src/JSon_StrategyParam.cpp
        src/JSon_StrategyParam_Real.cpp
        src/JSon_TimeZone.cpp
        src/JSon_TimeZone_Real.cpp
        src/JSon_TradePoint.cpp
        src/JSon_TradePoint_Real.cpp
        src/pch.cpp
)
set(INCLUDE_LIST
        include/Factory_IBJSon.h
        include/JSon.h
        include/JSon_Broker.h
        include/JSon_Contracts.h
        include/JSon_Deal.h
        include/JSon_LastOrder.h
        include/JSon_Order.h
        include/JSon_PositionSize.h
        include/JSon_PricePair.h
        include/JSon_StrategyParam.h
        include/JSon_TimeZone.h
        include/JSon_TradePoint.h


)

add_library(${TARGETNAME} SHARED)

target_sources(${TARGETNAME}
        PRIVATE ${SRC_LIST}
        PUBLIC ${INCLUDE_LIST}
)



target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)



# 关键：只在构建本动态库时定义 FACTORY_IBJSON_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_IBJSON_EXPORTS   # 注意是 PRIVATE
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
        Jsoncpp::json
        PRIVATE
        ToolLib
        Factory_IBGlobalShare
        Factory_Log
        Factory_HashEnv
)
