add_library(common STATIC
    Token.cpp
    SymbolTable.cpp
)
target_include_directories(common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
