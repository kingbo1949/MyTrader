
set(TARGETNAME "Factory_IBGlobalShare")

set(SRC_LIST
        src/CurrentTime.cpp
        src/CurrentTime_RunTime.cpp
        src/CurrentTime_Simulation.cpp
        src/Factory_IBGlobalShare.cpp
        src/GetRecordNo.cpp
        src/HighFrequencyGlobalFunc.cpp
        src/pch.cpp
        src/Reverse.cpp
        src/SeriesNoMaker.cpp
        src/SeriesNoMaker_Real.cpp
        src/Trans.cpp
        src/TransDataToStr.cpp
        src/TransToStr.cpp
        src/TradeDay_Real.cpp


)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

if(NOT WIN32)
    find_package(Iconv REQUIRED)
endif()


# 关键：只在构建本动态库时定义 FACTORY_IBGLOBALSHARE_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_IBGLOBALSHARE_EXPORTS   # 注意是 PRIVATE
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
        Boost::all
        Factory_Log
        PRIVATE
        IBSysDefine
        ToolLib

)
if(NOT WIN32)
    target_link_libraries(${TARGETNAME}
            PRIVATE
            Iconv::Iconv
    )
endif()

