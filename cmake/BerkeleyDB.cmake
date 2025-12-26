

include_guard(GLOBAL)

# 避免重复定义
if(TARGET BerkeleyDB::db)
    return()
endif()

if(NOT DEFINED BERKELEYDB_ROOT)
    message(FATAL_ERROR "BERKELEYDB_ROOT is not set. Define it via CMakePresets.json or -DBERKELEYDB_ROOT=...")
endif()

# 你已确认是 DLL，因此用 SHARED IMPORTED
add_library(BerkeleyDB::db SHARED IMPORTED GLOBAL)

# 头文件目录
set_target_properties(BerkeleyDB::db PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "${BERKELEYDB_ROOT}/include"
)

# Debug: dll + import lib
set_target_properties(BerkeleyDB::db PROPERTIES
        IMPORTED_LOCATION_DEBUG
        "${BERKELEYDB_ROOT}/lib/libdb62d.dll"
        IMPORTED_IMPLIB_DEBUG
        "${BERKELEYDB_ROOT}/lib/libdb62d.lib"
)

# Release: 如果你有对应文件，强烈建议补齐
 set_target_properties(BerkeleyDB::db PROPERTIES
   IMPORTED_LOCATION_RELEASE
     "${BERKELEYDB_ROOT}/lib/libdb62.dll"
   IMPORTED_IMPLIB_RELEASE
     "${BERKELEYDB_ROOT}/lib/libdb62.lib"
 )

