set(TARGETNAME "RocksDbDemo")

add_executable(${TARGETNAME} main.cpp)

target_link_libraries(${TARGETNAME} PRIVATE
        MyTrader::build_options
        RocksDbLib
)

if(NOT WIN32)
    find_package(Iconv REQUIRED)
    target_link_libraries(${TARGETNAME} PRIVATE Iconv::Iconv)
endif()
