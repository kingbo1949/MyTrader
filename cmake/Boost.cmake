include_guard(GLOBAL)

# 防重复：只要核心组件存在即可
if(TARGET Boost::all)
    return()
endif()

if(NOT DEFINED BOOST_ROOT)
    message(FATAL_ERROR "BOOST_ROOT is not set. Define it via CMakePresets.json or -DBOOST_ROOT =...")
endif()

# 小工具：为 静态库 创建 imported target（同时支持 Debug/Release）
function(_boost_import_static target_name release_lib debug_lib)
    add_library(${target_name} STATIC IMPORTED GLOBAL)
    set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_RELEASE "${release_lib}"
            IMPORTED_LOCATION_DEBUG   "${debug_lib}"
    )
endfunction()

# 小工具：为某个 target 设置 includes（统一）
function(_boost_set_includes target_name includes_dir)
    target_include_directories(${target_name} INTERFACE "${includes_dir}")
endfunction()

# ---------------- Windows ----------------
if(WIN32)
    set(_boost_includes "${BOOST_ROOT}/include/boost-1_90")

    _boost_import_static(Boost::filesystem
            "${BOOST_ROOT}/lib/libboost_filesystem-vc143-mt-x64-1_90.lib"
            "${BOOST_ROOT}/lib/libboost_filesystem-vc143-mt-gd-x64-1_90.lib"
    )
    _boost_set_includes(Boost::filesystem "${_boost_includes}")

    _boost_import_static(Boost::regex
            "${BOOST_ROOT}/lib/libboost_regex-vc143-mt-x64-1_90.lib"
            "${BOOST_ROOT}/lib/libboost_regex-vc143-mt-gd-x64-1_90.lib"
    )
    _boost_set_includes(Boost::regex "${_boost_includes}")

    _boost_import_static(Boost::locale
            "${BOOST_ROOT}/lib/libboost_locale-vc143-mt-x64-1_90.lib"
            "${BOOST_ROOT}/lib/libboost_locale-vc143-mt-gd-x64-1_90.lib"
    )
    _boost_set_includes(Boost::locale "${_boost_includes}")

    # ---------------- macOS ----------------
elseif(APPLE)
    # 你的 macOS 安装结构：BOOST_ROOT=/Users/fantang/local/boost.1.70.0
    # include 在 ${BOOST_ROOT}/include，库在 ${BOOST_ROOT}/lib
    set(_boost_includes "${BOOST_ROOT}/include")
    set(_boost_libdir   "${BOOST_ROOT}/lib")


    _boost_import_static(Boost::filesystem
            "${_boost_libdir}/libboost_filesystem.a"
            "${_boost_libdir}/libboost_filesystem-mt-d-a64.a"
    )
    _boost_set_includes(Boost::filesystem "${_boost_includes}")

    _boost_import_static(Boost::regex
            "${_boost_libdir}/libboost_regex.a"
            "${_boost_libdir}/libboost_regex-mt-d-a64.a"
    )
    _boost_set_includes(Boost::regex "${_boost_includes}")

    _boost_import_static(Boost::locale
            "${_boost_libdir}/libboost_locale.a"
            "${_boost_libdir}/libboost_locale-mt-d-a64.a"
    )
    _boost_set_includes(Boost::locale "${_boost_includes}")

else()
    message(FATAL_ERROR "Unsupported platform for this Boost import file (only WIN32 and APPLE are supported).")
endif()

# ---------- 你工程定义的“门面 targets”（聚合） ----------
add_library(Boost::all INTERFACE IMPORTED GLOBAL)
target_link_libraries(Boost::all INTERFACE
        Boost::filesystem
        Boost::regex
        Boost::locale
)

