#include "mainwindow.h"
#include <QLabel>
#include <QTextOption>
#include <QListView>
#include <QDebug>
#include <QMenuBar> // Для использования QMenuBar
#include <QApplication> // Для установки глобального стиля QApplication::setStyleSheet()

// system info
#include <QCoreApplication>
#include <QDebug>
#include <QSysInfo>
#include <QProcess>
#include <QThread>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    QString systemInfo = getSystemInfo();

    // Настройка UI
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setAcceptRichText(true);

    // chatDisplay->append("<b style='color: #9b59b6;'>Информация о системе:</b><pre>" + systemInfo + "</pre>");
    this->systemInfoContext = systemInfo;

    // Шрифты и размер будут установлены методом loadSettings()

    QTextOption textOption = chatDisplay->document()->defaultTextOption();

    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere); // Переносить в любом месте
    chatDisplay->document()->setDefaultTextOption(textOption);
    mainLayout->addWidget(chatDisplay);

    codeHighlighter = new CodeHighlighter(chatDisplay->document());

    // Layout для выбора модели
    QHBoxLayout *modelLayout = new QHBoxLayout();
    modelSelector = new QComboBox(this);
    modelLayout->addWidget(new QLabel("Выберите модель:"));
    modelLayout->addWidget(modelSelector);
    mainLayout->addLayout(modelLayout);

    // Заполнение доступных моделей с описаниями
    availableModels["GPT-4o (OpenAI)"] = {"openai/gpt-4o", "OpenAI's most advanced, multimodal model. Great for text, vision, and audio tasks. High quality."};
    availableModels["Mistral 7B Instruct (Free)"] = {"mistralai/mistral-7b-instruct:free", "A powerful, fast, and free open-source model from Mistral AI. Good for general chat."};
    availableModels["Llama 3 8B Instruct (Meta)"] = {"meta-llama/llama-3-8b-instruct", "Meta's Llama 3 8B Instruct model. Good performance for its size, open-source."};
    availableModels["Google Gemini Flash 1.5"] = {"google/gemini-flash-1.5", "Google's fast and efficient Gemini 1.5 Flash model. Optimized for speed and scale."};
    availableModels["Nous Hermes 2 Mixtral 8x7B (NousResearch)"] = {"nousresearch/nous-hermes-2-mixtral-8x7b-dpo", "A fine-tuned Mixtral 8x7B model, known for its creative and detailed responses."};

    availableModels["DeepSeek: DeepSeek V3"] = {"deepseek/deepseek-chat-v3-0324:free", "Google's lightweight, open-source model. Good for quick responses and smaller tasks."};
    availableModels["DeepSeek: R1"] = {"deepseek/deepseek-r1:free", "Google's lightweight, open-source model. Good for quick responses and smaller tasks."};
    availableModels["Qwen: QwQ 32B"] = {"qwen/qwq-32b:free", "Google's lightweight, open-source model. Good for quick responses and smaller tasks."};
    availableModels["Mistral: Mistral Nemo"] = {"mistralai/mistral-nemo:free", "Google's lightweight, open-source model. Good for quick responses and smaller tasks."};
    availableModels["Qwen3 235B"] = {"qwen/qwen3-235b-a22b:free", "Google's lightweight, open-source model. Good for quick responses and smaller tasks."};
    availableModels["Mistral Small"] = {"mistralai/mistral-small-3.2-24b-instruct:free", "Google's lightweight, open-source model. Good for quick responses and smaller tasks."};
    availableModels["Google gemma3"] = {"google/gemma-3-27b-it:free", "Google's lightweight, open-source model. Good for quick responses and smaller tasks."};


    for (const QString &displayName : availableModels.keys()) {
        modelSelector->addItem(displayName);
    }
    int defaultIndex = modelSelector->findText("DeepSeek: R1");
    if (defaultIndex != -1) {
        modelSelector->setCurrentIndex(defaultIndex);
    }

    // --- Настройка для показа информации при наведении ---
    hoverTimer = new QTimer(this);
    hoverTimer->setSingleShot(true);
    hoverTimer->setInterval(1000);

    connect(hoverTimer, &QTimer::timeout, this, &MainWindow::showModelInfo);

    QListView *listView = qobject_cast<QListView*>(modelSelector->view());
    if (listView) {
        listView->installEventFilter(this);
        listView->setMouseTracking(true);
        qDebug() << "Event filter installed on QComboBox's QListView.";
    } else {
        qDebug() << "ERROR: QComboBox view is not a QListView or is null!";
    }

    QHBoxLayout *inputLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Введите ваше сообщение...");
    messageInput->setFixedHeight(40);
    inputLayout->addWidget(messageInput);

    sendButton = new QPushButton("Отправить", this);

    inputLayout->addWidget(sendButton);

    mainLayout->addLayout(inputLayout);

    networkManager = new QNetworkAccessManager(this);

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    // --- Добавляем пункт меню "Настройки" ---
    QMenu *fileMenu = menuBar()->addMenu("Файл"); // Создаем меню "Файл"
    settingsAction = new QAction("Настройки...", this); // Создаем действие "Настройки..."
    fileMenu->addAction(settingsAction); // Добавляем действие в меню
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings); // Подключаем слот
    // --- Конец пункта меню "Настройки" ---

    setWindowTitle("AI QChat");
    resize(1280, 600);

    // Загружаем и применяем настройки при запуске
    loadSettings();
}

