

add_subdirectory(Factory_AnalystDeal)
add_subdirectory(Factory_Callback)
add_subdirectory(Factory_Simulator)

if(WIN32)
    add_subdirectory(Factory_UnifyInterface)
endif()



add_custom_target(layer_factory_base1)


if(WIN32)
    add_dependencies(layer_factory_base1
            Factory_AnalystDeal
            Factory_Callback
            Factory_Simulator
            Factory_UnifyInterface
    )
endif()

if(APPLE)
    add_dependencies(layer_factory_base1
            Factory_AnalystDeal
            Factory_Callback
            Factory_Simulator
    )
endif()



