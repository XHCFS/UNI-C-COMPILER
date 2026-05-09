#include "MainWindow.h"
#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QString>
#include "SymbolTable.h"
#include "Lexer.h"
#include "Parser.h"
#include "ParseError.h"
#include "Ast.h"
using namespace std;

// Sets the window title, fixes the initial size, and builds menus, toolbar, and central widget.
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Lexer");
    resize(1280, 760);
    buildMenus();
    buildToolbar();
    buildCentral();
    statusBar()->showMessage("Ready  |  Open a file or type source, F5 = lexer, F6 = lexer + parser.");
}

// Creates the File menu (Open, Exit) and Analyze menu (Run Lexer, Run Parser, Clear)
// with keyboard shortcuts.
void MainWindow::buildMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* openAct = fileMenu->addAction("&Open...");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpenFile);

    fileMenu->addSeparator();

    QAction* exitAct = fileMenu->addAction("E&xit");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Analyze menu (was "Lexer" — now hosts both phases)
    QMenu* analyzeMenu = menuBar()->addMenu("&Analyze");

    QAction* runLexAct = analyzeMenu->addAction("&Run Lexer");
    runLexAct->setShortcut(Qt::Key_F5);
    connect(runLexAct, &QAction::triggered, this, &MainWindow::onRunLexer);

    QAction* runParAct = analyzeMenu->addAction("Run &Parser");
    runParAct->setShortcut(Qt::Key_F6);
    connect(runParAct, &QAction::triggered, this, &MainWindow::onRunParser);

    QAction* clearAct = analyzeMenu->addAction("&Clear");
    clearAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(clearAct, &QAction::triggered, this, &MainWindow::onClear);
}

// Adds Open, Run Lexer, Run Parser, and Clear buttons to a non-movable toolbar.
void MainWindow::buildToolbar() {
    QToolBar* tb = addToolBar("Main");
    tb->setMovable(false);

    QAction* openAct = tb->addAction("Open");
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpenFile);

    tb->addSeparator();

    QAction* runLexAct = tb->addAction("Run Lexer  [F5]");
    connect(runLexAct, &QAction::triggered, this, &MainWindow::onRunLexer);

    QAction* runParAct = tb->addAction("Run Parser  [F6]");
    connect(runParAct, &QAction::triggered, this, &MainWindow::onRunParser);

    QAction* clearAct = tb->addAction("Clear");
    connect(clearAct, &QAction::triggered, this, &MainWindow::onClear);
}

// Creates the editor on the left and a vertical results splitter on the right
// containing the LexerPanel (top) and ParserPanel (bottom). Both nested in an
// outer horizontal splitter.
void MainWindow::buildCentral() {
    // Source editor
    editor_ = new QPlainTextEdit(this);
    QFont editorFont("Consolas", 11);
    editorFont.setStyleHint(QFont::Monospace);
    editor_->setFont(editorFont);
    editor_->setLineWrapMode(QPlainTextEdit::NoWrap);
    editor_->setTabStopDistance(28);
    editor_->setPlaceholderText("Enter or paste C source code here...");

    auto* leftWidget  = new QWidget(this);
    auto* leftLayout  = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(4, 4, 2, 4);
    leftLayout->addWidget(new QLabel("Source", leftWidget));
    leftLayout->addWidget(editor_);

    // Output panels
    lexerPanel_  = new LexerPanel(this);
    parserPanel_ = new ParserPanel(this);

    // Inner vertical splitter: lexer panel on top, parser panel on bottom.
    auto* resultsSplitter = new QSplitter(Qt::Vertical, this);
    resultsSplitter->addWidget(lexerPanel_);
    resultsSplitter->addWidget(parserPanel_);
    resultsSplitter->setStretchFactor(0, 1);
    resultsSplitter->setStretchFactor(1, 1);
    resultsSplitter->setSizes({400, 360});

    // Outer horizontal splitter: editor | results.
    auto* outerSplitter = new QSplitter(Qt::Horizontal, this);
    outerSplitter->addWidget(leftWidget);
    outerSplitter->addWidget(resultsSplitter);
    outerSplitter->setStretchFactor(0, 1);
    outerSplitter->setStretchFactor(1, 1);
    outerSplitter->setSizes({480, 800});

    setCentralWidget(outerSplitter);
}

