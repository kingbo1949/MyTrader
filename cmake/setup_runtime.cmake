

include_guard(GLOBAL)

# 指定target编译完成后创建目录
function(setup_makedir app_target dir)
    add_custom_command(TARGET ${app_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${dir}"
            VERBATIM
    )

endfunction()

# 指定target编译完成后拷贝文件
function(setup_cp app_target config_src config_dst_rel)

    add_custom_command(TARGET ${app_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${config_src}"
            "${config_dst_rel}"
            VERBATIM
    )

endfunction()


function(generate_debug_env TARGET_RUN_PATH TARGET_NAME)
    # 使用生成器表达式动态获取该 Target 运行时需要的 DLL/SO 路径
    # 这仅在 CMake 3.21+ 支持，是非常强大的功能
    set(ENV_CONTENT "PATH=$<TARGET_RUNTIME_DLLS:${TARGET_NAME}>:$PATH")

    file(GENERATE
            OUTPUT "${TARGET_RUN_PATH}/${TARGET_NAME}.env"
            CONTENT "${ENV_CONTENT}"
    )
endfunction()