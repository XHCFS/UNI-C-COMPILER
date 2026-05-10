add_executable(test_parser
    test_parser.cpp
)
target_link_libraries(test_parser PRIVATE parser lexer common)
target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Pass the fixtures directory in as a compile-time string so the test binary
# can locate declarations.c / expressions.c / statements.c / errors.c.
target_compile_definitions(test_parser PRIVATE
    PARSER_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME ParserTests COMMAND test_parser)
