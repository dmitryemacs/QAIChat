// codeblockwidget.cpp
#include "codeblockwidget.h"
#include <QTextOption>
#include <QDebug>

CodeBlockWidget::CodeBlockWidget(const QString &language, const QString &codeContent, QWidget *parent)
    : QWidget(parent)
{
    codeTextEdit = new QTextEdit(this);
    codeTextEdit->setReadOnly(true);
    codeTextEdit->setPlainText(codeContent);

    // Устанавливаем опцию переноса слов
    QTextOption textOption = codeTextEdit->document()->defaultTextOption();
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    codeTextEdit->document()->setDefaultTextOption(textOption);

    CodeHighlighter::BlockState languageBlockState = getBlockStateFromLanguage(language);

    highlighter = new CodeHighlighter(codeTextEdit->document(), languageBlockState);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(codeTextEdit);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

CodeBlockWidget::~CodeBlockWidget()
{

}

// Вспомогательная функция для преобразования строки языка в BlockState
CodeHighlighter::BlockState CodeBlockWidget::getBlockStateFromLanguage(const QString &language)
{
    if (language.compare("python", Qt::CaseInsensitive) == 0) {
        return CodeHighlighter::PythonCode;
    } else if (language.compare("cpp", Qt::CaseInsensitive) == 0 || language.compare("c++", Qt::CaseInsensitive) == 0) {
        return CodeHighlighter::CppCode;
    } else if (language.compare("json", Qt::CaseInsensitive) == 0) {
        return CodeHighlighter::JsonCode;
    } else {
        return CodeHighlighter::UnknownCode;
    }
}
