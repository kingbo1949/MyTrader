set(TARGETNAME "IBSourceLib")
set(SRC_LIST
        src/ContractCondition.cpp
        src/DefaultEWrapper.cpp
        src/EClient.cpp
        src/EClientSocket.cpp
        src/EDecoder.cpp
        src/EMessage.cpp
        src/EMutex.cpp
        src/EOrderDecoder.cpp
        src/EReader.cpp
        src/EReaderOSSignal.cpp
        src/ESocket.cpp
        src/executioncondition.cpp
        src/MarginCondition.cpp
        src/OperatorCondition.cpp
        src/OrderCondition.cpp
        src/PercentChangeCondition.cpp
        src/PriceCondition.cpp
        src/SoftDollarTier.cpp
        src/StdAfx.cpp
        src/TimeCondition.cpp
        src/Utils.cpp
        src/VolumeCondition.cpp



)

add_library(${TARGETNAME} STATIC ${SRC_LIST})

target_link_libraries(${TARGETNAME}
        PRIVATE
        MyTrader::build_options
        Boost::all
)





target_include_directories(${TARGETNAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE: include>

)