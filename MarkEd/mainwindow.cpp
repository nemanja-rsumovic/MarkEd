#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <QFrame>
#include <QScrollBar>

#include "mdrenderer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("MarkEd");
    resize(1280, 760);

    setupLayout();
    setupMenuBar();
    setupToolBar();
    setupFileTree();
    setupEditor();
    setupPreview();
    setupStatusBar();
    applyStylesheet();

    previewTimer = new QTimer(this);
    previewTimer->setSingleShot(true);
    previewTimer->setInterval(300);
    connect(previewTimer, &QTimer::timeout, this, &MainWindow::updatePreview);
    connect(editor, &QPlainTextEdit::textChanged, this, [this]() {
        previewTimer->start();
        markUnsaved();
    });
    connect(editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::onCursorPositionChanged);
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::syncScrollToPreview);

    updatePreview();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* Layout */

void MainWindow::setupLayout()
{
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setHandleWidth(1);

    // Side panel
    sidePanel = new QWidget();
    sidePanel->setObjectName("sidePanel");
    QVBoxLayout* sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);

    QLabel* filesLabel = new QLabel("  FILES");
    filesLabel->setObjectName("panelLabel");
    filesLabel->setFixedHeight(32);

    fileTree = new QTreeView();
    sideLayout->addWidget(filesLabel);
    sideLayout->addWidget(fileTree);

    // Editor panel
    editorPanel = new QWidget();
    editorPanel->setObjectName("editorPanel");
    QVBoxLayout* editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* editorLabel = new QLabel("  EDITOR");
    editorLabel->setObjectName("panelLabel");
    editorLabel->setFixedHeight(32);

    editor = new QPlainTextEdit();
    editor->setObjectName("editor");
    editorLayout->addWidget(editorLabel);
    editorLayout->addWidget(editor);

    // Preview panel
    previewPanel = new QWidget();
    previewPanel->setObjectName("previewPanel");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewPanel);
    previewLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* previewLabel = new QLabel("  PREVIEW");
    previewLabel->setObjectName("panelLabel");
    previewLabel->setFixedHeight(32);

    preview = new QWebEngineView();
    previewLayout->addWidget(previewLabel);
    previewLayout->addWidget(preview);

    mainSplitter->addWidget(sidePanel);
    mainSplitter->addWidget(editorPanel);
    mainSplitter->addWidget(previewPanel);
    mainSplitter->setSizes({220, 530, 530});

    setCentralWidget(mainSplitter);
}

/* Menu Bar */

void MainWindow::setupMenuBar()
{
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("New",         this, &MainWindow::newFile,    QKeySequence::New);
    fileMenu->addAction("Open Folder", this, &MainWindow::openFolder, QKeySequence("Ctrl+Shift+O"));
    fileMenu->addSeparator();
    fileMenu->addAction("Save",        this, &MainWindow::saveFile,   QKeySequence::Save);
    fileMenu->addAction("Save As...",  this, &MainWindow::saveFileAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Export as PDF", this, &MainWindow::exportPdf, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", qApp, &QApplication::quit, QKeySequence::Quit);

    QMenu* viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction("Toggle File Tree", this, &MainWindow::toggleFileTree, QKeySequence("Ctrl+\\"));
    viewMenu->addAction("Toggle Preview",   this, &MainWindow::togglePreview,  QKeySequence("Ctrl+Shift+P"));
    viewMenu->addSeparator();
    viewMenu->addAction("Zoom In",    this, &MainWindow::zoomIn,    QKeySequence("Ctrl++"));
    viewMenu->addAction("Zoom Out",   this, &MainWindow::zoomOut,   QKeySequence("Ctrl+-"));
    viewMenu->addAction("Reset Zoom", this, &MainWindow::zoomReset, QKeySequence("Ctrl+0"));

    QMenu* insertMenu = menuBar()->addMenu("Insert");
    insertMenu->addAction("Bold",           this, &MainWindow::insertBold,          QKeySequence("Ctrl+B"));
    insertMenu->addAction("Italic",         this, &MainWindow::insertItalic,        QKeySequence("Ctrl+I"));
    insertMenu->addAction("Strikethrough",  this, &MainWindow::insertStrikethrough, QKeySequence("Ctrl+Shift+S"));
    insertMenu->addSeparator();
    insertMenu->addAction("Heading 1",      this, &MainWindow::insertH1);
    insertMenu->addAction("Heading 2",      this, &MainWindow::insertH2);
    insertMenu->addAction("Heading 3",      this, &MainWindow::insertH3);
    insertMenu->addSeparator();
    insertMenu->addAction("Link",           this, &MainWindow::insertLink,      QKeySequence("Ctrl+K"));
    insertMenu->addAction("Code Block",     this, &MainWindow::insertCodeBlock, QKeySequence("Ctrl+Shift+C"));
    insertMenu->addAction("Horizontal Rule",this, &MainWindow::insertHR);
}

