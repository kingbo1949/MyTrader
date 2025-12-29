

include_guard(GLOBAL)

# 1) 实体 target（不带 ::）
add_library(build_options INTERFACE)

# 2) 命名空间别名（对外使用）
add_library(MyTrader::build_options ALIAS build_options)

# 3) 工程统一编译标准与宏
target_compile_features(build_options INTERFACE cxx_std_17)


if(APPLE)
    target_compile_definitions(build_options INTERFACE
            ICE_CPP11_MAPPING
    )

    return()
endif()

if(WIN32)
    target_compile_definitions(build_options INTERFACE
            WIN32_LEAN_AND_MEAN
            ICE_CPP11_MAPPING
    )

    return()
endif()



