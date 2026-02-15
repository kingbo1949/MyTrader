

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Zstd::zstd)
    return()
endif()


if(WIN32)
    if(NOT DEFINED ZSTD_ROOT)
        message(FATAL_ERROR "ZSTD_ROOT is not set. Define it via CMakePresets.json or -DZSTD_ROOT=...")
    endif()

    # Windows：强制使用**静态库**（zstd.lib）
    add_library(Zstd::zstd STATIC IMPORTED GLOBAL)

    set_target_properties(Zstd::zstd PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_ROOT}/include"

            # 静态库只需要 .lib 文件（无 DLL、无 -shared 后缀）
            IMPORTED_LOCATION_DEBUG   "${ZSTD_ROOT}/debug/lib/zstd.lib"
            IMPORTED_LOCATION_RELEASE "${ZSTD_ROOT}/lib/zstd.lib"
    )

endif()
