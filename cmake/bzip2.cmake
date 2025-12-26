

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Bzip2::bzip2)
    return()
endif()

if(NOT DEFINED BZIP2_ROOT)
    message(FATAL_ERROR "BZIP2_ROOT is not set. Define it via CMakePresets.json or -DBZIP2_ROOT=...")
endif()

# 你已确认是 DLL，因此用 SHARED IMPORTED
add_library(Bzip2::bzip2 SHARED IMPORTED GLOBAL)

# 头文件目录
set_target_properties(Bzip2::bzip2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "${BZIP2_ROOT}/include"
)

# Debug: dll + import lib
set_target_properties(Bzip2::bzip2 PROPERTIES
        IMPORTED_LOCATION_DEBUG
        "${BZIP2_ROOT}/debug/bin/bz2d.dll"
        IMPORTED_IMPLIB_DEBUG
        "${BZIP2_ROOT}/debug/lib/bz2d.lib"
)

# Release: 如果你有对应文件，强烈建议补齐
set_target_properties(Bzip2::bzip2 PROPERTIES
        IMPORTED_LOCATION_RELEASE
        "${BZIP2_ROOT}/bin/bz2.dll"
        IMPORTED_IMPLIB_RELEASE
        "${BZIP2_ROOT}/lib/bz2.lib"
)

