

add_subdirectory(JsonCsvTransfer)
add_subdirectory(QAnalyst)
add_subdirectory(QIBDatabase)
add_subdirectory(QTransKLineFormIB)
add_subdirectory(StLoader)
add_subdirectory(AnalystGui)
add_subdirectory(QuoteGenerator)

add_custom_target(layer_app)

add_dependencies(layer_app
        JsonCsvTransfer
        QAnalyst
        QIBDatabase
        QTransKLineFormIB
        StLoader
        QuoteGenerator
        AnalystGui
)
