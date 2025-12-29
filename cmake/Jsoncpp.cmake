include_guard(GLOBAL)

# 避免重复定义
if(TARGET Jsoncpp::json)
    return()
endif()

if(NOT DEFINED JSONCPP_ROOT)
    message(FATAL_ERROR "JSONCPP_ROOT is not set. Define it via CMakePresets.json or -DJSONCPP_ROOT=...")
endif()

# ---------------- Windows ----------------
if(WIN32)
    # 你已确认是 DLL，因此用 SHARED IMPORTED
    add_library(Jsoncpp::json SHARED IMPORTED GLOBAL)

    set_target_properties(Jsoncpp::json PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${JSONCPP_ROOT}/include"

            IMPORTED_LOCATION_DEBUG   "${JSONCPP_ROOT}/lib/debug/jsoncpp.dll"
            IMPORTED_IMPLIB_DEBUG     "${JSONCPP_ROOT}/lib/debug/jsoncpp.lib"

            IMPORTED_LOCATION_RELEASE "${JSONCPP_ROOT}/lib/release/jsoncpp.dll"
            IMPORTED_IMPLIB_RELEASE   "${JSONCPP_ROOT}/lib/release/jsoncpp.lib"
    )

    return()
endif()

# ---------------- macOS ----------------
if(APPLE)
    # macOS 下用 .dylib/.a，没有 import lib
    add_library(Jsoncpp::json SHARED IMPORTED GLOBAL)

    set_target_properties(Jsoncpp::json PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${JSONCPP_ROOT}/include/json"

            # 选择“真实文件”而不是 symlink：你截图里就是带版本号的 .dylib
            IMPORTED_LOCATION_RELEASE "${JSONCPP_ROOT}/lib/libjsoncpp.1.9.5.dylib"
            IMPORTED_LOCATION_DEBUG   "${JSONCPP_ROOT}/lib/libjsoncpp_debug.1.9.5.dylib"
    )

    return()
endif()

message(FATAL_ERROR "Unsupported platform for jsoncpp.cmake (only WIN32 and APPLE are supported).")
