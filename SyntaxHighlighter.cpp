// SyntaxHighlighter.cpp
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

// НОВЫЙ КОНСТРУКТОР: для использования с фиксированным языком (например, в CodeBlockWidget)
CodeHighlighter::CodeHighlighter(QTextDocument *parent, BlockState fixedLanguageState)
    : QSyntaxHighlighter(parent), m_fixedLanguageState(fixedLanguageState)
{
    setupFormats();
    setupPythonRules();
    setupCppRules();
    setupJsonRules();
    // Для этого конструктора регулярные выражения маркеров блоков кода не нужны,
    // так как предполагается, что подсветчик применяется к одному блоку известного языка.
    // Поэтому инициализируем их пустыми.
    codeBlockStartPython = QRegularExpression();
    codeBlockStartCpp = QRegularExpression();
    codeBlockStartJson = QRegularExpression();
    codeBlockStartUnknown = QRegularExpression();
    codeBlockEnd = QRegularExpression();
}


// Настройка форматов (цвета и стили)
void CodeHighlighter::setupFormats()
{
    keywordFormat.setForeground(QColor("#569cd6")); // Голубой (ключевые слова)
    keywordFormat.setFontWeight(QFont::Bold);

    stringFormat.setForeground(QColor("#d69d85"));  // Оранжевый (строки)

    commentFormat.setForeground(QColor("#608b4e")); // Зеленый (комментарии)
    commentFormat.setFontItalic(true);

    functionFormat.setForeground(QColor("#dcdcaa")); // Желтый (функции)

    numberFormat.setForeground(QColor("#b5cea8"));  // Светло-зеленый (числа)

    operatorFormat.setForeground(QColor("#d4d4d4")); // Серый (операторы)

    braceFormat.setForeground(QColor("#d4d4d4")); // Серый (скобки)

    classFormat.setForeground(QColor("#4ec9b0")); // Бирюзовый (имена классов)
    classFormat.setFontWeight(QFont::Bold);
}

