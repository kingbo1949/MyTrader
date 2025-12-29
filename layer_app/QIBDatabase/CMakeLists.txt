
set(TARGETNAME "QIBDatabase")

set(SRC_LIST
        src/Calculator.cpp
        src/Calculator_Average.cpp
        src/Calculator_DivType.cpp
        src/Calculator_Ema.cpp
        src/Calculator_Ma.cpp
        src/Calculator_Macd.cpp
        src/Calculator_VwMa.cpp
        src/Compare.cpp
        src/DbTable_Average.cpp
        src/DbTable_DivType.cpp
        src/DbTable_KLine.cpp
        src/DbTable_Macd.cpp
        src/DbTable_TickHis.cpp
        src/Env_IB.cpp
        src/Factory.cpp
        src/GetRecordNoForDb.cpp
        src/MakeKlinePairs.cpp
        src/pch.cpp
        src/QDatabaseImp.cpp
        src/QIBDatabase.cpp
        src/Shell.cpp
        src/TickUpdator.cpp
        src/TimerTask_CheckPoint.cpp
        src/TimerTask_UpdateIndex.cpp

)

add_executable(${TARGETNAME} ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_CALCULATOR_EXPORTS
#target_compile_definitions(${TARGETNAME}
#        PRIVATE
#        FACTORY_CALCULATOR_EXPORTS   # 注意是 PRIVATE
#)

target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)
target_link_libraries(${TARGETNAME} PRIVATE
        IceCompiler
        BdbLib
        IBSysDefine
        ToolLib
        Factory_Log
        ZeroIce::all
)

# 开始建立调试目录
set(TARGET_RUN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}")
message(STATUS "TARGET_RUN_PATH = ${TARGET_RUN_PATH}")

setup_makedir(${TARGETNAME} "${TARGET_RUN_PATH}/history")
setup_makedir(${TARGETNAME} "${TARGET_RUN_PATH}/log")

setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/config.server" "${TARGET_RUN_PATH}/config.server")

# 设置工作目录 执行文件并不需要在工作目录
set_target_properties(${TARGETNAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}" )

set(run_dir "${CMAKE_BINARY_DIR}/bin/run/$<TARGET_FILE_BASE_NAME:${TARGETNAME}>")

# 把第三方依赖拷贝到工作目录


if (WIN32)
    add_custom_command(TARGET ${TARGETNAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            "-Ddlls=$<JOIN:$<TARGET_RUNTIME_DLLS:${TARGETNAME}>,;>"
            "-Ddest_dir=${run_dir}"
            -P "${CMAKE_SOURCE_DIR}/cmake/copy_thirdparty_dlls.cmake"
            VERBATIM
    )

    return()
endif()

