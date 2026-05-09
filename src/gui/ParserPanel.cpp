#include "ParserPanel.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QString>
#include <sstream>
#include <vector>
using namespace std;

// Builds the two tabs and lays them out in a vertical layout matching
// LexerPanel's structure.
ParserPanel::ParserPanel(QWidget* parent) : QWidget(parent) {
    // --- Parse Tree tab ---
    tree_ = new QTreeWidget(this);
    tree_->setColumnCount(1);
    tree_->setHeaderLabel("Node");
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree_->setAlternatingRowColors(true);
    QFont treeFont("Consolas", 10);
    treeFont.setStyleHint(QFont::Monospace);
    tree_->setFont(treeFont);

    // --- Errors tab ---
    errorList_ = new QListWidget(this);
    QFont errFont("Consolas", 9);
    errFont.setStyleHint(QFont::Monospace);
    errorList_->setFont(errFont);

    // --- Tabs ---
    tabs_ = new QTabWidget(this);
    tabs_->addTab(tree_,      "Parse Tree");
    tabs_->addTab(errorList_, "Errors");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs_);
    setLayout(layout);
}

// Forwards results into both tabs, updates titles with counts, and switches
// to the appropriate tab based on whether errors were reported.
void ParserPanel::showResults(const TranslationUnit* root,
                               const vector<ParseError>& errors) {
    // Tree
    populateTree(root);

    // Errors
    errorList_->clear();
    for (const auto& e : errors)
        errorList_->addItem(QString::fromStdString(e.toString()));

    // Tab titles with counts.
    int declCount = root ? static_cast<int>(root->getDecls().size()) : 0;
    tabs_->setTabText(0, QString("Parse Tree (%1)").arg(declCount));
    tabs_->setTabText(1, QString("Errors (%1)").arg(errors.size()));

    // Auto-switch: errors front-and-center when present.
    tabs_->setCurrentIndex(errors.empty() ? 0 : 1);
}

// Empties both widgets and resets titles to their no-count form.
void ParserPanel::clear() {
    tree_->clear();
    errorList_->clear();
    tabs_->setTabText(0, "Parse Tree");
    tabs_->setTabText(1, "Errors");
    tabs_->setCurrentIndex(0);
}

// Walks the AST by leveraging AstNode::dump(). The dump format guarantees
// 2 leading spaces per depth level on every line, so we can reconstruct the
// tree by counting the indent of each line and parenting accordingly.
//
// We use this indirect approach so AstNode (in the common library) stays free
// of Qt dependencies — the GUI converts text -> tree, the AST converts tree
// -> text. The two layers don't know about each other.
void ParserPanel::populateTree(const TranslationUnit* root) {
    tree_->clear();
    if (!root) return;

    ostringstream oss;
    root->dump(oss, 0);

    vector<QTreeWidgetItem*> stack;   // index = depth; back() is current parent
    istringstream iss(oss.str());
    string line;
    while (getline(iss, line)) {
        if (line.empty()) continue;
        size_t spaces = 0;
        while (spaces < line.size() && line[spaces] == ' ') ++spaces;
        size_t depth = spaces / 2;        // dump() uses 2 spaces per level
        string label = line.substr(spaces);

        // Pop stack so it represents only ancestors of the new item.
        while (stack.size() > depth) stack.pop_back();

        QTreeWidgetItem* item = stack.empty()
            ? new QTreeWidgetItem(tree_)
            : new QTreeWidgetItem(stack.back());
        item->setText(0, QString::fromStdString(label));
        stack.push_back(item);
    }

    tree_->expandAll();
}
