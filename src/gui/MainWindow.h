#pragma once
#include <QMainWindow>
#include <QPlainTextEdit>
#include "LexerPanel.h"

// The application's main window: a source-code editor on the left and
// a tabbed lexer-output panel on the right, connected by a splitter.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    // Constructs the window, builds menus/toolbar/central widget, and shows a status message.
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    // Reads source from the editor, runs the Lexer, and forwards results to LexerPanel.
    void onRunLexer();

    // Clears both the source editor and the LexerPanel output.
    void onClear();

    // Opens a file dialog, loads the selected .c/.h file into the editor.
    void onOpenFile();

private:
    QPlainTextEdit* editor_;      // Source-code editor where the user types or pastes C code.
    LexerPanel* lexerPanel_;  // Output panel that shows tokens, symbol table, and errors.

    // Creates the File and Lexer menu entries with their keyboard shortcuts.
    void buildMenus();

    // Creates the toolbar with Open, Run Lexer, and Clear actions.
    void buildToolbar();

    // Creates the central splitter containing the editor and the lexer panel.
    void buildCentral();
};
