include_guard(GLOBAL)

# 避免重复定义
if(TARGET Bzip2::bzip2)
    return()
endif()

# ---------------- macOS：用系统自带 bzip2 ----------------
if(APPLE)
    # 让 CMake 自己找（通常是 /usr/lib/libbz2.dylib + SDK include）
    find_package(BZip2 REQUIRED)

    message(STATUS "BZip2 include: ${BZIP2_INCLUDE_DIR}")
    message(STATUS "BZip2 libs   : ${BZIP2_LIBRARIES}")

    # 用 INTERFACE target 包装一下，统一你工程里的命名空间：Bzip2::bzip2
    add_library(Bzip2::bzip2 INTERFACE IMPORTED GLOBAL)
    set_target_properties(Bzip2::bzip2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${BZIP2_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES      "${BZIP2_LIBRARIES}"
    )

    # 可选：防止意外链接到 Homebrew 的 bzip2（你已明确要用系统版本）
    if(BZIP2_LIBRARIES MATCHES "/opt/homebrew")
        message(FATAL_ERROR "BZip2 resolved to Homebrew path, but system BZip2 is required: ${BZIP2_LIBRARIES}")
    endif()

    return()
endif()

# ---------------- Windows：沿用你现有手写导入 ----------------
if(WIN32)
    if(NOT DEFINED BZIP2_ROOT)
        message(FATAL_ERROR "BZIP2_ROOT is not set. Define it via CMakePresets.json or -DBZIP2_ROOT=...")
    endif()

    # DLL 模式
    add_library(Bzip2::bzip2 SHARED IMPORTED GLOBAL)

    set_target_properties(Bzip2::bzip2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${BZIP2_ROOT}/include"

            IMPORTED_LOCATION_DEBUG   "${BZIP2_ROOT}/debug/bin/bz2d.dll"
            IMPORTED_IMPLIB_DEBUG     "${BZIP2_ROOT}/debug/lib/bz2d.lib"

            IMPORTED_LOCATION_RELEASE "${BZIP2_ROOT}/bin/bz2.dll"
            IMPORTED_IMPLIB_RELEASE   "${BZIP2_ROOT}/lib/bz2.lib"
    )

    return()
endif()

message(FATAL_ERROR "Unsupported platform for bzip2.cmake (only WIN32 and APPLE are supported).")