// Counts entries in the lexer lane (scope == -1) of a composite-keyed SymbolTable.
// The full table also holds parser-lane entries (scope >= 0) after a Run Parser,
// which would inflate the "identifiers seen by the lexer" count.
static size_t lexerLaneCount(const SymbolTable& table) {
    size_t n = 0;
    for (const auto& [key, entry] : table.getAllEntries())
        if (key.scope == -1) ++n;
    return n;
}

// Reads source from the editor, constructs a fresh SymbolTable and Lexer, runs tokenize(),
// forwards all results to LexerPanel, and updates the status bar with counts. Leaves the
// ParserPanel untouched — F5 is the lexer-only run.
void MainWindow::onRunLexer() {
    const string src = editor_->toPlainText().toStdString();
    if (src.empty()) {
        statusBar()->showMessage("Nothing to lex.");
        return;
    }

    SymbolTable table;
    Lexer lexer(src, table);
    const vector<Token>       tokens = lexer.tokenize();
    const vector<LexerError>& errors = lexer.getErrors();

    lexerPanel_->showResults(tokens, table, errors);

    statusBar()->showMessage(
        QString("%1 token(s)  |  %2 identifier(s)  |  %3 error(s)")
            .arg(tokens.size())
            .arg(lexerLaneCount(table))
            .arg(errors.size()));
}

// Full source -> tokens -> AST pipeline (F6). Runs the lexer first, forwards
// results to LexerPanel, then runs the parser over the same token vector and
// SymbolTable, forwards to ParserPanel. Lexer errors do NOT abort the parser —
// the parser is given whatever token vector the lexer produced, so the user
// can see partial parser output alongside lexer errors.
void MainWindow::onRunParser() {
    const string src = editor_->toPlainText().toStdString();
    if (src.empty()) {
        statusBar()->showMessage("Nothing to analyze.");
        return;
    }

    // --- Lexer phase ---
    SymbolTable table;
    Lexer lexer(src, table);
    const vector<Token>       tokens   = lexer.tokenize();
    const vector<LexerError>& lexErrs  = lexer.getErrors();
    lexerPanel_->showResults(tokens, table, lexErrs);

    // --- Parser phase (same SymbolTable: lexer wrote scope=-1, parser writes scope>=0) ---
    Parser parser(tokens, table);
    auto                       ast      = parser.parse();
    const vector<ParseError>&  parErrs  = parser.getErrors();
    parserPanel_->showResults(ast.get(), parErrs);

    int declCount = ast ? static_cast<int>(ast->getDecls().size()) : 0;
    statusBar()->showMessage(
        QString("%1 token(s)  |  %2 identifier(s)  |  %3 lex err  |  %4 decl(s)  |  %5 parse err")
            .arg(tokens.size())
            .arg(lexerLaneCount(table))
            .arg(lexErrs.size())
            .arg(declCount)
            .arg(parErrs.size()));
}

// Resets the source editor and BOTH output panels to their empty initial state.
void MainWindow::onClear() {
    editor_->clear();
    lexerPanel_->clear();
    parserPanel_->clear();
    statusBar()->showMessage("Cleared.");
}

// Opens a file dialog filtered to .c and .h files, loads the chosen file into the editor,
// and updates the window title and status bar with the file name.
void MainWindow::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open C Source File", "",
        "C Source Files (*.c *.h);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Open File",
            "Cannot open: " + path);
        return;
    }

    editor_->setPlainText(QTextStream(&file).readAll());
    setWindowTitle("UNI-C Compiler  -  " + QFileInfo(path).fileName());
    statusBar()->showMessage("Loaded: " + path);
}
