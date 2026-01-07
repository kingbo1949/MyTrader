
set(TARGETNAME "QAnalyst")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)


set(SRC_LIST
        src/main.cpp
        src/ModifyKline.cpp
        src/QAnalyst.h
        src/QAnalyst.cpp
        src/SaveData_Klines.cpp
        src/StatisTick.cpp
        src/stdafx.cpp
        src/TableModel.h
        src/TableModel.cpp
        src/TableModel_Kline.h
        src/TableModel_Kline.cpp
        src/TableModel_Tick.h
        src/TableModel_Tick.cpp
        src/TableModel_Tick_Future.h
        src/TableModel_Tick_Future.cpp
        src/TableModel_Tick_Stock.h
        src/TableModel_Tick_Stock.cpp
        src/Tick_BigGap.cpp
        src/TradeDay.cpp
        ui/QAnalyst.ui
        resources/QAnalyst.qrc
)

add_executable(${TARGETNAME} ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)

set_target_properties(${TARGETNAME} PROPERTIES
        AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

if(WIN32)
    target_sources(${TARGETNAME} PRIVATE
            resources/win/QAnalyst.rc
    )
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
target_link_libraries(${TARGETNAME}
        PUBLIC
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        IBSysDefine
        Factory_QDatabase
        Factory_IBGlobalShare
        ToolLib
        QtGlobalLib
        Factory_IBJSon
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
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/strategyParams.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/strategyParams.json")
setup_cp(${TARGETNAME} "${CMAKE_SOURCE_DIR}/config/db/Simulator_TimeZone.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run/${TARGETNAME}/db/Simulator_TimeZone.json")


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

set(_install_bin_dir "${CMAKE_INSTALL_BINDIR}")                 # bin
set(_install_plugins_dir "${CMAKE_INSTALL_BINDIR}/plugins")     # bin/plugins
set(_install_run_dir "${CMAKE_INSTALL_BINDIR}/run/${TARGETNAME}") # bin/run/QAnalyst

# 安装可执行文件到 <prefix>/bin
install(TARGETS ${TARGETNAME} RUNTIME DESTINATION "${_install_bin_dir}")

# 安装运行目录需要的子目录（可选，但建议）
install(DIRECTORY DESTINATION "${_install_run_dir}/db")
install(DIRECTORY DESTINATION "${_install_run_dir}/log")

# 安装 config.server 到 <prefix>/bin/run/QAnalyst/config.server
install(FILES "${CMAKE_SOURCE_DIR}/config/config.server" DESTINATION "${_install_run_dir}")

# 安装json文件到<prefix>/bin/run/QAnalyst/db目录
install(FILES "${CMAKE_SOURCE_DIR}/config/db/contracts.json" DESTINATION "${_install_run_dir}/db")
install(FILES "${CMAKE_SOURCE_DIR}/config/db/tradePoints.json" DESTINATION "${_install_run_dir}/db")
install(FILES "${CMAKE_SOURCE_DIR}/config/db/strategyParams.json" DESTINATION "${_install_run_dir}/db")
install(FILES "${CMAKE_SOURCE_DIR}/config/db/Simulator_TimeZone.json" DESTINATION "${_install_run_dir}/db")



# 安装 QAnalyst 运行所需第三方 DLL 到 <prefix>/bin
#    （与当前你调试工作目录保持一致）
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${TARGETNAME}>
            DESTINATION "${_install_bin_dir}" OPTIONAL
    )

    get_target_property(_qt_qmake_exe Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake_exe}" DIRECTORY)
    set(_windeployqt "${_qt_bin_dir}/windeployqt.exe")

    # 安装时执行：把Qt运行库/插件部署到 <prefix>/bin
    install(CODE
            "execute_process(
            COMMAND \"${_windeployqt}\"
                    --no-translations
                    --no-opengl-sw
                    --plugindir \"\${CMAKE_INSTALL_PREFIX}/${_install_plugins_dir}\"
                    \"\${CMAKE_INSTALL_PREFIX}/${_install_bin_dir}/$<TARGET_FILE_NAME:${TARGETNAME}>\"
            RESULT_VARIABLE _res
        )
        if(NOT _res EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed with code: \${_res}\")
        endif()
        "
    )


    install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/run_QAnalyst.bat"
            DESTINATION "${_install_run_dir}"
    )

endif()



