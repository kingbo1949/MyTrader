

add_subdirectory(BdbLib)
add_subdirectory(IBSysDefine)
add_subdirectory(IceCompiler)
add_subdirectory(ToolLib)
if(WIN32)
    add_subdirectory(IBSourceLib)
    return()
endif()

add_custom_target(layer_lib)
if(WIN32)
    add_dependencies(layer_lib
            BdbLib
            IBSourceLib
            IBSysDefine
            IceCompiler
            ToolLib
    )
    return()
endif()

if(APPLE)
    add_dependencies(layer_lib
            BdbLib
            IBSysDefine
            IceCompiler
            ToolLib
    )
    return()
endif()
