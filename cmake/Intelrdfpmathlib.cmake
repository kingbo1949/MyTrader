

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Intelrdfpmath::intelrdfpmath)
    return()
endif()

if(NOT DEFINED INTERRDFPMATH_ROOT)
    message(FATAL_ERROR "INTERRDFPMATH_ROOT is not set. Define it via CMakePresets.json or -DINTERRDFPMATH_ROOT =...")
endif()

add_library(Intelrdfpmath::intelrdfpmath STATIC IMPORTED GLOBAL)

# 头文件目录
set_target_properties(Intelrdfpmath::intelrdfpmath PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "${INTERRDFPMATH_ROOT}/include"
)

# lib
#set_target_properties(Intelrdfpmath::intelrdfpmath PROPERTIES
#        IMPORTED_IMPLIB
#        "${INTERRDFPMATH_ROOT}/lib/libbid.lib"
#        IMPORTED_IMPLIB_DEBUG
#        "${INTERRDFPMATH_ROOT}/lib/libbid.lib"
#)

set_target_properties(Intelrdfpmath::intelrdfpmath PROPERTIES
        IMPORTED_LOCATION_DEBUG        "${INTERRDFPMATH_ROOT}/lib/libbid.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${INTERRDFPMATH_ROOT}/lib/libbid.lib"
        IMPORTED_LOCATION_MINSIZEREL   "${INTERRDFPMATH_ROOT}/lib/libbid.lib"
        IMPORTED_LOCATION_RELEASE      "${INTERRDFPMATH_ROOT}/lib/libbid.lib"
)


