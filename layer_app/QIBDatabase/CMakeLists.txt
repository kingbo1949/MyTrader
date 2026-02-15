
set(TARGETNAME "QIBDatabase")

set(SRC_LIST
        src/Calculator/Calculator.cpp
        src/Calculator/Calculator_Average.cpp
        src/Calculator/Calculator_DivType.cpp
        src/Calculator/Calculator_Ema.cpp
        src/Calculator/Calculator_Ma.cpp
        src/Calculator/Calculator_Macd.cpp
        src/Calculator/Calculator_VwMa.cpp
        src/Calculator/Calculator_Atr.cpp
        src/DbTable/DbTable_Average.cpp
        src/DbTable/DbTable_DivType.cpp
        src/DbTable/DbTable_KLine.cpp
        src/DbTable/DbTable_Macd.cpp
        src/DbTable/DbTable_TickHis.cpp
        src/DbTable/CDbTable_Atr.cpp
        src/Env_IB.cpp
        src/Factory.cpp
        src/GetRecordNoForDb.cpp
        src/MakeKlinePairs.cpp
        src/pch.cpp
        src/QDatabaseImp.cpp
        src/QIBDatabase.cpp
        src/Shell.cpp
        src/TickUpdator.cpp
        src/TimerTask_UpdateIndex.cpp

)

add_executable(${TARGETNAME} ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

if(NOT WIN32)
    find_package(Iconv REQUIRED)
endif()

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
        RocksDbLib
        IBSysDefine
        ToolLib
        Factory_Log
        ZeroIce::all
)
if(NOT WIN32)
    target_link_libraries(${TARGETNAME}
            PRIVATE
            Iconv::Iconv
    )
endif()

# 开始建立调试目录
set(TARGET_RUN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}")
message(STATUS "TARGET_RUN_PATH = ${TARGET_RUN_PATH}")

setup_makedir(${TARGETNAME} "${TARGET_RUN_PATH}/history_rocksdb")
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

endif()

# 开始安装
include(GNUInstallDirs)

# 你现有 run 目录布局：bin/run/QIBDatabase/...
set(_install_bin_dir "${CMAKE_INSTALL_BINDIR}")                 # bin
set(_install_run_dir "${CMAKE_INSTALL_BINDIR}/run/${TARGETNAME}")

# 1) 安装可执行文件到 <prefix>/bin
install(TARGETS ${TARGETNAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 2) 安装运行目录需要的子目录（可选，但建议）
install(DIRECTORY DESTINATION "${_install_run_dir}/history")
install(DIRECTORY DESTINATION "${_install_run_dir}/log")

# 3) 安装 config.server 到 <prefix>/bin/run/QIBDatabase/config.server
install(FILES "${CMAKE_SOURCE_DIR}/config/config.server"
        DESTINATION "${_install_run_dir}"
)

# 4) 安装 QIBDatabase 运行所需第三方 DLL 到 <prefix>/bin/
#    （与当前你调试工作目录保持一致）
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${TARGETNAME}>
            DESTINATION "${_install_bin_dir}"
    )
    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/run_QIBDatabase.bat"
            DESTINATION "${_install_run_dir}"
    )

endif()


