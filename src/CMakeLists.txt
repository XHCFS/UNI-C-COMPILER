add_subdirectory(common)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(gui)

# Main executable
add_executable(ccompiler main.cpp)
target_link_libraries(ccompiler
    PRIVATE
        common
        lexer
        parser
        gui
        Qt${QT_VERSION_MAJOR}::Widgets
)
target_include_directories(ccompiler PRIVATE ${CMAKE_SOURCE_DIR}/src)
