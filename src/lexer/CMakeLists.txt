add_library(lexer STATIC
    Lexer.cpp
    CharStream.cpp
    LexerError.cpp
    ReservedWords.cpp
    TokenClassifier.cpp
)
target_include_directories(lexer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(lexer PUBLIC common)