/* Toolbar */

void MainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar("Format");
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    auto addBtn = [&](const QString& text, const QString& tip, auto slot) {
        QAction* a = toolbar->addAction(text);
        a->setToolTip(tip);
        connect(a, &QAction::triggered, this, slot);
        return a;
    };

    actionToggleTree = toolbar->addAction("☰");
    actionToggleTree->setToolTip("Toggle file tree (Ctrl+\\)");
    connect(actionToggleTree, &QAction::triggered, this, &MainWindow::toggleFileTree);
    toolbar->addSeparator();

    addBtn("B",   "Bold (Ctrl+B)",          &MainWindow::insertBold);
    addBtn("I",   "Italic (Ctrl+I)",         &MainWindow::insertItalic);
    addBtn("S",   "Strikethrough",           &MainWindow::insertStrikethrough);
    toolbar->addSeparator();
    addBtn("H1",  "Heading 1",               &MainWindow::insertH1);
    addBtn("H2",  "Heading 2",               &MainWindow::insertH2);
    addBtn("H3",  "Heading 3",               &MainWindow::insertH3);
    toolbar->addSeparator();
    addBtn("Link","Insert Link (Ctrl+K)",    &MainWindow::insertLink);
    addBtn("</>", "Code Block",              &MainWindow::insertCodeBlock);
    addBtn("—",   "Horizontal Rule",         &MainWindow::insertHR);
}

/* File Tree */

void MainWindow::setupFileTree()
{
    fsModel = new QFileSystemModel(this);
    fsModel->setNameFilters({"*.md"});
    fsModel->setNameFilterDisables(false);
    fsModel->setRootPath(QDir::homePath());

    fileTree->setModel(fsModel);
    fileTree->setRootIndex(fsModel->index(QDir::homePath()));
    fileTree->hideColumn(1);
    fileTree->hideColumn(2);
    fileTree->hideColumn(3);
    fileTree->setHeaderHidden(true);
    fileTree->setAnimated(true);
    fileTree->setIndentation(14);

    connect(fileTree, &QTreeView::doubleClicked,
            this, &MainWindow::onFileDoubleClicked);
}

/* Editor */

void MainWindow::setupEditor()
{
    QFont font("Consolas", 13);
    font.setStyleHint(QFont::Monospace);
    editor->setFont(font);
    editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    editor->setTabStopDistance(28);
    editor->setPlaceholderText("Start writing Markdown...");

    highlighter = new MarkdownHighlighter(editor->document());
}

/* Preview */

void MainWindow::setupPreview()
{
    preview->setContextMenuPolicy(Qt::NoContextMenu);
}

/* Status Bar */

void MainWindow::setupStatusBar()
{
    lblCursorPos = new QLabel("Ln 1  Col 1");
    lblWordCount = new QLabel("Words: 0");

    lblCursorPos->setObjectName("statusLabel");
    lblWordCount->setObjectName("statusLabel");

    statusBar()->addPermanentWidget(lblWordCount);
    statusBar()->addPermanentWidget(lblCursorPos);
    statusBar()->showMessage("Ready");
}

