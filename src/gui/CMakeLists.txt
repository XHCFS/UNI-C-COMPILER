set(CMAKE_AUTOMOC ON)

add_library(gui STATIC
    MainWindow.cpp
    LexerPanel.cpp
    ParserPanel.cpp
)
target_include_directories(gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gui PUBLIC common lexer parser Qt${QT_VERSION_MAJOR}::Widgets)
