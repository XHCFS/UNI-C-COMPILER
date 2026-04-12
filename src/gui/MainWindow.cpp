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
using namespace std;

// Sets the window title, fixes the initial size, and builds menus, toolbar, and central widget.
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Lexer");
    resize(1280, 760);
    buildMenus();
    buildToolbar();
    buildCentral();
    statusBar()->showMessage("Ready  |  Open a file or type source, then press F5 to run the lexer.");
}

// Creates the File menu (Open, Exit) and Lexer menu (Run Lexer, Clear) with keyboard shortcuts.
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

    // Lexer menu
    QMenu* lexMenu = menuBar()->addMenu("&Lexer");

    QAction* runAct = lexMenu->addAction("&Run Lexer");
    runAct->setShortcut(Qt::Key_F5);
    connect(runAct, &QAction::triggered, this, &MainWindow::onRunLexer);

    QAction* clearAct = lexMenu->addAction("&Clear");
    clearAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(clearAct, &QAction::triggered, this, &MainWindow::onClear);
}

// Adds Open, Run Lexer, and Clear buttons to a non-movable toolbar at the top of the window.
void MainWindow::buildToolbar() {
    QToolBar* tb = addToolBar("Main");
    tb->setMovable(false);

    QAction* openAct = tb->addAction("Open");
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpenFile);

    tb->addSeparator();

    QAction* runAct = tb->addAction("Run Lexer  [F5]");
    connect(runAct, &QAction::triggered, this, &MainWindow::onRunLexer);

    QAction* clearAct = tb->addAction("Clear");
    connect(clearAct, &QAction::triggered, this, &MainWindow::onClear);
}

// Creates the monospaced source editor, the LexerPanel, and places them side-by-side in a splitter.
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

    // Lexer output panel
    lexerPanel_ = new LexerPanel(this);

    // Horizontal splitter: editor | results
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftWidget);
    splitter->addWidget(lexerPanel_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({480, 800});

    setCentralWidget(splitter);
}

// Reads source from the editor, constructs a fresh SymbolTable and Lexer, runs tokenize(),
// forwards all results to LexerPanel, and updates the status bar with counts.
void MainWindow::onRunLexer() {
    const string src = editor_->toPlainText().toStdString();
    if (src.empty()) {
        statusBar()->showMessage("Nothing to lex.");
        return;
    }

    SymbolTable table;
    Lexer lexer(src, table);
    const vector<Token>      tokens = lexer.tokenize();
    const vector<LexerError>& errors = lexer.getErrors();

    lexerPanel_->showResults(tokens, table, errors);

    statusBar()->showMessage(
        QString("%1 token(s)  |  %2 identifier(s)  |  %3 error(s)")
            .arg(tokens.size())
            .arg(table.getAllEntries().size())
            .arg(errors.size()));
}

// Resets both the source editor and the LexerPanel to their empty initial state.
void MainWindow::onClear() {
    editor_->clear();
    lexerPanel_->clear();
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
