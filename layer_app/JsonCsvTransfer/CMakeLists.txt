
set(TARGETNAME "JsonCsvTransfer")

set(SRC_LIST
        src/CsvContract.cpp
        src/CsvContract_Real.cpp
        src/CsvStrategy.cpp
        src/CsvStrategy_Real.cpp
        src/CsvTradePoints.cpp
        src/CsvTradePoints_Real.cpp
        src/CsvTransor.cpp
        src/JsonCsvTransfer.cpp


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
        IBSysDefine
        Factory_IBGlobalShare
        ToolLib
        Factory_Log
        Factory_HashEnv
        Factory_IBJSon
)

# 开始建立调试目录
set(TARGET_RUN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}")
message(STATUS "TARGET_RUN_PATH = ${TARGET_RUN_PATH}")

setup_makedir(${TARGETNAME} "${TARGET_RUN_PATH}/csv")
setup_makedir(${TARGETNAME} "${TARGET_RUN_PATH}/db")

setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/csv/contracts.csv" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/csv/contracts.csv")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/csv/tradePoints.csv" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/csv/tradePoints.csv")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/csv/strategyParams.csv" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/csv/strategyParams.csv")


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

# 你现有 run 目录布局：bin/run/QuoteGenerator/...
set(_install_bin_dir "${CMAKE_INSTALL_BINDIR}")                 # bin
set(_install_run_dir "${CMAKE_INSTALL_BINDIR}/run/${TARGETNAME}")

# 安装可执行文件到 <prefix>/bin
install(TARGETS ${TARGETNAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装运行目录需要的子目录（可选，但建议）
install(DIRECTORY DESTINATION "${_install_run_dir}/db")
install(DIRECTORY DESTINATION "${_install_run_dir}/csv")

# 安装csv文件到<prefix>/bin/run/JsonCsvTransfer/csv目录
install(FILES "${CMAKE_SOURCE_DIR}/config/csv/contracts.csv" DESTINATION "${_install_run_dir}/csv")
install(FILES "${CMAKE_SOURCE_DIR}/config/csv/tradePoints.csv" DESTINATION "${_install_run_dir}/csv")
install(FILES "${CMAKE_SOURCE_DIR}/config/csv/strategyParams.csv" DESTINATION "${_install_run_dir}/csv")



# 安装 JsonCsvTransfer 运行所需第三方 DLL 到 <prefix>/bin/
#    （与当前你调试工作目录保持一致）
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${TARGETNAME}>
            DESTINATION "${_install_bin_dir}"
    )
    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/run_JsonCsvTransfer.bat"
            DESTINATION "${_install_run_dir}"
    )

endif()

