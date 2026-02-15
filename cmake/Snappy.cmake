include_guard(GLOBAL)

# 避免重复定义
if(TARGET Snappy::snappy)
    return()
endif()


if(WIN32)
    if(NOT DEFINED SNAPPY_ROOT)
        message(FATAL_ERROR "SNAPPY_ROOT is not set. Define it via CMakePresets.json or -DSNAPPY_ROOT=...")
    endif()

    # Windows：强制使用**静态库**（zstd.lib）
    add_library(Snappy::snappy STATIC IMPORTED GLOBAL)

    set_target_properties(Snappy::snappy PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SNAPPY_ROOT}/include"

            # 静态库只需要 .lib 文件（无 DLL、无 -shared 后缀）
            IMPORTED_LOCATION_DEBUG   "${SNAPPY_ROOT}/debug/lib/snappy.lib"
            IMPORTED_LOCATION_RELEASE "${SNAPPY_ROOT}/lib/snappy.lib"
    )

endif()
