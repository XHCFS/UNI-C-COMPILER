#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QListWidget>
#include <vector>
#include "Token.h"
#include "SymbolTable.h"
#include "LexerError.h"
using namespace std;
// A tabbed output widget that displays the three result sets produced by the Lexer:
// a token table, a symbol-table listing, and an error list.
class LexerPanel : public QWidget {
    Q_OBJECT
public:
    // Constructs the panel, creating and configuring all three tab widgets.
    explicit LexerPanel(QWidget* parent = nullptr);

    // Populates all three tabs with the supplied lexer results and switches to
    // the Errors tab automatically if any errors are present.
    void showResults(const vector<Token>&      tokens,
                     const SymbolTable&              table,
                     const vector<LexerError>&  errors);

    // Clears all rows from every tab and resets the tab title counts to zero.
    void clear();

private:
    QTabWidget*   tabs_;         // Tab container that houses the three output views.
    QTableWidget* tokenTable_;   // Five-column table: index, type, lexeme, line, column.
    QTableWidget* symbolTable_;  // Two-column table: identifier name and first-seen line.
    QListWidget*  errorList_;    // Scrollable list of formatted LexerError strings.
};
