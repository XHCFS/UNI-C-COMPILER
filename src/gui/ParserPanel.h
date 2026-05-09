#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <vector>
#include "Ast.h"
#include "ParseError.h"
using namespace std;


// A tabbed output widget that displays the two result sets produced by the
// Parser: a parse-tree view and an error list. Layout, fonts, and tab-title-
// with-count behaviour mirror LexerPanel so the two panels feel uniform inside
// the unified MainWindow described in Parser_Design.md §5.1.
class ParserPanel : public QWidget {
    Q_OBJECT
public:
    // Constructs the panel, creating and configuring both tab widgets.
    explicit ParserPanel(QWidget* parent = nullptr);

    // Populates both tabs with the supplied parser results, updates tab titles
    // with counts, and auto-switches to the Errors tab when errors are present
    // (otherwise switches to the Parse Tree tab). Same call shape as
    // LexerPanel::showResults.
    void showResults(const TranslationUnit* root,
                     const vector<ParseError>& errors);

    // Clears both tabs and resets tab titles to their no-count form.
    void clear();

private:
    QTabWidget*  tabs_;       // Tab container.
    QTreeWidget* tree_;       // Parse Tree tab.
    QListWidget* errorList_;  // Errors tab.

    // Populates tree_ by walking the AST. Reuses AstNode::dump(): renders the
    // tree to a string with 2-space indents per depth, then builds one
    // QTreeWidgetItem per line, wiring parent/child by indent.
    void populateTree(const TranslationUnit* root);
};
