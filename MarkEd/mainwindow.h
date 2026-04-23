#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QWebEngineView>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSplitter>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QAction>
#include "highlighter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updatePreview();
    void onCursorPositionChanged();
    void onFileDoubleClicked(const QModelIndex& index);

    void newFile();
    void openFolder();
    void saveFile();
    void saveFileAs();

    void insertBold();
    void insertItalic();
    void insertStrikethrough();
    void insertH1();
    void insertH2();
    void insertH3();
    void insertLink();
    void insertCodeBlock();
    void insertHR();

    void toggleFileTree();
    void togglePreview();
    void exportPdf();
    void syncScrollToPreview(int value);
    void zoomIn();
    void zoomOut();
    void zoomReset();

private:
    Ui::MainWindow *ui;

    QSplitter*        mainSplitter;
    QWidget*          sidePanel;
    QWidget*          editorPanel;
    QWidget*          previewPanel;
    QPlainTextEdit*   editor;
    QWebEngineView*   preview;
    QTreeView*        fileTree;
    QFileSystemModel* fsModel;

    QAction*              actionToggleTree;
    MarkdownHighlighter*  highlighter;

    QTimer*  previewTimer;
    QLabel*  lblCursorPos;
    QLabel*  lblWordCount;

    QString currentFilePath;
    bool    unsavedChanges = false;

    void setupLayout();
    void setupMenuBar();
    void setupToolBar();
    void setupFileTree();
    void setupEditor();
    void setupPreview();
    void setupStatusBar();
    void applyStylesheet();

    void loadFile(const QString& path);
    void wrapSelection(const QString& before, const QString& after);
    void prependToLine(const QString& prefix);
    QString buildHtml(const QString& body) const;
    void updateWordCount();
    void markUnsaved();
};

#endif // MAINWINDOW_H
