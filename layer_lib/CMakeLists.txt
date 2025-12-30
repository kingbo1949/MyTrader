

add_subdirectory(BdbLib)
add_subdirectory(IBSysDefine)
add_subdirectory(IceCompiler)
add_subdirectory(ToolLib)
add_subdirectory(IBSourceLib)
add_custom_target(layer_lib)

add_dependencies(layer_lib
        BdbLib
        IBSourceLib
        IBSysDefine
        IceCompiler
        ToolLib
)

