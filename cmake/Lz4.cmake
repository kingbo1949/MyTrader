

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Lz4::lz4)
    return()
endif()


if(WIN32)
    if(NOT DEFINED LZ4_ROOT)
        message(FATAL_ERROR "LZ4_ROOT is not set. Define it via CMakePresets.json or -DLZ4_ROOT=...")
    endif()

    # Windows：强制使用**静态库**（lz4.lib）
    add_library(Lz4::lz4 STATIC IMPORTED GLOBAL)

    set_target_properties(Lz4::lz4 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LZ4_ROOT}/include"

            # 静态库只需要 .lib 文件（无 DLL、无 -shared 后缀）
            IMPORTED_LOCATION_DEBUG   "${LZ4_ROOT}/debug/lib/lz4d.lib"
            IMPORTED_LOCATION_RELEASE "${LZ4_ROOT}/lib/lz4d.lib"
    )

endif()
