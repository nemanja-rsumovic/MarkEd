#include "highlighter.h"

MarkdownHighlighter::MarkdownHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    // Headings -> ljubicasta, bold
    QTextCharFormat headingFormat;
    headingFormat.setForeground(QColor("#d2a8ff"));
    headingFormat.setFontWeight(QFont::Bold);
    rules.append({ QRegExp("^#{1,6} .*"), headingFormat });

    // Blockquote -> zelena, italic
    QTextCharFormat blockquoteFormat;
    blockquoteFormat.setForeground(QColor("#3fb950"));
    blockquoteFormat.setFontItalic(true);
    rules.append({ QRegExp("^>.*"), blockquoteFormat });

    // Horizontal rule
    QTextCharFormat hrFormat;
    hrFormat.setForeground(QColor("#484f58"));
    rules.append({ QRegExp("^(---+|\\*\\*\\*+|___+)\\s*$"), hrFormat });

    // Bold -> zuta (bold mora pre italic!)
    QTextCharFormat boldFormat;
    boldFormat.setForeground(QColor("#e3b341"));
    boldFormat.setFontWeight(QFont::Bold);
    rules.append({ QRegExp("\\*\\*[^*]+\\*\\*"), boldFormat });
    rules.append({ QRegExp("__[^_]+__"),          boldFormat });

    // Italic -> svetlo plava, italic
    QTextCharFormat italicFormat;
    italicFormat.setForeground(QColor("#79c0ff"));
    italicFormat.setFontItalic(true);
    rules.append({ QRegExp("\\*[^*]+\\*"), italicFormat });
    rules.append({ QRegExp("_[^_]+_"),     italicFormat });

    // Strikethrough -> siva, precrtano
    QTextCharFormat strikeFormat;
    strikeFormat.setForeground(QColor("#7d8590"));
    strikeFormat.setFontStrikeOut(true);
    rules.append({ QRegExp("~~[^~]+~~"), strikeFormat });

    // Inline code -> narandzasta
    QTextCharFormat inlineCodeFormat;
    inlineCodeFormat.setForeground(QColor("#f0883e"));
    inlineCodeFormat.setFontFamily("Consolas");
    rules.append({ QRegExp("`[^`]+`"), inlineCodeFormat });

    // Link -> [text](url) — tirkizna
    QTextCharFormat linkFormat;
    linkFormat.setForeground(QColor("#39d353"));
    rules.append({ QRegExp("\\[([^\\]]+)\\]\\([^\\)]+\\)"), linkFormat });

    // Image -> ![alt](url)
    rules.append({ QRegExp("!\\[([^\\]]+)\\]\\([^\\)]+\\)"), linkFormat });

    // Code fence linija (``` ili ~~~)
    QTextCharFormat codeFenceFormat;
    codeFenceFormat.setForeground(QColor("#484f58"));
    rules.append({ QRegExp("^(`{3,}|~{3,}).*"), codeFenceFormat });

    // Format za sadrzaj unutar code bloka
    codeBlockFormat.setForeground(QColor("#8b949e"));
    codeBlockFormat.setFontFamily("Consolas");
}

void MarkdownHighlighter::highlightBlock(const QString& text)
{
    // Višelinijski code blokovi (state 1 = unutar bloka)
    int prevState = previousBlockState();

    if (text.contains(QRegExp("^(`{3,}|~{3,})"))) {
        setFormat(0, text.length(), codeBlockFormat);
        setCurrentBlockState(prevState == 1 ? 0 : 1);
        return;
    }

    if (prevState == 1) {
        setFormat(0, text.length(), codeBlockFormat);
        setCurrentBlockState(1);
        return;
    }

    setCurrentBlockState(0);

    // Primena pravila redom
    for (const Rule& rule : rules) {
        QRegExp rx = rule.pattern;
        int idx = rx.indexIn(text);
        while (idx >= 0) {
            int len = rx.matchedLength();
            setFormat(idx, len, rule.format);
            idx = rx.indexIn(text, idx + len);
        }
    }
}
