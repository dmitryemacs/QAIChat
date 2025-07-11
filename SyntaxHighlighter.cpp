#include "SyntaxHighlighter.h"
#include <QDebug>

CodeHighlighter::CodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();      // Настраиваем цвета и стили
    setupPythonRules();  // Определяем правила для Python
    setupCppRules();     // Определяем правила для C++
    setupJsonRules();    // Определяем правила для JSON

    // Регулярные выражения для определения начала и конца блоков кода
    // ^```(python|cpp|json)(?:\\s*(.*))?$
    // (?:\\s*(.*))? - это необязательная группа, которая захватывает опциональные пробелы и весь оставшийся текст на строке.
    // (.*) - это вторая захватывающая группа для Cpp (первая для языка), и первая для Python/JSON/Unknown.
    codeBlockStartPython = QRegularExpression("^```python(?:\\s*(.*))?$");
    codeBlockStartCpp = QRegularExpression("^```(cpp|c\\+\\+)(?:\\s*(.*))?$"); // Поддержка cpp и c++
    codeBlockStartJson = QRegularExpression("^```json(?:\\s*(.*))?$");
    codeBlockStartUnknown = QRegularExpression("^```(?!\\S)(?:\\s*(.*))?$"); // ``` в начале строки, не за которым следует не-пробельный символ
    codeBlockEnd = QRegularExpression("^```\\s*$"); // Закрывающий маркер по-прежнему ожидается на отдельной строке
}

// Настройка форматов (цветов и стилей)
void CodeHighlighter::setupFormats()
{
    // Цвета вдохновлены темой Visual Studio Code "Dark+"
    keywordFormat.setForeground(QColor("#569cd6")); // Синий
    keywordFormat.setFontWeight(QFont::Bold);

    stringFormat.setForeground(QColor("#ce9178")); // Оранжевый
    stringFormat.setFontWeight(QFont::Normal);

    commentFormat.setForeground(QColor("#6a9955")); // Зеленый
    commentFormat.setFontItalic(true);

    functionFormat.setForeground(QColor("#dcdcaa")); // Желтый
    functionFormat.setFontWeight(QFont::Normal);

    numberFormat.setForeground(QColor("#b5cea8")); // Светло-зеленый
    numberFormat.setFontWeight(QFont::Normal);

    operatorFormat.setForeground(QColor("#d4d4d4")); // Белый/Серый
    operatorFormat.setFontWeight(QFont::Normal);

    braceFormat.setForeground(QColor("#d4d4d4")); // Белый/Серый
    braceFormat.setFontWeight(QFont::Normal);

    classFormat.setForeground(QColor("#4ec9b0")); // Бирюзовый
    classFormat.setFontWeight(QFont::Bold);
}

