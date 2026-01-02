
add_subdirectory(Factory_HashEnv)
add_subdirectory(Factory_IBGlobalShare)
add_subdirectory(Factory_IBJson)
add_subdirectory(Factory_Log)
add_subdirectory(Factory_Monitor)
add_subdirectory(Factory_QDatabase)
add_subdirectory(Factory_QShareEnv)
add_subdirectory(Factory_Topics)
add_subdirectory(Factory_TShareEnv)
add_subdirectory(QtGlobalLib)

add_custom_target(layer_factory_base0)

add_dependencies(layer_factory_base0
        QtGlobalLib
        Factory_HashEnv
        Factory_IBGlobalShare
        Factory_IBJSon
        Factory_Log
        Factory_Monitor
        Factory_QDatabase
        Factory_QShareEnv
        Factory_Topics
        Factory_TShareEnv
)

