

set(TARGETNAME "StLoader")

set(SRC_LIST
        src/CmdQGernerator_Simulator.cpp
        src/CompareQuote.cpp
        src/DownloadData.cpp
        src/PrintQuoteLastBar.cpp
        src/QueryPosition.cpp
        src/StLoader.cpp
        src/SubMenu.cpp
        src/SubMenu_ManualOrder.cpp
        src/SubMenu_PendingOrder.cpp

)

add_executable(${TARGETNAME} ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


# 关键：只在构建本动态库时定义 FACTORY_CALCULATOR_EXPORTS
#target_compile_definitions(QuoteGenerator
#        PRIVATE
#        FACTORY_CALCULATOR_EXPORTS   # 注意是 PRIVATE
#)


target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)
target_link_libraries(${TARGETNAME}
        PRIVATE
        Factory_Setup
        Factory_IBJSon
        Factory_QDatabase
        Factory_HashEnv
        Factory_UnifyInterface
        Factory_AnalystDeal
        Factory_Topics
        Factory_IBGlobalShare
        Factory_QShareEnv
        Factory_TShareEnv
        Factory_Monitor
        Factory_StOperator
        Factory_Calculator
        ToolLib
)

# 开始建立调试目录
#message(STATUS "DIR = ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db")
setup_makedir(${TARGETNAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db")
setup_makedir(${TARGETNAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/log")

# 拷贝配置文件
#set(run_dir "${CMAKE_BINARY_DIR}/run/${run_subdir}")

setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/config.server" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/config.server")

setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/contracts.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/contracts.json")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/tradePoints.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/tradePoints.json")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/strategyParams.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/strategyParams.json")

setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/Simulator_TimeZone.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/Simulator_TimeZone.json")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/broker_stloader.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/broker.json")

# 设置工作目录 执行文件并不需要在工作目录
set_target_properties(${TARGETNAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}" )


set(run_dir "${CMAKE_BINARY_DIR}/bin/run/$<TARGET_FILE_BASE_NAME:${TARGETNAME}>")

# 把第三方依赖拷贝到工作目录
add_custom_command(TARGET ${TARGETNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        "-Ddlls=$<JOIN:$<TARGET_RUNTIME_DLLS:${TARGETNAME}>,;>"
        "-Ddest_dir=${run_dir}"
        -P "${CMAKE_SOURCE_DIR}/cmake/copy_thirdparty_dlls.cmake"
        VERBATIM
)