/* Stylesheet */

void MainWindow::applyStylesheet()
{
    // setStyleSheet prima CSS-like string koji se kaskadno primenjuje na sve child widgete
    setStyleSheet(R"(
QMainWindow, QWidget {
    background-color: #0d1117;
    color: #e6edf3;
}
QMenuBar {
    background-color: #161b22;
    color: #c9d1d9;
    border-bottom: 1px solid #21262d;
    padding: 2px;
}
QMenuBar::item:selected {
    background-color: #21262d;
    border-radius: 4px;
}
QMenu {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 4px;
}
QMenu::item {
    padding: 5px 24px;
    border-radius: 4px;
    color: #c9d1d9;
}
QMenu::item:selected {
    background-color: #21262d;
    color: #e6edf3;
}
QMenu::separator {
    height: 1px;
    background: #21262d;
    margin: 4px 8px;
}
QToolBar#mainToolbar {
    background-color: #161b22;
    border-bottom: 1px solid #21262d;
    padding: 3px 6px;
    spacing: 2px;
}
QToolBar#mainToolbar QToolButton {
    background-color: transparent;
    color: #c9d1d9;
    border: 1px solid transparent;
    border-radius: 5px;
    padding: 3px 9px;
    font-size: 12px;
    font-weight: 600;
    min-width: 24px;
}
QToolBar#mainToolbar QToolButton:hover {
    background-color: #21262d;
    border-color: #30363d;
    color: #58a6ff;
}
QToolBar#mainToolbar QToolButton:pressed {
    background-color: #30363d;
}
QToolBar::separator {
    width: 1px;
    background: #30363d;
    margin: 4px 3px;
}
QWidget#sidePanel {
    background-color: #161b22;
    border-right: 1px solid #21262d;
}
QWidget#editorPanel {
    background-color: #0d1117;
    border-right: 1px solid #21262d;
}
QWidget#previewPanel {
    background-color: #0d1117;
}
QLabel#panelLabel {
    background-color: #161b22;
    color: #484f58;
    font-size: 9px;
    font-weight: bold;
    border-bottom: 1px solid #21262d;
    padding-left: 4px;
}
QPlainTextEdit#editor {
    background-color: #0d1117;
    color: #e6edf3;
    border: none;
    font-family: "Consolas", "Courier New", monospace;
    font-size: 13px;
    selection-background-color: #1f6feb;
    selection-color: #ffffff;
    padding: 16px;
}
QTreeView {
    background-color: #161b22;
    color: #c9d1d9;
    border: none;
    font-size: 12px;
    outline: none;
}
QTreeView::item {
    padding: 3px 4px;
    border-radius: 4px;
}
QTreeView::item:hover {
    background-color: #21262d;
}
QTreeView::item:selected {
    background-color: #1f6feb;
    color: #ffffff;
}
QTreeView::branch {
    background-color: #161b22;
}
QSplitter::handle {
    background-color: #21262d;
}
QScrollBar:vertical {
    background: transparent;
    width: 6px;
}
QScrollBar::handle:vertical {
    background: #30363d;
    border-radius: 3px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover { background: #484f58; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal {
    background: transparent;
    height: 6px;
}
QScrollBar::handle:horizontal {
    background: #30363d;
    border-radius: 3px;
    min-width: 20px;
}
QScrollBar::handle:horizontal:hover { background: #484f58; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QStatusBar {
    background-color: #161b22;
    color: #484f58;
    border-top: 1px solid #21262d;
    font-size: 11px;
    padding: 0 8px;
}
QLabel#statusLabel {
    color: #484f58;
    font-size: 11px;
    padding: 0 8px;
    background: transparent;
}
    )");
}

/* Preview Rendering */

void MainWindow::updatePreview()
{
    preview->setHtml(buildHtml(renderMarkdown(editor->toPlainText())));
    updateWordCount();
}

