add_library(lexer STATIC
    Lexer.cpp
)
target_include_directories(lexer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(lexer PUBLIC common)
