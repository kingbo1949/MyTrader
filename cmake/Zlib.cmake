

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Zlib::zlib)
    return()
endif()

if(NOT DEFINED ZLIB_ROOT)
    message(FATAL_ERROR "ZLIB_ROOT is not set. Define it via CMakePresets.json or -DZLIB_ROOT=...")
endif()

if(WIN32)
    # Windows：强制使用**静态库**
    add_library(Zlib::zlib STATIC IMPORTED GLOBAL)

    set_target_properties(Zlib::zlib PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_ROOT}/include"

            # 静态库只需要 .lib 文件（无 DLL、无 -shared 后缀）
            IMPORTED_LOCATION_DEBUG   "${ZLIB_ROOT}/debug/lib/zlibd.lib"
            IMPORTED_LOCATION_RELEASE "${ZLIB_ROOT}/lib/zlib.lib"
    )

endif()