QString MainWindow::buildHtml(const QString& body) const
{
    // Omotava renderovani HTML fragment u kompletan dokument sa CSS temom za preview
    return QString(R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif;
    font-size: 15px;
    line-height: 1.7;
    color: #e6edf3;
    background-color: #0d1117;
    padding: 32px 48px;
    max-width: 860px;
    margin: 0 auto;
}
h1,h2,h3,h4,h5,h6 { color: #e6edf3; margin: 24px 0 12px; font-weight: 600; line-height: 1.25; }
h1 { font-size: 2em;    padding-bottom: 8px;  border-bottom: 1px solid #30363d; }
h2 { font-size: 1.5em;  padding-bottom: 6px;  border-bottom: 1px solid #30363d; }
h3 { font-size: 1.25em; }
p  { margin-bottom: 16px; }
a  { color: #58a6ff; text-decoration: none; }
a:hover { text-decoration: underline; }
code {
    background: #161b22; color: #f0883e;
    padding: 2px 6px; border-radius: 4px;
    font-family: 'Consolas','Courier New',monospace; font-size: 0.9em;
}
pre {
    background: #161b22; border: 1px solid #30363d;
    border-radius: 6px; padding: 16px;
    overflow-x: auto; margin-bottom: 16px;
}
pre code { background: none; padding: 0; color: #e6edf3; font-size: 0.875em; }
blockquote {
    border-left: 4px solid #30363d; padding: 0 16px;
    color: #7d8590; margin-bottom: 16px;
}
ul,ol { padding-left: 2em; margin-bottom: 16px; }
li { margin-bottom: 4px; }
table { border-collapse: collapse; width: 100%; margin-bottom: 16px; }
th { background: #161b22; font-weight: 600; }
th,td { border: 1px solid #30363d; padding: 8px 13px; text-align: left; }
tr:nth-child(even) td { background: #111820; }
img { max-width: 100%; border-radius: 4px; }
hr  { border: none; border-top: 1px solid #30363d; margin: 24px 0; }
strong { font-weight: 600; }
del { color: #7d8590; }
</style>
</head>
<body>%1</body>
</html>)").arg(body);
}

/* File Operations - New File */

void MainWindow::newFile()
{
    if (unsavedChanges)
    {
        auto btn = QMessageBox::question(this, "Unsaved changes",
            "Save current file before creating a new one?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (btn == QMessageBox::Cancel) return;
        if (btn == QMessageBox::Save)   saveFile();
    }

    editor->clear();
    currentFilePath.clear();
    unsavedChanges = false;
    setWindowTitle("MarkEd");
}

/* File Operations - Open Foleder */

void MainWindow::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Open Folder", QDir::homePath());
    if (dir.isEmpty()) return;

    fsModel->setRootPath(dir);
    fileTree->setRootIndex(fsModel->index(dir));
    statusBar()->showMessage("Folder: " + dir, 3000);
}

/* File Operations - Save File */

void MainWindow::saveFile()
{
    if (currentFilePath.isEmpty()) { saveFileAs(); return; }

    QFile file(currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream(&file) << editor->toPlainText();
    unsavedChanges = false;
    setWindowTitle("MarkEd — " + QFileInfo(currentFilePath).fileName());
    statusBar()->showMessage("Saved", 2000);
}

/* File Operations - Save File As */

void MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(this, "Save As", QDir::homePath(), "Markdown (*.md);;All files (*)");
    if (path.isEmpty()) return;

    currentFilePath = path;
    saveFile();
}

/* File Operations - Load File */

void MainWindow::loadFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    editor->setPlainText(QTextStream(&file).readAll());
    currentFilePath = path;
    unsavedChanges  = false;
    setWindowTitle("MarkEd — " + QFileInfo(path).fileName());
    statusBar()->showMessage("Opened: " + path, 3000);
}

void MainWindow::onFileDoubleClicked(const QModelIndex& index)
{
    if (!fsModel->isDir(index))
        loadFile(fsModel->filePath(index));
}

/* Toolbar Actions */

void MainWindow::wrapSelection(const QString& before, const QString& after)
{
    QTextCursor cursor = editor->textCursor();
    QString sel = cursor.selectedText();
    cursor.insertText(sel.isEmpty() ? before + "text" + after : before + sel + after);
    editor->setTextCursor(cursor);
}

void MainWindow::prependToLine(const QString& prefix)
{
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText(prefix);
    editor->setTextCursor(cursor);
}

void MainWindow::insertBold()
{
    wrapSelection("**", "**");
}

void MainWindow::insertItalic()
{
    wrapSelection("*", "*");
}

void MainWindow::insertStrikethrough()
{
    wrapSelection("~~", "~~");
}

void MainWindow::insertH1()
{
    prependToLine("# ");
}

void MainWindow::insertH2()
{
    prependToLine("## ");
}

void MainWindow::insertH3()
{
    prependToLine("### ");
}

void MainWindow::insertHR()
{
    editor->insertPlainText("\n---\n");
}

void MainWindow::insertLink()
{
    wrapSelection("[", "](url)");
}

void MainWindow::insertCodeBlock()
{
    QTextCursor cursor = editor->textCursor();
    cursor.insertText("```\n" + (cursor.selectedText().isEmpty() ? "" : cursor.selectedText()) + "\n```");
    editor->setTextCursor(cursor);
}

/* View Toggles */

void MainWindow::zoomIn()
{
    double z = preview->zoomFactor();
    if (z < 3.0) preview->setZoomFactor(z + 0.1);
}

void MainWindow::zoomOut()
{
    double z = preview->zoomFactor();
    if (z > 0.3) preview->setZoomFactor(z - 0.1);
}

void MainWindow::zoomReset()
{
    preview->setZoomFactor(1.0);
}

void MainWindow::syncScrollToPreview(int value)
{
    QScrollBar* sb = editor->verticalScrollBar();
    if (sb->maximum() == 0) return;

    double pct = (double)value / sb->maximum();
    // šalje se JavaScript komanda u preview koji skroluje na isti relativni položaj
    QString js = QString("window.scrollTo(0, %1 * (document.body.scrollHeight - window.innerHeight));").arg(pct);
    preview->page()->runJavaScript(js);
}

void MainWindow::exportPdf()
{
    QString path = QFileDialog::getSaveFileName(this, "Export as PDF",
        QDir::homePath(), "PDF files (*.pdf)");
    if (path.isEmpty()) return;

    // printToPdf je na QWebEnginePage — renderuje trenutni HTML preview direktno u PDF
    connect(preview->page(), &QWebEnginePage::pdfPrintingFinished,
            this, [this](const QString& filePath, bool success) {
        disconnect(preview->page(), &QWebEnginePage::pdfPrintingFinished, this, nullptr);
        if (success)
            statusBar()->showMessage("Exported: " + filePath, 4000);
        else
            statusBar()->showMessage("Export failed.", 4000);
    });

    preview->page()->printToPdf(path);
}

void MainWindow::toggleFileTree()
{
    bool visible = sidePanel->isVisible();
    sidePanel->setVisible(!visible);
    actionToggleTree->setToolTip(visible ? "Show file tree (Ctrl+\\)" : "Hide file tree (Ctrl+\\)");
}

void MainWindow::togglePreview()
{
    previewPanel->setVisible(!previewPanel->isVisible());
}

/* Status Bar Helpers */

void MainWindow::onCursorPositionChanged()
{
    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col  = cursor.columnNumber() + 1;
    lblCursorPos->setText(QString("Ln %1  Col %2").arg(line).arg(col));
}

void MainWindow::updateWordCount()
{
    QString text = editor->toPlainText().trimmed();
    int words = text.isEmpty() ? 0 : text.split(QRegExp("\\s+"), QString::SkipEmptyParts).size();
    lblWordCount->setText(QString("Words: %1").arg(words));
}

void MainWindow::markUnsaved()
{
    if (!unsavedChanges)
    {
        unsavedChanges = true;
        QString title = windowTitle();
        if (!title.endsWith(" ●"))
            setWindowTitle(title + " ●");
    }
}
