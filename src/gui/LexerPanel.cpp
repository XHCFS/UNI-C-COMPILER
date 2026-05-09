#include "LexerPanel.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QString>
#include <algorithm>
#include <vector>
using namespace std;
// Constructs and configures the three tab widgets (token table, symbol table, error list)
// and assembles them into a QTabWidget inside a vertical layout.
LexerPanel::LexerPanel(QWidget* parent) : QWidget(parent) {
    // --- Token table ---
    tokenTable_ = new QTableWidget(0, 5, this);
    tokenTable_->setHorizontalHeaderLabels({"#", "Type", "Lexeme", "Line", "Col"});
    tokenTable_->verticalHeader()->hide();
    {
        auto* h = tokenTable_->horizontalHeader();
        h->setMinimumSectionSize(40);
        h->setSectionResizeMode(0, QHeaderView::Fixed);       // #      — narrow, fixed
        h->setSectionResizeMode(1, QHeaderView::Interactive); // Type   — user-resizable
        h->setSectionResizeMode(2, QHeaderView::Stretch);     // Lexeme — takes remaining space
        h->setSectionResizeMode(3, QHeaderView::Fixed);       // Line   — narrow, fixed
        h->setSectionResizeMode(4, QHeaderView::Fixed);       // Col    — narrow, fixed
        tokenTable_->setColumnWidth(0,  48);
        tokenTable_->setColumnWidth(1, 160);
        tokenTable_->setColumnWidth(3,  60);
        tokenTable_->setColumnWidth(4,  60);
    }
    tokenTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tokenTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tokenTable_->setAlternatingRowColors(true);

    // --- Symbol table ---
    symbolTable_ = new QTableWidget(0, 2, this);
    symbolTable_->setHorizontalHeaderLabels({"Identifier", "First seen (line)"});
    symbolTable_->verticalHeader()->hide();
    {
        auto* h = symbolTable_->horizontalHeader();
        h->setSectionResizeMode(0, QHeaderView::Stretch);  // Identifier — takes remaining space
        h->setSectionResizeMode(1, QHeaderView::Fixed);    // Line       — narrow, fixed
        symbolTable_->setColumnWidth(1, 120);
    }
    symbolTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    symbolTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    symbolTable_->setAlternatingRowColors(true);

    // --- Error list ---
    errorList_ = new QListWidget(this);
    QFont errFont("Consolas", 9);
    errFont.setStyleHint(QFont::Monospace);
    errorList_->setFont(errFont);

    // --- Tabs ---
    tabs_ = new QTabWidget(this);
    tabs_->addTab(tokenTable_,  "Tokens");
    tabs_->addTab(symbolTable_, "Symbol Table");
    tabs_->addTab(errorList_,   "Errors");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs_);
    setLayout(layout);
}

// Fills the token table row-by-row, sorts symbol-table entries by first-seen line,
// populates the error list, updates tab title counts, and switches to the Errors tab if needed.
void LexerPanel::showResults(const vector<Token>& tokens,
                              const SymbolTable& table,
                              const vector<LexerError>&  errors)
{
    // Helper: create a center-aligned numeric cell
    auto numCell = [](int n) {
        auto* item = new QTableWidgetItem(QString::number(n));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    // Tokens tab
    tokenTable_->setRowCount(static_cast<int>(tokens.size()));
    for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
        const Token& t = tokens[i];
        tokenTable_->setItem(i, 0, numCell(i));
        tokenTable_->setItem(i, 1, new QTableWidgetItem(
            QString::fromStdString(tokenTypeToString(t.getType()))));
        tokenTable_->setItem(i, 2, new QTableWidgetItem(
            QString::fromStdString(t.getLexeme())));
        tokenTable_->setItem(i, 3, numCell(t.getLine()));
        tokenTable_->setItem(i, 4, numCell(t.getColumn()));
    }

    // Symbol table tab — sort by first-occurrence line so entries appear in source order.
    // getAllEntries() returns an unordered_map whose iteration order is arbitrary.
    // Filter to the lexer lane (scope == -1); parser-lane entries (scope >= 0)
    // belong to the future ParserPanel.
    const auto& entries = table.getAllEntries();
    vector<const SymbolEntry*> sorted;
    sorted.reserve(entries.size());
    for (const auto& [key, entry] : entries) {
        if (key.scope == -1)
            sorted.push_back(&entry);
    }
    sort(sorted.begin(), sorted.end(),
        [](const SymbolEntry* a, const SymbolEntry* b) {
            return a->getLine() != b->getLine()
                ? a->getLine()   < b->getLine()
                : a->getToken().getColumn() < b->getToken().getColumn();
        });

    symbolTable_->setRowCount(static_cast<int>(sorted.size()));
    int row = 0;
    for (const SymbolEntry* entry : sorted) {
        symbolTable_->setItem(row, 0, new QTableWidgetItem(
            QString::fromStdString(entry->getName())));
        symbolTable_->setItem(row, 1, numCell(entry->getLine()));
        ++row;
    }

    // Errors tab — switch to it automatically if there are any
    errorList_->clear();
    for (const auto& e : errors)
        errorList_->addItem(QString::fromStdString(e.toString()));

    tabs_->setTabText(0, QString("Tokens (%1)").arg(tokens.size()));
    tabs_->setTabText(1, QString("Symbol Table (%1)").arg(entries.size()));
    tabs_->setTabText(2, QString("Errors (%1)").arg(errors.size()));

    if (!errors.empty())
        tabs_->setCurrentIndex(2);
    else
        tabs_->setCurrentIndex(0);
}

// Empties all three tables/lists, resets tab title counts, and switches back to the Tokens tab.
void LexerPanel::clear() {
    tokenTable_->setRowCount(0);
    symbolTable_->setRowCount(0);
    errorList_->clear();
    tabs_->setTabText(0, "Tokens");
    tabs_->setTabText(1, "Symbol Table");
    tabs_->setTabText(2, "Errors");
    tabs_->setCurrentIndex(0);
}
