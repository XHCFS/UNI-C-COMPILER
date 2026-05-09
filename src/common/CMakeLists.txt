add_library(common STATIC
    Token.cpp
    SymbolEntry.cpp
    SymbolTable.cpp
    Type.cpp
    Ast.cpp
)
target_include_directories(common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
