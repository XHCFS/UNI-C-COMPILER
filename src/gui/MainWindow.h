#pragma once
#include <QMainWindow>
#include <QPlainTextEdit>
#include "LexerPanel.h"
#include "ParserPanel.h"

// The application's main window: source-code editor on the left, and on the
// right a vertical splitter containing the LexerPanel (top) and ParserPanel
// (bottom). The user runs the full source -> tokens -> AST pipeline via
// the Analyze menu (F5 = lexer only, F6 = lexer + parser).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    // Constructs the window, builds menus/toolbar/central widget, and shows a status message.
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    // Reads source from the editor, runs the Lexer, and forwards results to LexerPanel.
    void onRunLexer();

    // Runs the full pipeline: tokenize -> push to LexerPanel -> parse -> push to ParserPanel.
    void onRunParser();

    // Clears the source editor and BOTH output panels.
    void onClear();

    // Opens a file dialog, loads the selected .c/.h file into the editor.
    void onOpenFile();

private:
    QPlainTextEdit* editor_;        // Source-code editor where the user types or pastes C code.
    LexerPanel*     lexerPanel_;    // Lexer output panel: Tokens / Symbol Table / Errors.
    ParserPanel*    parserPanel_;   // Parser output panel: Parse Tree / Errors.

    // Creates the File and Analyze menu entries with their keyboard shortcuts.
    void buildMenus();

    // Creates the toolbar with Open, Run Lexer, Run Parser, and Clear actions.
    void buildToolbar();

    // Creates the central splitter: editor on the left; on the right, a vertical
    // splitter holding lexerPanel_ on top and parserPanel_ on bottom.
    void buildCentral();
};
