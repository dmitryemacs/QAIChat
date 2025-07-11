#define SYNTAXHIGHLIGH_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QMap>
#include <QList>

// Класс для подсветки синтаксиса
class CodeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    // Перечисление для различных состояний блоков текста (языков)
    enum BlockState {
        PlainText = -1, // Обычный текст, без специфичной подсветки
        PythonCode = 0,
        CppCode = 1,
        JsonCode = 2,
        UnknownCode = 3 // Блок кода без указания языка или с неизвестным языком
    };

    CodeHighlighter(QTextDocument *parent = nullptr);

protected:
    // Основной метод для подсветки каждого блока текста
    void highlightBlock(const QString &text) override;

private:
    // Структура для определения правила подсветки: паттерн и формат
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    // Карта, хранящая правила подсветки для каждого состояния/языка
    QMap<BlockState, QList<HighlightingRule>> highlightingRules;

    // Форматы для различных элементов синтаксиса
    QTextCharFormat keywordFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat braceFormat;
    QTextCharFormat classFormat;

    // Методы для настройки форматов и правил для каждого языка
    void setupFormats();
    void setupPythonRules();
    void setupCppRules();
    void setupJsonRules();

    // Регулярные выражения для определения начала и конца блоков кода
    QRegularExpression codeBlockStartPython;
    QRegularExpression codeBlockStartCpp;
    QRegularExpression codeBlockStartJson;
    QRegularExpression codeBlockStartUnknown; // Для ``` без указания языка
    QRegularExpression codeBlockEnd;
};