MainWindow::~MainWindow()
{
    // Все дочерние объекты (включая hoverTimer) удаляются автоматически, т.к. у них есть родитель.
}

void MainWindow::sendMessage()
{
    QString userMessage = messageInput->text().trimmed();
    if (userMessage.isEmpty()) {
        return;
    }

    QString selectedModelDisplayName = modelSelector->currentText();
    ModelData selectedModelData = availableModels.value(selectedModelDisplayName);
    QString modelId = selectedModelData.id;

    if (modelId.isEmpty()) {
        chatDisplay->append("<b style='color: red;'>Ошибка:</b> Выбранная модель не найдена в списке.");
        qDebug() << "Error: Selected model display name not found in availableModels map:" << selectedModelDisplayName;
        return;
    }

    chatDisplay->append("<b style='color: #3498db;'>Вы:</b> " + userMessage); // Синий цвет
    messageInput->clear();

    QJsonObject requestBody;
    requestBody["model"] = modelId;

    QJsonArray messagesArray;

    // --- КОНТЕКСТ О СИСТЕМЕ ---
    QJsonObject systemMessageObj;
    systemMessageObj["role"] = "system";
    systemMessageObj["content"] = QString("Вы работаете на следующей системе:\n%1\n"
                                          "Используйте эту информацию, если вопрос касается операционной системы, железа или ресурсов.")
                                      .arg(systemInfoContext);
    messagesArray.append(systemMessageObj);

    // --- ПОЛЬЗОВАТЕЛЬСКИЙ ЗАПРОС ---
    QJsonObject userMessageObj;
    userMessageObj["role"] = "user";
    userMessageObj["content"] = userMessage;
    messagesArray.append(userMessageObj);

    requestBody["messages"] = messagesArray;

    QJsonDocument doc(requestBody);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    QNetworkRequest request{QUrl(OPENROUTER_API_URL)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + OPENROUTER_API_KEY).toUtf8());
    request.setRawHeader("HTTP-Referer", "https://your-app-domain.com "); // <-- ЗАМЕНИТЕ ЭТО
    request.setRawHeader("X-Title", "Your App Name");

    networkManager->post(request, postData);

    chatDisplay->append("<i>AI печатает...</i>");
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    // Удаляем индикатор "AI печатает..." из чата
    QString htmlContent = chatDisplay->toHtml();
    int lastIndex = htmlContent.lastIndexOf("<i>AI печатает...</i>");
    if (lastIndex != -1) {
        htmlContent.remove(lastIndex, QString("<i>AI печатает...</i>").length());
    }
    chatDisplay->setHtml(htmlContent);

    // Обработка ответа от сервера
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject()) {
            QJsonObject jsonObject = jsonDoc.object();

            // Проверка наличия поля "choices"
            if (jsonObject.contains("choices") && jsonObject["choices"].isArray()) {
                QJsonArray choices = jsonObject["choices"].toArray();

                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices.first().toObject();

                    // Проверка наличия поля "message"
                    if (firstChoice.contains("message") && firstChoice["message"].isObject()) {
                        QJsonObject message = firstChoice["message"].toObject();

                        // Получение и вывод ответа AI
                        if (message.contains("content") && message["content"].isString()) {
                            QString aiResponse = message["content"].toString();
                            chatDisplay->append("<b style='color: #27ae60;'>AI:</b> " + aiResponse); // Зеленый цвет
                        } else {
                            chatDisplay->append("<b style='color: red;'>Ошибка:</b> Не найден 'content' в сообщении AI.");
                            qDebug() << "JSON Error: 'content' not found in AI message.";
                        }
                    } else {
                        chatDisplay->append("<b style='color: red;'>Ошибка:</b> Не найден 'message' в выборе AI.");
                        qDebug() << "JSON Error: 'message' not found in AI choice.";
                    }
                } else {
                    chatDisplay->append("<b style='color: red;'>Ошибка:</b> Выбор AI пуст.");
                    qDebug() << "JSON Error: AI choices array is empty.";
                }
            } else {
                chatDisplay->append("<b style='color: red;'>Ошибка:</b> Не найдены 'choices' в ответе API.");
                qDebug() << "JSON Error: 'choices' not found in API response.";
            }
        } else {
            chatDisplay->append("<b style='color: red;'>Ошибка:</b> Ответ API не является JSON-объектом.");
            qDebug() << "JSON Error: API response is not a JSON object." << responseData;
        }
    } else {
        // Обработка ошибок сети
        chatDisplay->append(QString("<b style='color: red;'>Ошибка сети:</b> %1").arg(reply->errorString()));
        qDebug() << "Network Error:" << reply->errorString();
        qDebug() << "Response Data:" << reply->readAll();
    }

    reply->deleteLater();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == modelSelector->view()) {
        if (event->type() == QEvent::HoverMove) {
            QHoverEvent *hoverEvent = static_cast<QHoverEvent*>(event);
            QModelIndex currentIndex = modelSelector->view()->indexAt(hoverEvent->pos());
            if (currentIndex.isValid() && currentIndex != lastHoveredIndex) {
                lastHoveredIndex = currentIndex;
                hoverTimer->stop();
                hoverTimer->start();
            } else if (!currentIndex.isValid()) {
                hoverTimer->stop();
                lastHoveredIndex = QModelIndex();
                QToolTip::hideText();
            }
        } else if (event->type() == QEvent::Leave) {
            hoverTimer->stop();
            lastHoveredIndex = QModelIndex();
            QToolTip::hideText();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::showModelInfo()
{
    if (lastHoveredIndex.isValid()) {
        QString displayName = modelSelector->model()->data(lastHoveredIndex, Qt::DisplayRole).toString();
        ModelData modelData = availableModels.value(displayName);

        if (!modelData.description.isEmpty()) {
            QToolTip::showText(QCursor::pos(), modelData.description, modelSelector->view());
        }
    }
}

// Слот для открытия диалога настроек
void MainWindow::openSettings()
{
    // Создаем диалог, передавая текущие настройки
    SettingsDialog settingsDialog(currentChatFont, currentChatFontSize, currentTheme, this);
    // Если пользователь нажал OK
    if (settingsDialog.exec() == QDialog::Accepted) {
        // Получаем выбранные настройки
        currentChatFont = settingsDialog.selectedFont();
        currentChatFontSize = settingsDialog.selectedFontSize();
        currentTheme = settingsDialog.selectedTheme();

        // Применяем новый шрифт к chatDisplay
        QFont newFont = currentChatFont;
        newFont.setPointSize(currentChatFontSize);
        chatDisplay->setFont(newFont);

        // Применяем новую тему
        applyTheme(currentTheme);

        // Сохраняем настройки для следующего запуска
        saveSettings();
    }
}

// Метод для загрузки настроек из QSettings
void MainWindow::loadSettings()
{
    // Используйте уникальные имя организации и приложения
    QSettings settings("YourOrg", "OpenRouterAIChat"); // ВАЖНО: ЗАМЕНИТЕ "YourOrg" и "OpenRouterAIChat"

    // Загружаем шрифт (по умолчанию Consolas, размер 10)
    currentChatFont = QFont(settings.value("chatFont/family", "Consolas").toString());
    currentChatFontSize = settings.value("chatFont/size", 10).toInt();
    currentTheme = settings.value("theme", "light").toString(); // По умолчанию светлая тема

    // Применяем загруженный шрифт
    QFont initialFont = currentChatFont;
    initialFont.setPointSize(currentChatFontSize);
    chatDisplay->setFont(initialFont);

    // Применяем загруженную тему
    applyTheme(currentTheme);
}

// Метод для сохранения настроек в QSettings
void MainWindow::saveSettings()
{
    QSettings settings("YourOrg", "OpenRouterAIChat"); // ВАЖНО: ЗАМЕНИТЕ "YourOrg" и "OpenRouterAIChat"
    settings.setValue("chatFont/family", currentChatFont.family());
    settings.setValue("chatFont/size", currentChatFontSize);
    settings.setValue("theme", currentTheme);
}

//SYSTEM INFORMATION
QString MainWindow::getSystemInfo()
{
    QString info;

    info += "ОС: " + QSysInfo::prettyProductName() + "\n";
    info += "Архитектура процессора: " + QSysInfo::currentCpuArchitecture() + "\n";
    info += "Количество ядер процессора: " + QString::number(QThread::idealThreadCount()) + "\n";

    // Получаем информацию о памяти (только Linux)
    QProcess process;
    process.start("free", QStringList() << "-h");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    info += "Информация о памяти:\n" + output + "\n";

    // Получаем информацию о дисковом пространстве
    process.start("df", QStringList() << "-h");
    process.waitForFinished();
    output = process.readAllStandardOutput();
    info += "Информация о дисковом пространстве:\n" + output + "\n";

    return info;
}

// Метод для применения темы с помощью QSS
void MainWindow::applyTheme(const QString &theme)
{
    QString styleSheet = "";
    if (theme == "dark") {
        styleSheet = R"(
            QMainWindow {
                background-color: #2e2e2e; /* Темно-серый фон окна */
                color: #f0f0f0; /* Светлый текст */
            }
            QTextEdit {
                background-color: #1e1e1e; /* Очень темный фон для текстового поля */
                color: #d4d4d4; /* Светлый текст */
                border: 1px solid #3c3c3c; /* Темная рамка */
            }
            QLineEdit {
                background-color: #3c3c3c; /* Темный фон для поля ввода */
                color: #d4d4d4; /* Светлый текст */
                border: 1px solid #505050; /* Чуть светлее рамка */
            }
            QPushButton {
                background-color: #007acc; /* Синяя кнопка */
                color: #ffffff; /* Белый текст */
                border: none;
                padding: 5px 10px;
            }
            QPushButton:hover {
                background-color: #005f99; /* Темнее при наведении */
            }
            QComboBox {
                background-color: #3c3c3c;
                color: #d4d4d4;
                border: 1px solid #505050;
            }
            QComboBox::drop-down {
                border: 0px; /* Убираем рамку выпадающего списка */
            }
            /* QComboBox::down-arrow { image: url(down_arrow_dark.png); } */ /* Если хотите кастомную стрелку */
            QComboBox QAbstractItemView { /* Стили для элементов выпадающего списка */
                background-color: #3c3c3c;
                color: #d4d4d4;
                selection-background-color: #007acc;
            }
            QLabel {
                color: #f0f0f0; /* Цвет текста для QLabel */
            }
            QMenuBar {
                background-color: #2e2e2e;
                color: #f0f0f0;
            }
            QMenuBar::item {
                background-color: transparent;
                color: #f0f0f0;
            }
            QMenuBar::item:selected {
                background-color: #007acc;
            }
            QMenu {
                background-color: #2e2e2e;
                color: #f0f0f0;
                border: 1px solid #505050;
            }
            QMenu::item:selected {
                background-color: #007acc;
            }
            QDialog { /* Стили для диалогового окна настроек */
                background-color: #2e2e2e;
                color: #f0f0f0;
            }
            QSpinBox, QFontComboBox { /* Стили для полей в диалоге настроек */
                background-color: #3c3c3c;
                color: #d4d4d4;
                border: 1px solid #505050;
            }
            QDialogButtonBox QPushButton { /* Стили для кнопок в диалоге настроек */
                background-color: #007acc;
                color: #ffffff;
            }
            QDialogButtonBox QPushButton:hover {
                background-color: #005f99;
            }
        )";
    } else { // Светлая тема (по умолчанию)
        styleSheet = R"(
            QMainWindow {
                background-color: #f0f0f0;
                color: #333333;
            }
            QTextEdit {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #cccccc;
            }
            QLineEdit {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #cccccc;
            }
            QPushButton {
                background-color: #007bff;
                color: #ffffff;
                border: none;
                padding: 5px 10px;
            }
            QPushButton:hover {
                background-color: #0056b3;
            }
            QComboBox {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #cccccc;
            }
            QComboBox::drop-down {
                border: 0px;
            }
            /* QComboBox::down-arrow { image: url(down_arrow_light.png); } */ /* Если хотите кастомную стрелку */
            QComboBox QAbstractItemView {
                background-color: #ffffff;
                color: #333333;
                selection-background-color: #007bff;
            }
            QLabel {
                color: #333333;
            }
            QMenuBar {
                background-color: #e0e0e0;
                color: #333333;
            }
            QMenuBar::item {
                background-color: transparent;
                color: #333333;
            }
            QMenuBar::item:selected {
                background-color: #007bff;
                color: #ffffff;
            }
            QMenu {
                background-color: #e0e0e0;
                color: #333333;
                border: 1px solid #cccccc;
            }
            QMenu::item:selected {
                background-color: #007bff;
                color: #ffffff;
            }
            QDialog {
                background-color: #f0f0f0;
                color: #333333;
            }
            QSpinBox, QFontComboBox {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #cccccc;
            }
            QDialogButtonBox QPushButton {
                background-color: #007bff;
                color: #ffffff;
            }
            QDialogButtonBox QPushButton:hover {
                background-color: #0056b3;
            }
        )";
    }
    // Применяем таблицу стилей ко всему приложению
    qApp->setStyleSheet(styleSheet);
}
