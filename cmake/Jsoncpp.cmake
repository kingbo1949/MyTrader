

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Jsoncpp::json)
    return()
endif()

if(NOT DEFINED JSONCPP_ROOT)
    message(FATAL_ERROR "JSONCPP_ROOT is not set. Define it via CMakePresets.json or -DJSONCPP_ROOT =...")
endif()

# 你已确认是 DLL，因此用 SHARED IMPORTED
add_library(Jsoncpp::json SHARED IMPORTED GLOBAL)

# 头文件目录
set_target_properties(Jsoncpp::json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "${JSONCPP_ROOT}/include"
)

# Debug: dll + import lib
set_target_properties(Jsoncpp::json PROPERTIES
        IMPORTED_LOCATION_DEBUG
        "${JSONCPP_ROOT}/lib/debug/jsoncpp.dll"
        IMPORTED_IMPLIB_DEBUG
        "${JSONCPP_ROOT}/lib/debug/jsoncpp.lib"
)

# Release: 如果你有对应文件，强烈建议补齐
set_target_properties(Jsoncpp::json PROPERTIES
        IMPORTED_LOCATION_RELEASE
        "${JSONCPP_ROOT}/lib/release/jsoncpp.dll"
        IMPORTED_IMPLIB_RELEASE
        "${JSONCPP_ROOT}/lib/release/jsoncpp.lib"
)


