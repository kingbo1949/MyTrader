
set(TARGETNAME "AnalystGui")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)


set(ITEMHANDLER_LIST
        ItemHandler/AvgValueGraph.cpp
        ItemHandler/AvgValueGraph.h
        ItemHandler/CloseTag.cpp
        ItemHandler/CloseTag.h
        ItemHandler/CloseTag_Main.cpp
        ItemHandler/CloseTag_Main.h
        ItemHandler/CloseTag_Sub.cpp
        ItemHandler/CloseTag_Sub.h
        ItemHandler/EmaGraph.cpp
        ItemHandler/EmaGraph.h
        ItemHandler/ItemsHandler.cpp
        ItemHandler/ItemsHandler.h
        ItemHandler/KLinePlot.cpp
        ItemHandler/KLinePlot.h
        ItemHandler/LineDrawer.cpp
        ItemHandler/LineDrawer.h
        ItemHandler/MacdDivergenceItem.cpp
        ItemHandler/MacdDivergenceItem.h
        ItemHandler/MacdGraph.cpp
        ItemHandler/MacdGraph.h
        ItemHandler/MaGraph.cpp
        ItemHandler/MaGraph.h
        ItemHandler/MouseMoveItems.cpp
        ItemHandler/MouseMoveItems.h
        ItemHandler/TMatchesItems.cpp
        ItemHandler/TMatchesItems.h
        ItemHandler/TradeSideRect.cpp
        ItemHandler/TradeSideRect.h
        ItemHandler/VwMaGraph.cpp
        ItemHandler/VwMaGraph.h
)

set(KLINESHAPE_LIST
        klineShape/KLineShape.cpp
        klineShape/KLineShape.h
        klineShape/KLineShape_Div.cpp
        klineShape/KLineShape_Div.h
        klineShape/KShapeDefine.cpp
        klineShape/KShapeDefine.h
        klineShape/MakeScanPacket.cpp
        klineShape/MakeScanPacket.h
        klineShape/ScanKShape.cpp
        klineShape/ScanKShape.h
        klineShape/ScanKShape_Future.cpp
        klineShape/ScanKShape_Future.h
        klineShape/ScanKShape_Stock.cpp
        klineShape/ScanKShape_Stock.h
)
set(QCUSTOMPLOT_LIST
        qcustomplot/qcustomplot.h
        qcustomplot/qcustomplot.cpp
)
set(TABLEMODEL_LIST
        tableModel/TableModel.cpp
        tableModel/TableModel.h
        tableModel/TableModel_Ma.cpp
        tableModel/TableModel_Ma.h
        tableModel/TableModel_Ma_Mouse.cpp
        tableModel/TableModel_Ma_Mouse.h
        tableModel/TableModel_Macd.cpp
        tableModel/TableModel_Macd.h
)
set(TSIDETIMEHANDLER_LIST
        tSideTimeHandler/TSideTimeHandler.cpp
        tSideTimeHandler/TSideTimeHandler.h
        tSideTimeHandler/TSideTimeHandler_Future.cpp
        tSideTimeHandler/TSideTimeHandler_Future.h
        tSideTimeHandler/TSideTimeHandler_Stock.cpp
        tSideTimeHandler/TSideTimeHandler_Stock.h
)




set(SRC_LIST
        src/AnalystGui.cpp
        src/AnalystGui.h
        src/axistag.cpp
        src/axistag.h
        src/AxisTickerText_KLine.cpp
        src/AxisTickerText_KLine.h
        src/BracketWithArrow.cpp
        src/BracketWithArrow.h
        src/ColorManager.cpp
        src/ColorManager.h
        src/Factory_AnalystGui.cpp
        src/HtmlItemText.cpp
        src/HtmlItemText.h
        src/KLineUpdator.cpp
        src/KLineUpdator.h
        src/main.cpp
        src/PlotContainer.cpp
        src/PlotContainer.h
        src/PlotObject.cpp
        src/PlotObject.h

        ui/AnalystGui.ui
        resources/AnalystGui.qrc
)

add_executable(${TARGETNAME}
        ${SRC_LIST}
        ${ITEMHANDLER_LIST}
        ${KLINESHAPE_LIST}
        ${QCUSTOMPLOT_LIST}

        ${TABLEMODEL_LIST}
        ${TSIDETIMEHANDLER_LIST}

)

set_target_properties(${TARGETNAME} PROPERTIES
        AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

if(WIN32)
    target_sources(${TARGETNAME} PRIVATE
#            resources/win/AnalystGui.rc
    )
endif()



# 关键：只在构建本动态库时定义 FACTORY_CALCULATOR_EXPORTS
#target_compile_definitions(${TARGETNAME}
#        PRIVATE
#        FACTORY_CALCULATOR_EXPORTS   # 注意是 PRIVATE
#)

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)


target_include_directories(${TARGETNAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qcustomplot>
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:qcustomplot>
)

target_link_libraries(${TARGETNAME}
        PUBLIC
        IBSysDefine
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::PrintSupport
        Factory_AnalystDeal
        Factory_Calculator
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
add_custom_command(TARGET ${TARGETNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        "-Ddlls=$<JOIN:$<TARGET_RUNTIME_DLLS:${TARGETNAME}>,;>"
        "-Ddest_dir=${run_dir}"
        -P "${CMAKE_SOURCE_DIR}/cmake/copy_thirdparty_dlls.cmake"
        VERBATIM
)


