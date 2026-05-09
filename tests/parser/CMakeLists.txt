add_executable(test_parser
    test_parser.cpp
)
target_link_libraries(test_parser PRIVATE parser common)
target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME ParserTests COMMAND test_parser)