// Настройка правил подсветки для Python
void CodeHighlighter::setupPythonRules()
{
    QList<HighlightingRule> pythonRules;

    // Ключевые слова
    QStringList pythonKeywords;
    pythonKeywords << "\\band\\b" << "\\basb" << "\\bassert\\b" << "\\bbreak\\b" << "\\bclass\\b"
                   << "\\bcontinue\\b" << "\\bdef\\b" << "\\bdel\\b" << "\\belif\\b" << "\\belse\\b"
                   << "\\bexcept\\b" << "\\bfinally\\b" << "\\bfor\\b" << "\\bfrom\\b" << "\\bglobal\\b"
                   << "\\bif\\b" << "\\bimport\\b" << "\\bin\\b" << "\\bis\\b" << "\\blambda\\b"
                   << "\\bnonlocal\\b" << "\\bnot\\b" << "\\bor\\b" << "\\bpass\\b" << "\\braise\\b"
                   << "\\breturn\\b" << "\\btry\\b" << "\\bwhile\\b" << "\\bwith\\b" << "\\byield\\b";
    for (const QString &pattern : pythonKeywords) {
        pythonRules.append({QRegularExpression(pattern), keywordFormat});
    }

    // Встроенные функции и константы (немного другой цвет или стиль)
    QTextCharFormat builtinFormat;
    builtinFormat.setForeground(QColor("#569cd6")); // Голубой
    QStringList pythonBuiltins;
    pythonBuiltins << "\\bFalse\\b" << "\\bNone\\b" << "\\bTrue\\b" << "\\babs\\b" << "\\ball\\b"
                   << "\\bany\\b" << "\\bascii\\b" << "\\bbool\\b" << "\\bcallable\\b" << "\\bchr\\b"
                   << "\\bclassmethod\\b" << "\\bcomplex\\b" << "\\bdelattr\\b" << "\\bdict\\b"
                   << "\\bdir\\b" << "\\bdivmod\\b" << "\\benumerate\\b" << "\\beval\\b" << "\\bexec\\b"
                   << "\\bfilter\\b" << "\\bfloat\\b" << "\\bformat\\b" << "\\bfrozenset\\b"
                   << "\\bgetattr\\b" << "\\bhasattr\\b" << "\\bhash\\b" << "\\bhelp\\b" << "\\bid\\b"
                   << "\\binput\\b" << "\\bint\\b" << "\\bisinstance\\b" << "\\bisubclass\\b"
                   << "\\biter\\b" << "\\blen\\b" << "\\blist\\b" << "\\bmap\\b" << "\\bmax\\b"
                   << "\\bmin\\b" << "\\bnext\\b" << "\\bobject\\b" << "\\bopen\\b" << "\\bord\\b"
                   << "\\bpow\\b" << "\\bprint\\b" << "\\bproperty\\b" << "\\brange\\b" << "\\brepr\\b"
                   << "\\breversed\\b" << "\\bround\\b" << "\\bset\\b" << "\\bsetattr\\b" << "\\bslice\\b"
                   << "\\bsorted\\b" << "\\bstaticmethod\\b" << "\\bstr\\b" << "\\bsum\\b"
                   << "\\bsuper\\b" << "\\btuple\\b" << "\\btype\\b" << "\\bvars\\b" << "\\bzip\\b";
    for (const QString &pattern : pythonBuiltins) {
        pythonRules.append({QRegularExpression(pattern), builtinFormat});
    }


    // Строки (одиночные и двойные кавычки)
    pythonRules.append({QRegularExpression("['][^']*[']"), stringFormat});
    pythonRules.append({QRegularExpression("[\"][^\"]*[\"]"), stringFormat});

    // Комментарии
    pythonRules.append({QRegularExpression("#[^\n]*"), commentFormat});

    // Функции (простые)
    pythonRules.append({QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"), functionFormat});

    // Операторы
    pythonRules.append({QRegularExpression("[-+=*/%&|^~!<>=?:]"), operatorFormat});

    // Скобки
    pythonRules.append({QRegularExpression("[(){}\\[\\]]"), braceFormat});

    highlightingRules.insert(PythonCode, pythonRules);
}

// Настройка правил подсветки для C++
void CodeHighlighter::setupCppRules()
{
    QList<HighlightingRule> cppRules;

    // Ключевые слова C++
    QStringList cppKeywords;
    cppKeywords << "\\b(alignas|alignof|and|and_eq|asm|atomic_cancel|atomic_commit|atomic_noexcept|auto|bitand|bitor|bool|break|case|catch|char|char8_t|char16_t|char32_t|class|compl|concept|const|consteval|constexpr|constinit|const_cast|continue|co_await|co_return|co_yield|decltype|default|delete|do|double|dynamic_cast|else|enum|explicit|export|extern|false|float|for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|not|not_eq|nullptr|operator|or|or_eq|private|protected|public|reflexpr|register|reinterpret_cast|requires|return|short|signed|sizeof|static|static_assert|static_cast|struct|switch|synchronized|template|this|thread_local|throw|true|try|typedef|typeid|typename|union|unsigned|using|virtual|void|volatile|wchar_t|while|xor|xor_eq)\\b";
    for (const QString &pattern : cppKeywords) {
        cppRules.append({QRegularExpression(pattern), keywordFormat});
    }

    // Строки
    cppRules.append({QRegularExpression("\".*\""), stringFormat});
    cppRules.append({QRegularExpression("'.*'"), stringFormat});

    // Комментарии
    cppRules.append({QRegularExpression("//[^\n]*"), commentFormat});
    // Исправлено: используем правильный флаг для QRegularExpression
    cppRules.append({QRegularExpression("/\\*.*\\*/", QRegularExpression::DotMatchesEverythingOption), commentFormat});

    // Функции
    cppRules.append({QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"), functionFormat});

    // Числа
    cppRules.append({QRegularExpression("\\b[0-9]+\\b"), numberFormat}); // Целые
    cppRules.append({QRegularExpression("\\b[0-9]*\\.[0-9]+([eE][+-]?[0-9]+)?\\b"), numberFormat}); // С плавающей точкой

    // Операторы
    cppRules.append({QRegularExpression("[-+*/%=&|^~!<>=?:,;.]"), operatorFormat});

    // Скобки
    cppRules.append({QRegularExpression("[(){}\\[\\]]"), braceFormat});

    // Классы (простое определение: class Name)
    cppRules.append({QRegularExpression("\\b(class|struct)\\s+\\b([A-Za-z_][A-Za-z0-9_]*)\\b"), classFormat});


    highlightingRules.insert(CppCode, cppRules);
}

// Настройка правил подсветки для JSON
void CodeHighlighter::setupJsonRules()
{
    QList<HighlightingRule> jsonRules;

    // Ключи JSON (строки в двойных кавычках, за которыми следует двоеточие)
    QTextCharFormat jsonKeyFormat;
    jsonKeyFormat.setForeground(QColor("#9cdcfe")); // Светло-голубой
    jsonRules.append({QRegularExpression("\"([^\"]+)\"\\s*:"), jsonKeyFormat});

    // Строковые значения JSON
    jsonRules.append({QRegularExpression("(?<!:)\\s*\"([^\"]*)\""), stringFormat}); // Исключаем ключи

    // Числа JSON
    jsonRules.append({QRegularExpression("\\b-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b"), numberFormat});

    // Логические значения (true, false) и null
    QTextCharFormat jsonBooleanNullFormat;
    jsonBooleanNullFormat.setForeground(QColor("#569cd6")); // Голубой
    jsonRules.append({QRegularExpression("\\b(true|false|null)\\b"), jsonBooleanNullFormat});

    // Скобки и квадратные скобки
    jsonRules.append({QRegularExpression("[\\[\\]{}]"), braceFormat});

    // Запятые и двоеточия (операторы)
    jsonRules.append({QRegularExpression("[,:]"), operatorFormat});

    highlightingRules.insert(JsonCode, jsonRules);
}

// Основной метод для подсветки каждого блока текста
void CodeHighlighter::highlightBlock(const QString &text)
{
    // Если установлен фиксированный язык, просто применяем его правила ко всему блоку.
    if (m_fixedLanguageState != PlainText) {
        applyHighlightingRules(text, 0, m_fixedLanguageState);
        return;
    }

    BlockState lastBlockState = static_cast<BlockState>(previousBlockState());
    BlockState currentLanguageState = lastBlockState;
    int codeContentOffset = 0;

    // если в режиме PlainText, пытаемся найти начало блока кода.
    if (lastBlockState == PlainText) {
        QRegularExpressionMatch matchStartCpp = codeBlockStartCpp.match(text);
        QRegularExpressionMatch matchStartPython = codeBlockStartPython.match(text);
        QRegularExpressionMatch matchStartJson = codeBlockStartJson.match(text);
        QRegularExpressionMatch matchStartUnknown = codeBlockStartUnknown.match(text);

        if (matchStartCpp.hasMatch()) {
            currentLanguageState = CppCode;
            setFormat(matchStartCpp.capturedStart(), matchStartCpp.capturedLength(), commentFormat);
            // Для C++/C++ язык находится в первой группе захвата, содержимое - во второй (если есть)
            if (matchStartCpp.lastCapturedIndex() >= 2 && !matchStartCpp.captured(2).isEmpty()) {
                codeContentOffset = matchStartCpp.capturedStart(2);
            } else {
                codeContentOffset = matchStartCpp.capturedEnd();
            }
        } else if (matchStartPython.hasMatch()) {
            currentLanguageState = PythonCode;
            setFormat(matchStartPython.capturedStart(), matchStartPython.capturedLength(), commentFormat);
            if (matchStartPython.lastCapturedIndex() >= 1 && !matchStartPython.captured(1).isEmpty()) {
                codeContentOffset = matchStartPython.capturedStart(1);
            } else {
                codeContentOffset = matchStartPython.capturedEnd();
            }
        } else if (matchStartJson.hasMatch()) {
            currentLanguageState = JsonCode;
            setFormat(matchStartJson.capturedStart(), matchStartJson.capturedLength(), commentFormat);
            if (matchStartJson.lastCapturedIndex() >= 1 && !matchStartJson.captured(1).isEmpty()) {
                codeContentOffset = matchStartJson.capturedStart(1);
            } else {
                codeContentOffset = matchStartJson.capturedEnd();
            }
        } else if (matchStartUnknown.hasMatch()) {
            currentLanguageState = UnknownCode;
            setFormat(matchStartUnknown.capturedStart(), matchStartUnknown.capturedLength(), commentFormat);
            if (matchStartUnknown.lastCapturedIndex() >= 1 && !matchStartUnknown.captured(1).isEmpty()) {
                codeContentOffset = matchStartUnknown.capturedStart(1);
            } else {
                codeContentOffset = matchStartUnknown.capturedEnd();
            }
        }
    }
    // 3. Если мы уже внутри блока кода, проверяем на закрывающий маркер.
    else {
        QRegularExpressionMatch matchEnd = codeBlockEnd.match(text);
        if (matchEnd.hasMatch()) {
            // Применяем подсветку к части до закрывающего маркера
            applyHighlightingRules(text.left(matchEnd.capturedStart()), 0, currentLanguageState);
            // Форматируем сам закрывающий маркер как комментарий
            setFormat(matchEnd.capturedStart(), matchEnd.capturedLength(), commentFormat);
            currentLanguageState = PlainText;
        }
    }

    if (currentLanguageState != PlainText && currentLanguageState == lastBlockState) {
        applyHighlightingRules(text, 0, currentLanguageState);
    } else if (currentLanguageState != PlainText && currentLanguageState != lastBlockState) {
        // Это строка, которая содержит открывающий маркер. Применяем правила от смещения.
        applyHighlightingRules(text, codeContentOffset, currentLanguageState);
    }


    // 5. Устанавливаем состояние для следующего блока.
    setCurrentBlockState(currentLanguageState);
}


// Приватный метод для применения правил подсветки к тексту в зависимости от текущего состояния блока.
void CodeHighlighter::applyHighlightingRules(const QString &text, int offset, BlockState languageState)
{
    QList<HighlightingRule> rulesToApply = highlightingRules.value(languageState);
    for (const HighlightingRule &rule : rulesToApply) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            // Применяем формат только если совпадение начинается на или после вычисленного смещения.
            if (match.capturedStart() >= offset) {
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
}
