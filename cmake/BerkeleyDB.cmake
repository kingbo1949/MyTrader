

include_guard(GLOBAL)

# 避免重复定义
if(TARGET BerkeleyDB::db)
    return()
endif()

if(NOT DEFINED BERKELEYDB_ROOT)
    message(FATAL_ERROR "BERKELEYDB_ROOT is not set. Define it via CMakePresets.json or -DBERKELEYDB_ROOT=...")
endif()



# --- Windows ---
if(WIN32)
    add_library(BerkeleyDB::db SHARED IMPORTED GLOBAL)

    set_target_properties(BerkeleyDB::db PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${BERKELEYDB_ROOT}/include"

            IMPORTED_LOCATION_DEBUG "${BERKELEYDB_ROOT}/lib/libdb62d.dll"
            IMPORTED_IMPLIB_DEBUG   "${BERKELEYDB_ROOT}/lib/libdb62d.lib"

            IMPORTED_LOCATION_RELEASE "${BERKELEYDB_ROOT}/lib/libdb62.dll"
            IMPORTED_IMPLIB_RELEASE   "${BERKELEYDB_ROOT}/lib/libdb62.lib"
    )

    # --- macOS (Apple Silicon / Intel 都可) ---
elseif(APPLE)
    add_library(BerkeleyDB::db UNKNOWN IMPORTED GLOBAL)
    set_target_properties(BerkeleyDB::db PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${BERKELEYDB_ROOT}/release/include"

            IMPORTED_LOCATION_DEBUG   "${BERKELEYDB_ROOT}/debug/lib/libdb_cxx-6.2.dylib"
            IMPORTED_LOCATION_RELEASE "${BERKELEYDB_ROOT}/release/lib/libdb_cxx-6.2.dylib"
    )

else()
    message(FATAL_ERROR "Unsupported platform for BerkeleyDB (only WIN32 and APPLE are supported in this project).")
endif()