// Правила для Python
void CodeHighlighter::setupPythonRules()
{
    QList<HighlightingRule> pythonRules;

    // Ключевые слова
    QStringList pythonKeywordPatterns;
    pythonKeywordPatterns << "\\b(?:and|as|assert|async|await|break|class|continue|def|del|elif|else|except|finally|for|from|global|if|import|in|is|lambda|nonlocal|not|or|pass|raise|return|try|while|with|yield)\\b";
    for (const QString &pattern : pythonKeywordPatterns) {
        pythonRules.append({QRegularExpression(pattern), keywordFormat});
    }

    // Операторы
    QStringList pythonOperatorPatterns;
    pythonOperatorPatterns << "=|\\+|\\-|\\*|/|%|\\*\\*|//|&&|\\|\\||!|==|!=|<|>|<=|>=|&|\\||\\^|~|<<|>>";
    for (const QString &pattern : pythonOperatorPatterns) {
        pythonRules.append({QRegularExpression(pattern), operatorFormat});
    }

    // Скобки
    QStringList pythonBracePatterns;
    pythonBracePatterns << "\\{|\\}|\\(|\\)|[|]";
    for (const QString &pattern : pythonBracePatterns) {
        pythonRules.append({QRegularExpression(pattern), braceFormat});
    }

    // Строки (одиночные и двойные кавычки)
    pythonRules.append({QRegularExpression("(\"|').*?(\\1)"), stringFormat});

    // Комментарии
    pythonRules.append({QRegularExpression("#[^\n]*"), commentFormat});

    // Функции (имя перед открывающейся скобкой)
    pythonRules.append({QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"), functionFormat});

    // Числа
    pythonRules.append({QRegularExpression("\\b\\d+(\\.\\d*)?([eE][+\\-]?\\d+)?\\b"), numberFormat});

    highlightingRules.insert(PythonCode, pythonRules);
}

// Правила для C++
void CodeHighlighter::setupCppRules()
{
    QList<HighlightingRule> cppRules;

    // Ключевые слова C++
    QStringList cppKeywordPatterns;
    cppKeywordPatterns << "\\b(?:alignas|alignof|and|and_eq|asm|auto|bitand|bitor|bool|break|case|catch|char|char8_t|char16_t|char32_t|class|compl|concept|const|consteval|constexpr|constinit|const_cast|continue|co_await|co_return|co_yield|decltype|default|delete|do|double|dynamic_cast|else|enum|explicit|export|extern|false|float|for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|not|not_eq|nullptr|operator|or|or_eq|private|protected|public|reflexpr|register|reinterpret_cast|requires|return|short|signed|sizeof|static|static_assert|static_cast|struct|switch|synchronized|template|this|thread_local|throw|true|try|typedef|typeid|typename|union|unsigned|using|virtual|void|volatile|wchar_t|while|xor|xor_eq)\\b";
    for (const QString &pattern : cppKeywordPatterns) {
        cppRules.append({QRegularExpression(pattern), keywordFormat});
    }

    // Операторы
    QStringList cppOperatorPatterns;
    cppOperatorPatterns << "=|\\+|\\-|\\*|/|%|&&|\\|\\||!|==|!=|<|>|<=|>=|&|\\||\\^|~|<<|>>|\\+=|\\-=|\\*=|/=|%=|&=|\\|=|^=|<<=|>>=";
    for (const QString &pattern : cppOperatorPatterns) {
        cppRules.append({QRegularExpression(pattern), operatorFormat});
    }

    // Скобки
    QStringList cppBracePatterns;
    cppBracePatterns << "\\{|\\}|\\(|\\)|[|]";
    for (const QString &pattern : cppBracePatterns) {
        cppRules.append({QRegularExpression(pattern), braceFormat});
    }

    // Строки (одиночные и двойные кавычки)
    cppRules.append({QRegularExpression("(\"|').*?(\\1)"), stringFormat});

    // Комментарии (однострочные)
    cppRules.append({QRegularExpression("//[^\n]*"), commentFormat});

    // Функции
    cppRules.append({QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"), functionFormat});

    // Числа
    cppRules.append({QRegularExpression("\\b\\d+(\\.\\d*)?([eE][+\\-]?\\d+)?\\b"), numberFormat});

    // Классы (начинаются с заглавной буквы или Qt-классы)
    cppRules.append({QRegularExpression("\\bQ[A-Za-z]+\\b"), classFormat}); // Например, QWidget, QString
    cppRules.append({QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*\\b"), classFormat}); // Общие имена классов

    highlightingRules.insert(CppCode, cppRules);
}

// Правила для JSON
void CodeHighlighter::setupJsonRules()
{
    QList<HighlightingRule> jsonRules;

    // Ключи (строки перед двоеточием)
    jsonRules.append({QRegularExpression("\"[^\"]+\"(?=\\s*:)"), keywordFormat});

    // Строковые значения
    jsonRules.append({QRegularExpression("(?<=\":\\s*)\"[^\"]*\""), stringFormat});

    // Числа
    jsonRules.append({QRegularExpression("\\b-?\\d+(\\.\\d*)?([eE][+\\-]?\\d+)?\\b"), numberFormat});

    // Булевы значения и null
    jsonRules.append({QRegularExpression("\\b(?:true|false|null)\\b"), keywordFormat});

    // Скобки и квадратные скобки
    jsonRules.append({QRegularExpression("[\\{\\}\\[\\]]"), braceFormat});

    // Двоеточия и запятые
    jsonRules.append({QRegularExpression("[:,]"), operatorFormat});

    highlightingRules.insert(JsonCode, jsonRules);
}

// Основной метод подсветки каждого блока текста
void CodeHighlighter::highlightBlock(const QString &text)
{
    // 1. Определяем языковое состояние для текущего блока.
    //    Оно начинается как состояние предыдущего блока.
    BlockState currentLanguageState = static_cast<BlockState>(previousBlockState());
    int codeContentOffset = 0; // Смещение, с которого начинать применять правила подсветки языка

    // 2. Проверяем на конец блока кода (закрывающий маркер ```)
    QRegularExpressionMatch matchEnd = codeBlockEnd.match(text);
    if (matchEnd.hasMatch()) {
        setFormat(matchEnd.capturedStart(), matchEnd.capturedLength(), commentFormat); // Подсвечиваем сам маркер
        setCurrentBlockState(PlainText); // Следующий блок будет обычным текстом
        return; // Эта строка - просто закрывающий маркер, дальнейшая подсветка кода не нужна.
    }

    // 3. Проверяем на начало блока кода (открывающий маркер ```lang)
    QRegularExpressionMatch matchStartPython = codeBlockStartPython.match(text);
    QRegularExpressionMatch matchStartCpp = codeBlockStartCpp.match(text);
    QRegularExpressionMatch matchStartJson = codeBlockStartJson.match(text);
    QRegularExpressionMatch matchStartUnknown = codeBlockStartUnknown.match(text);

    if (matchStartPython.hasMatch()) {
        currentLanguageState = PythonCode;
        setFormat(matchStartPython.capturedStart(), matchStartPython.capturedLength(), commentFormat);
        // Если есть содержимое после маркера, устанавливаем смещение
        if (matchStartPython.lastCapturedIndex() >= 1 && !matchStartPython.captured(1).isEmpty()) {
            codeContentOffset = matchStartPython.capturedStart(1); // Начальный индекс захваченного содержимого
        } else {
            codeContentOffset = text.length(); // Нет содержимого на этой строке, фактически пропускаем подсветку
        }
    } else if (matchStartCpp.hasMatch()) {
        currentLanguageState = CppCode;
        setFormat(matchStartCpp.capturedStart(), matchStartCpp.capturedLength(), commentFormat);
        // Для Cpp, захваченная группа 1 - это язык, группа 2 - это содержимое
        if (matchStartCpp.lastCapturedIndex() >= 2 && !matchStartCpp.captured(2).isEmpty()) {
            codeContentOffset = matchStartCpp.capturedStart(2);
        } else {
            codeContentOffset = text.length();
        }
    } else if (matchStartJson.hasMatch()) {
        currentLanguageState = JsonCode;
        setFormat(matchStartJson.capturedStart(), matchStartJson.capturedLength(), commentFormat);
        if (matchStartJson.lastCapturedIndex() >= 1 && !matchStartJson.captured(1).isEmpty()) {
            codeContentOffset = matchStartJson.capturedStart(1);
        } else {
            codeContentOffset = text.length();
        }
    } else if (matchStartUnknown.hasMatch()) {
        currentLanguageState = UnknownCode;
        setFormat(matchStartUnknown.capturedStart(), matchStartUnknown.capturedLength(), commentFormat);
        if (matchStartUnknown.lastCapturedIndex() >= 1 && !matchStartUnknown.captured(1).isEmpty()) {
            codeContentOffset = matchStartUnknown.capturedStart(1);
        } else {
            codeContentOffset = text.length();
        }
    }
    // Если ни одно из вышеперечисленных, currentLanguageState остается тем, чем было previousBlockState.
    // codeContentOffset остается 0, что означает применение правил с начала строки.

    // 4. Применяем правила подсветки на основе определенного языкового состояния.
    if (currentLanguageState != PlainText) {
        QList<HighlightingRule> rulesToApply = highlightingRules.value(currentLanguageState);
        for (const HighlightingRule &rule : rulesToApply) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                // Применяем формат только если совпадение начинается на или после вычисленного смещения.
                if (match.capturedStart() >= codeContentOffset) {
                    setFormat(match.capturedStart(), match.capturedLength(), rule.format);
                }
            }
        }
    }

    // 5. Устанавливаем состояние для следующего блока.
    setCurrentBlockState(currentLanguageState);
}
