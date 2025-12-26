

include_guard(GLOBAL)

# 避免重复定义
if(TARGET Concurrentqueue::queue)
    return()
endif()

if(NOT DEFINED CONCURRENTQUEUE_ROOT)
    message(FATAL_ERROR "CONCURRENTQUEUE_ROOT is not set. Define it via CMakePresets.json or -DCONCURRENTQUEUE_ROOT =...")
endif()

# 仅包含头文件的库，因此用 INTERFACE IMPORTED
add_library(Concurrentqueue::queue INTERFACE IMPORTED GLOBAL)

# 头文件目录
set_target_properties(Concurrentqueue::queue PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        "${CONCURRENTQUEUE_ROOT}"
)

