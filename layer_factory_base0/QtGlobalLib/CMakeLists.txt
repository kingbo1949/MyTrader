
set(TARGETNAME "QtGlobalLib")
set(SRC_LIST
        src/QtGlobal.cpp
        src/stdafx.cpp

)

add_library(${TARGETNAME} STATIC ${SRC_LIST})

target_link_libraries(${TARGETNAME} PRIVATE MyTrader::build_options)





target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)


target_link_libraries(${TARGETNAME}
        PUBLIC
            Qt6::Core
            Qt6::Widgets
        PRIVATE
            IBSysDefine
            Factory_IBGlobalShare
            ToolLib
)