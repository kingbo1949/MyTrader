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
message(STATUS "ZeroIce include = '${ICE_ROOT}/include'")

function(_ice_import_dll_lib target_name dll_debug_path lib_debug_path dll_release_path lib_release_path)
    add_library(${target_name} SHARED IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_DEBUG "${dll_debug_path}"
            IMPORTED_IMPLIB_DEBUG   "${lib_debug_path}"
            IMPORTED_LOCATION_RELEASE "${dll_release_path}"
            IMPORTED_IMPLIB_RELEASE   "${lib_release_path}"
    )
endfunction()

set(_zeroice_includes "${ICE_ROOT}/include")

# 1) Ice 主库
_ice_import_dll_lib(ZeroIce::ice
        "${ICE_ROOT}/bin/debug/ice37++11d.dll"
        "${ICE_ROOT}/lib/debug/ice37++11d.lib"
        "${ICE_ROOT}/bin/release/ice37++11.dll"
        "${ICE_ROOT}/lib/release/ice37++11.lib"
)
set_target_properties(ZeroIce::ice PROPERTIES INTERFACE_LINK_LIBRARIES "Bzip2::bzip2")
target_include_directories(ZeroIce::ice INTERFACE "${_zeroice_includes}")



# 2) IceDiscovery
_ice_import_dll_lib(ZeroIce::icediscovery
        "${ICE_ROOT}/bin/debug/icediscovery37++11d.dll"
        "${ICE_ROOT}/lib/debug/icediscovery37++11d.lib"
        "${ICE_ROOT}/bin/release/icediscovery37++11.dll"
        "${ICE_ROOT}/lib/release/icediscovery37++11.lib"
)
target_include_directories(ZeroIce::icediscovery INTERFACE "${_zeroice_includes}")

# 3) Icelocatordiscovery
_ice_import_dll_lib(ZeroIce::icelocatordiscovery
        "${ICE_ROOT}/bin/debug/icelocatordiscovery37++11d.dll"
        "${ICE_ROOT}/lib/debug/icelocatordiscovery37++11d.lib"
        "${ICE_ROOT}/bin/release/icelocatordiscovery37++11.dll"
        "${ICE_ROOT}/lib/release/icelocatordiscovery37++11.lib"
)
target_include_directories(ZeroIce::icelocatordiscovery INTERFACE "${_zeroice_includes}")

# 3) IceSSL
_ice_import_dll_lib(ZeroIce::iceSSL
        "${ICE_ROOT}/bin/debug/icessl37++11d.dll"
        "${ICE_ROOT}/lib/debug/icessl37++11d.lib"
        "${ICE_ROOT}/bin/release/icessl37++11.dll"
        "${ICE_ROOT}/lib/release/icessl37++11.lib"
)
target_include_directories(ZeroIce::iceSSL INTERFACE "${_zeroice_includes}")

# ---------- 你工程定义的“门面 targets”（聚合） ----------
add_library(ZeroIce::all INTERFACE IMPORTED GLOBAL)
target_link_libraries(ZeroIce::all INTERFACE
        ZeroIce::ice
        ZeroIce::icediscovery
        ZeroIce::icelocatordiscovery
        ZeroIce::iceSSL
)

