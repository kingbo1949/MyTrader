

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Spdlog::log)
    return()
endif()

if(NOT DEFINED SPDLOG_ROOT)
    message(FATAL_ERROR "SPDLOG_ROOT is not set. Define it via CMakePresets.json or -DSPDLOG_ROOT =...")
endif()

# 仅包含头文件的库，因此用 INTERFACE IMPORTED
add_library(Spdlog::log INTERFACE IMPORTED GLOBAL)

# 头文件目录
set_target_properties(Spdlog::log PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "${SPDLOG_ROOT}/.."
)

