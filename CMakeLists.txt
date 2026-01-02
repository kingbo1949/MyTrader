#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required(VERSION 4.0)
project(MyTrader VERSION 1.0 LANGUAGES CXX)


#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


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

