include_guard(GLOBAL)

# 防重复：只要核心组件存在即可
if(TARGET Boost::all)
    return()
endif()

if(NOT DEFINED BOOST_ROOT)
    message(FATAL_ERROR "BOOST_ROOT is not set. Define it via CMakePresets.json or -DBOOST_ROOT =...")
endif()

# 小工具：为 静态库 创建 imported target
function(_boost_import_lib target_name release_lib debug_lib)
    add_library(${target_name} STATIC IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_RELEASE "${release_lib}"
            IMPORTED_LOCATION_DEBUG   "${debug_lib}"
    )
endfunction()

set(_boost_includes "${BOOST_ROOT}")

# 1) system
_boost_import_lib(Boost::system
        "${BOOST_ROOT}/lib32-msvc-142/libboost_system-vc142-mt-x64-1_70.lib"
        "${BOOST_ROOT}/lib32-msvc-142/libboost_system-vc142-mt-gd-x64-1_70.lib"
)
target_include_directories(Boost::system INTERFACE "${_boost_includes}")

# 2) filesystem
_boost_import_lib(Boost::filesystem
        "${BOOST_ROOT}/lib32-msvc-142/libboost_filesystem-vc142-mt-x64-1_70.lib"
        "${BOOST_ROOT}/lib32-msvc-142/libboost_filesystem-vc142-mt-gd-x64-1_70.lib"

)
target_include_directories(Boost::filesystem INTERFACE "${_boost_includes}")

# 3) regex
_boost_import_lib(Boost::regex
        "${BOOST_ROOT}/lib32-msvc-142/libboost_regex-vc142-mt-x64-1_70.lib"
        "${BOOST_ROOT}/lib32-msvc-142/libboost_regex-vc142-mt-gd-x64-1_70.lib"
)
target_include_directories(Boost::regex INTERFACE "${_boost_includes}")

# 4) local
_boost_import_lib(Boost::local
        "${BOOST_ROOT}/lib32-msvc-142/libboost_locale-vc142-mt-x64-1_70.lib"
        "${BOOST_ROOT}/lib32-msvc-142/libboost_locale-vc142-mt-gd-x64-1_70.lib"
)
target_include_directories(Boost::regex INTERFACE "${_boost_includes}")


# ---------- 你工程定义的“门面 targets”（聚合） ----------
add_library(Boost::all INTERFACE IMPORTED GLOBAL)
target_link_libraries(Boost::all INTERFACE Boost::system Boost::filesystem Boost::regex Boost::local)

