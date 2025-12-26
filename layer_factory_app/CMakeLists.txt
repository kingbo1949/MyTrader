

add_subdirectory(Factory_StOperator)
add_subdirectory(Factory_Strategys)
add_subdirectory(Factory_Setup)

add_custom_target(layer_factory_app)

add_dependencies(layer_factory_app
        Factory_StOperator
        Factory_Strategys
        Factory_Setup
)
