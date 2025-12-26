
# copy_thirdparty_dlls.cmake
# 输入参数：
#   dlls     : 分号分隔的 DLL 路径列表
#   dest_dir : 目标工作目录

if(NOT DEFINED dlls OR NOT DEFINED dest_dir)
    message(FATAL_ERROR "need -Ddlls=... -Ddest_dir=...")
endif()

file(MAKE_DIRECTORY "${dest_dir}")

# dlls 是 ; 分隔列表
foreach(dll IN LISTS dlls)
    if(dll STREQUAL "")
        continue()
    endif()

    # 只要路径中包含 ThirdParty，就认为是第三方 DLL
    string(FIND "${dll}" "ThirdParty" _pos)
    if(_pos EQUAL -1)
        continue()
    endif()

    # copy_if_different 已经帮你避免无意义覆盖
    execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${dll}"
            "${dest_dir}"
    )
endforeach()
