
set(TARGETNAME "QTransKLineFormIB")
set(SRC_LIST
        src/AppFuncs.cpp
        src/CFixQuote.cpp
        src/CheckQuote.cpp
        src/CleanUpKline.cpp
        src/FixOneDayQuote.cpp
        src/KLineConverter_NoMix.cpp
        src/QTransKLineFormIB.cpp
        src/RecountKline.cpp
        src/SetupEnv.cpp
        src/TickConverter.cpp
        src/UpdateKlineFromFile.cpp


)

add_executable(${TARGETNAME} ${SRC_LIST})



# 关键：只在构建本动态库时定义 FACTORY_CALCULATOR_EXPORTS
#target_compile_definitions(${TARGETNAME}
#        PRIVATE
#        FACTORY_CALCULATOR_EXPORTS   # 注意是 PRIVATE
#)

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)
target_link_libraries(${TARGETNAME} PRIVATE
        Factory_IBJSon
        Factory_QDatabase
        Factory_UnifyInterface
        Factory_Log
        Factory_IBGlobalShare
        ToolLib
        Factory_HashEnv
)

# 开始建立调试目录
set(TARGET_RUN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}")
message(STATUS "TARGET_RUN_PATH = ${TARGET_RUN_PATH}")

setup_makedir(${TARGETNAME} "${TARGET_RUN_PATH}/db")
setup_makedir(${TARGETNAME} "${TARGET_RUN_PATH}/log")

setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/config.server" "${TARGET_RUN_PATH}/config.server")

setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/contracts.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/contracts.json")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/tradePoints.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/tradePoints.json")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/broker_qTransKLineFromIB.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/broker.json")


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
endif ()

# 开始安装
include(GNUInstallDirs)

# 你现有 run 目录布局：bin/run/QTransKLineFormIB/...
set(_install_bin_dir "${CMAKE_INSTALL_BINDIR}")                 # bin
set(_install_run_dir "${CMAKE_INSTALL_BINDIR}/run/${TARGETNAME}")

# 安装可执行文件到 <prefix>/bin
install(TARGETS ${TARGETNAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装运行目录需要的子目录（可选，但建议）
install(DIRECTORY DESTINATION "${_install_run_dir}/db")
install(DIRECTORY DESTINATION "${_install_run_dir}/log")

# 安装 config.server 到 <prefix>/bin/run/QTransKLineFormIB/config.server
install(FILES "${CMAKE_SOURCE_DIR}/config/config.server" DESTINATION "${_install_run_dir}")

# 安装json文件到<prefix>/bin/run/QTransKLineFormIB/db目录
install(FILES "${CMAKE_SOURCE_DIR}/config/db/contracts.json" DESTINATION "${_install_run_dir}/db")
install(FILES "${CMAKE_SOURCE_DIR}/config/db/tradePoints.json" DESTINATION "${_install_run_dir}/db")
install(FILES "${CMAKE_SOURCE_DIR}/config/db/broker_qTransKLineFromIB.json" DESTINATION "${_install_run_dir}/db" RENAME broker.json)


# 安装 QTransKLineFormIB 运行所需第三方 DLL 到 <prefix>/bin/
#    （与当前你调试工作目录保持一致）
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${TARGETNAME}>
            DESTINATION "${_install_bin_dir}"
    )
    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/run_QTransKLineFormIB.bat"
            DESTINATION "${_install_run_dir}"
    )

endif()



