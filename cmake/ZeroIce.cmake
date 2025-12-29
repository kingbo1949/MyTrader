include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/bzip2.cmake")

# 防重复：只要核心组件存在即可
if(TARGET ZeroIce::all)
    return()
endif()

if(NOT DEFINED ICE_ROOT)
    message(FATAL_ERROR "ICE_ROOT is not set. Define it via CMakePresets.json or -DICE_ROOT=...")
endif()

message(STATUS "ICE_ROOT = '${ICE_ROOT}'")

# ---------------- Windows helper ----------------
function(_ice_import_dll_lib target_name dll_debug_path lib_debug_path dll_release_path lib_release_path)
    add_library(${target_name} SHARED IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_DEBUG   "${dll_debug_path}"
            IMPORTED_IMPLIB_DEBUG     "${lib_debug_path}"
            IMPORTED_LOCATION_RELEASE "${dll_release_path}"
            IMPORTED_IMPLIB_RELEASE   "${lib_release_path}"
    )
endfunction()

# ---------------- macOS helper ----------------
function(_ice_import_dylib target_name dylib_debug_path dylib_release_path)
    add_library(${target_name} SHARED IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_DEBUG   "${dylib_debug_path}"
            IMPORTED_LOCATION_RELEASE "${dylib_release_path}"
    )
endfunction()

# include：macOS 下你这里是 debug/include 与 release/include 各一份
# 一般应一致；我们固定使用 release/include（也可以改为 debug/include）
set(_zeroice_includes "${ICE_ROOT}/release/include")

# ---------------- Platform branches ----------------
if(WIN32)

    # 1) Ice 主库
    _ice_import_dll_lib(ZeroIce::ice
            "${ICE_ROOT}/bin/debug/ice37++11d.dll"
            "${ICE_ROOT}/lib/debug/ice37++11d.lib"
            "${ICE_ROOT}/bin/release/ice37++11.dll"
            "${ICE_ROOT}/lib/release/ice37++11.lib"
    )
    target_include_directories(ZeroIce::ice INTERFACE "${ICE_ROOT}/include")
    target_link_libraries(ZeroIce::ice INTERFACE Bzip2::bzip2)

    # 2) IceDiscovery
    _ice_import_dll_lib(ZeroIce::icediscovery
            "${ICE_ROOT}/bin/debug/icediscovery37++11d.dll"
            "${ICE_ROOT}/lib/debug/icediscovery37++11d.lib"
            "${ICE_ROOT}/bin/release/icediscovery37++11.dll"
            "${ICE_ROOT}/lib/release/icediscovery37++11.lib"
    )
    target_include_directories(ZeroIce::icediscovery INTERFACE "${ICE_ROOT}/include")

    # 3) IceLocatorDiscovery
    _ice_import_dll_lib(ZeroIce::icelocatordiscovery
            "${ICE_ROOT}/bin/debug/icelocatordiscovery37++11d.dll"
            "${ICE_ROOT}/lib/debug/icelocatordiscovery37++11d.lib"
            "${ICE_ROOT}/bin/release/icelocatordiscovery37++11.dll"
            "${ICE_ROOT}/lib/release/icelocatordiscovery37++11.lib"
    )
    target_include_directories(ZeroIce::icelocatordiscovery INTERFACE "${ICE_ROOT}/include")

    # 4) IceSSL
    _ice_import_dll_lib(ZeroIce::iceSSL
            "${ICE_ROOT}/bin/debug/icessl37++11d.dll"
            "${ICE_ROOT}/lib/debug/icessl37++11d.lib"
            "${ICE_ROOT}/bin/release/icessl37++11.dll"
            "${ICE_ROOT}/lib/release/icessl37++11.lib"
    )
    target_include_directories(ZeroIce::iceSSL INTERFACE "${ICE_ROOT}/include")

elseif(APPLE)

    # 关键点：
    # - 选择版本化真实库（...++11.37.dylib），不要选 19/24 字节的 symlink（...++11.dylib）
    # - debug/release 分别指向各自目录

    # 1) Ice 主库：libIce++11.37.dylib
    _ice_import_dylib(ZeroIce::ice
            "${ICE_ROOT}/debug/lib/libIce++11.37.dylib"
            "${ICE_ROOT}/release/lib/libIce++11.37.dylib"
    )
    target_include_directories(ZeroIce::ice INTERFACE "${_zeroice_includes}")
    target_link_libraries(ZeroIce::ice INTERFACE Bzip2::bzip2)

    # 2) IceDiscovery：libIceDiscovery++11.37.dylib
    _ice_import_dylib(ZeroIce::icediscovery
            "${ICE_ROOT}/debug/lib/libIceDiscovery++11.37.dylib"
            "${ICE_ROOT}/release/lib/libIceDiscovery++11.37.dylib"
    )
    target_include_directories(ZeroIce::icediscovery INTERFACE "${_zeroice_includes}")

    # 3) IceLocatorDiscovery：libIceLocatorDiscovery++11.37.dylib
    _ice_import_dylib(ZeroIce::icelocatordiscovery
            "${ICE_ROOT}/debug/lib/libIceLocatorDiscovery++11.37.dylib"
            "${ICE_ROOT}/release/lib/libIceLocatorDiscovery++11.37.dylib"
    )
    target_include_directories(ZeroIce::icelocatordiscovery INTERFACE "${_zeroice_includes}")

    # 4) IceSSL：libIceSSL++11.37.dylib
    _ice_import_dylib(ZeroIce::iceSSL
            "${ICE_ROOT}/debug/lib/libIceSSL++11.37.dylib"
            "${ICE_ROOT}/release/lib/libIceSSL++11.37.dylib"
    )
    target_include_directories(ZeroIce::iceSSL INTERFACE "${_zeroice_includes}")

else()
    message(FATAL_ERROR "Unsupported platform for ZeroC Ice (only WIN32 and APPLE are supported).")
endif()

# ---------- 门面 targets（聚合） ----------
add_library(ZeroIce::all INTERFACE IMPORTED GLOBAL)
target_link_libraries(ZeroIce::all INTERFACE
        ZeroIce::ice
        ZeroIce::icediscovery
        ZeroIce::icelocatordiscovery
        ZeroIce::iceSSL
)
