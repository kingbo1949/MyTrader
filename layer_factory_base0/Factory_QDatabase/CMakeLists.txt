
set(TARGETNAME "Factory_QDatabase")
set(SRC_LIST
        src/CheckTrend.cpp
        src/CheckTrend_Crose.cpp
        src/CheckTrend_Price.cpp
        src/CodeIdManager.cpp
        src/CodeIdManager_Real.cpp
        src/ContractEnv.cpp
        src/ContractEnv_Real.cpp
        src/Factory_IceProxy.cpp
        src/Factory_QDatabase.cpp
        src/IceProxy.cpp
        src/IceProxy_Real.cpp
        src/IceTransfor.cpp
        src/KLineBoard.cpp
        src/KLineBoard_Real.cpp
        src/MarketDepth.cpp
        src/MarketDepth_Real.cpp
        src/MyQDatabase.cpp
        src/MyQDatabase_Real.cpp
        src/pch.cpp
        src/TickBoard.cpp
        src/TickBoard_Real.cpp
        src/TradePointEnv.cpp
        src/TradePointEnv_Real.cpp
        src/WhiteBoard.cpp
        src/WhiteBoard_Real.cpp


)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

if(NOT WIN32)
    find_package(Iconv REQUIRED)
endif()



# 关键：只在构建本动态库时定义 FACTORY_QDATABASE_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_QDATABASE_EXPORTS   # 注意是 PRIVATE
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
        Factory_IBJSon
        IceCompiler
        ToolLib
        Factory_IBGlobalShare
        Factory_Log
        Factory_HashEnv

)

if(NOT WIN32)
    target_link_libraries(${TARGETNAME}
            PRIVATE
            Iconv::Iconv
    )
endif()

#target_link_options(${TARGETNAME} PRIVATE /VERBOSE:LIB)
set_target_properties(${TARGETNAME} PROPERTIES INTERFACE_LINK_LIBRARIES "ZeroIce::all")
#set_target_properties(ZeroIce::ice PROPERTIES INTERFACE_LINK_LIBRARIES "Bzip2::bzip2")
