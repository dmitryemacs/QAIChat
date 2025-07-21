// codeblockwidget.h
#ifndef CODEBLOCKWIDGET_H
#define CODEBLOCKWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include "SyntaxHighlighter.h"

class CodeBlockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CodeBlockWidget(const QString &language, const QString &codeContent, QWidget *parent = nullptr);
    ~CodeBlockWidget();

private:
    QTextEdit *codeTextEdit;
    CodeHighlighter *highlighter;

    // для преобразования строки языка в BlockState
    CodeHighlighter::BlockState getBlockStateFromLanguage(const QString &language);
};

#endif // CODEBLOCKWIDGET_H
