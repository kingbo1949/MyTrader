#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required(VERSION 4.0)
project(MyTrader VERSION 1.0 LANGUAGES CXX)
include(GNUInstallDirs)


#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 建议添加：禁止编译器特定的扩展，确保代码标准、跨平台

# 针对 MSVC 编译器的特殊配置（修复 sys_time 找不到的问题）
if(MSVC)
    add_compile_options(
            /Zc:__cplusplus   # 强制编译器报告正确的 C++ 版本号，解决头文件定义隐藏问题
            /utf-8            # 确保代码中的中文字符串不会乱码
            /permissive-      # 启用严格的标准符合性模式
    )

endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin") # .exe / .dll
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin") # .dll (windows)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib") # .lib (import/static)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(build_options)   # 注意：文件名是 build_options.cmake
include(Dependencies)  # 统一创建所有外部 imported targets
include(setup_runtime)

add_subdirectory(layer_lib)
add_subdirectory(layer_factory_base0)
add_subdirectory(layer_factory_base1)
add_subdirectory(layer_factory_app)
add_subdirectory(layer_app)


# add_executable(MyTrader main.cpp)

# target_link_libraries(MyTrader PRIVATE IBSysDefine)

