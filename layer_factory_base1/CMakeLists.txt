

add_subdirectory(Factory_AnalystDeal)
add_subdirectory(Factory_Callback)
add_subdirectory(Factory_Simulator)
add_subdirectory(Factory_UnifyInterface)

add_custom_target(layer_factory_base1)

add_dependencies(layer_factory_base1
        Factory_AnalystDeal
        Factory_Callback
        Factory_Simulator
        Factory_UnifyInterface
)

