
set(TARGETNAME "Factory_Log")
set(SRC_LIST
        src/Factory_Log.cpp
        src/DailyFile.cpp
        src/DailyFile_Real.cpp
        src/LogMsgs.cpp
        src/LogMsgs_Real.cpp
        src/pch.cpp
        src/PrintMsgOnce.cpp
        src/PrintMsgOnce_Real.cpp
        src/RotatingFile.cpp
        src/RotatingFile_Real.cpp
        src/ToStr.cpp
        src/ToStr_FunctionObj.cpp
        src/ToStr_Str.cpp

)

add_library(${TARGETNAME} SHARED ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)



# 关键：只在构建本动态库时定义 FACTORY_LOG_EXPORTS
target_compile_definitions(${TARGETNAME}
        PRIVATE
        FACTORY_LOG_EXPORTS   # 注意是 PRIVATE
)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)

target_link_libraries(${TARGETNAME}
        PUBLIC
        Spdlog::log
)