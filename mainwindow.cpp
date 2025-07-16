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
#include <QDateTime> // Для системной информации
#include <QDir> // Для информации о диске
#include <QStorageInfo> // Для информации о диске
#include <QClipboard> // Включен из mainwindow.h

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

    codeHighlighter = new CodeHighlighter(chatDisplay->document()); // Инициализируем подсветку синтаксиса

    // Ввод сообщения
    messageInput = new QLineEdit(this);
    sendButton = new QPushButton("Отправить", this);
    modelSelector = new QComboBox(this);

    // Инициализация моделей
    availableModels["gpt-4o"] = {"gpt-4o", "OpenAI GPT-4o (лучшая)"};
    availableModels["mistral/mistral-7b-instruct"] = {"mistral/mistral-7b-instruct", "Mistral 7B Instruct (хорошая)"};
    availableModels["google/gemini-flash-1.5"] = {"google/gemini-flash-1.5", "Google Gemini Flash 1.5 (быстрая)"};
    availableModels["microsoft/phi-3-mini-4k-instruct"] = {"microsoft/phi-3-mini-4k-instruct", "Microsoft Phi-3 Mini 4K Instruct (компактная)"};
    availableModels["meta-llama/llama-3-8b-instruct"] = {"meta-llama/llama-3-8b-instruct", "Llama 3 8B Instruct (базовая)"};
    availableModels["nousresearch/nous-hermes-2-mixtral-8x7b-dpo"] = {"nousresearch/nous-hermes-2-mixtral-8x7b-dpo", "Nous Hermes 2 Mixtral 8x7B (мощная)"};
    availableModels["deepseek-ai/deepseek-coder-6.7b-instruct"] = {"deepseek-ai/deepseek-coder-6.7b-instruct", "DeepSeek Coder 6.7B Instruct (код)"};
    availableModels["qwen/qwen-14b-chat"] = {"qwen/qwen-14b-chat", "Qwen 14B Chat (многоязычная)"};
    availableModels["mistralai/mistral-nemo:free"] = {"mistralai/mistral-nemo:free", "Mixtral Nemo (универсальная)"};
    availableModels["google/gemma-7b-it"] = {"google/gemma-7b-it", "Google Gemma 7B IT (легкая)"};


    // Добавляем модели в QComboBox
    for (const auto& model : availableModels) {
        modelSelector->addItem(model.description, model.id);
    }

    // Устанавливаем ширину выпадающего списка
    modelSelector->view()->setMinimumWidth(modelSelector->minimumSizeHint().width() + 50); // Добавляем отступ

    // Подсказки для моделей
    hoverTimer = new QTimer(this);
    hoverTimer->setInterval(300); // Задержка 300 мс
    connect(hoverTimer, &QTimer::timeout, this, &MainWindow::showModelInfo);
    modelSelector->view()->installEventFilter(this);


    networkManager = new QNetworkAccessManager(this);

    // Соединения
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::sendMessage); // Отправка по Enter
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    // Размещение элементов
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    mainLayout->addWidget(chatDisplay);
    mainLayout->addWidget(modelSelector);
    mainLayout->addLayout(inputLayout);

    // Меню
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("Файл");
    settingsAction = fileMenu->addAction("Настройки");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    fileMenu->addAction("Выход", this, &QApplication::quit);
    setMenuBar(menuBar);

    // Загружаем настройки при старте
    loadSettings();

    // Применяем тему после загрузки настроек
    applyTheme(currentTheme);

    // Приветствие
    chatDisplay->append("<b style='color: #27ae60;'>AI:</b> Привет! Чем могу помочь сегодня?");
}

MainWindow::~MainWindow()
{
    saveSettings(); // Сохраняем настройки при закрытии
}

void MainWindow::sendMessage()
{
    QString message = messageInput->text().trimmed();
    if (message.isEmpty())
        return;

    chatDisplay->append("<b style='color: #3498db;'>Вы:</b> " + message);
    messageInput->clear();

    // Добавляем "AI печатает..."
    chatDisplay->append("AI печатает...");
    chatDisplay->repaint(); // Обновляем, чтобы сразу показать

    QString selectedModelId = modelSelector->currentData().toString();
    QUrl url("https://openrouter.ai/api/v1/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + OPENROUTER_API_KEY).toUtf8());
    request.setRawHeader("HTTP-Referer", "https://github.com/maksimb/AIAssistant"); // Замените на домен вашего приложения
    request.setRawHeader("X-Title", "AI Assistant Qt App"); // Замените на название вашего приложения

    QJsonObject messageUser;
    messageUser["role"] = "user";
    messageUser["content"] = message;

    QJsonObject messageSystem;
    messageSystem["role"] = "system";
    messageSystem["content"] = systemInfoContext; // Добавляем системную информацию как контекст

    QJsonArray messages;
    messages.append(messageSystem);
    messages.append(messageUser);

    QJsonObject requestBody;
    requestBody["model"] = selectedModelId;
    requestBody["messages"] = messages;

    QJsonDocument doc(requestBody);
    networkManager->post(request, doc.toJson());
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    // Удаляем "AI печатает..."
    if (chatDisplay->toPlainText().endsWith("AI печатает...\n")) {
        QTextCursor cursor = chatDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.select(QTextCursor::BlockUnderCursor);
        QString lastBlockText = cursor.selectedText();
        if (lastBlockText.trimmed() == "AI печатает...") {
            cursor.removeSelectedText();
            // Также удаляем предшествующий перевод строки, если он был добавлен append
            cursor.movePosition(QTextCursor::End);
            if (cursor.positionInBlock() == 0 && cursor.blockNumber() > 0) {
                cursor.deletePreviousChar(); // Удаляем перевод строки
            }
        }
    }


    if (reply->error() == QNetworkReply::NoError) {
        QString aiResponse = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(aiResponse.toUtf8());
        QJsonObject jsonObject = jsonResponse.object();

        QString content = "";
        if (jsonObject.contains("choices") && jsonObject["choices"].isArray()) {
            QJsonArray choices = jsonObject["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject firstChoice = choices[0].toObject();
                if (firstChoice.contains("message") && firstChoice["message"].isObject()) {
                    QJsonObject message = firstChoice["message"].toObject();
                    if (message.contains("content") && message["content"].isString()) {
                        content = message["content"].toString();
                    }
                }
            }
        }

        // ИЗМЕНЕНО: Добавляем "AI:" как отдельный блок, а затем содержимое как обычный текст.
        // Это позволяет QSyntaxHighlighter правильно обрабатывать маркеры кода.
        chatDisplay->append("<b style='color: #27ae60;'>AI:</b>"); // Добавляем "AI:" с цветом
        chatDisplay->append(content); // Добавляем фактический ответ как обычный текст

    } else {
        chatDisplay->append(QString("<b style='color: red;'>Ошибка:</b> %1").arg(reply->errorString()));
        qDebug() << "Network error:" << reply->errorString();
    }
    reply->deleteLater();
}

void MainWindow::showModelInfo()
{
    // Получаем текущий индекс элемента, над которым находится курсор
    // ИСПРАВЛЕНО: Используем mapFromGlobal(QCursor::pos()) для получения позиции курсора в координатах виджета
    QPoint pos = modelSelector->mapFromGlobal(QCursor::pos());
    QModelIndex index = modelSelector->view()->indexAt(pos);

    if (index.isValid() && index != lastHoveredIndex) {
        QString modelId = index.data(Qt::UserRole).toString();
        // Убедитесь, что 'availableModels' содержит ключ 'modelId'
        if (availableModels.contains(modelId)) {
            QToolTip::showText(QCursor::pos(), availableModels[modelId].description, modelSelector);
        }
        lastHoveredIndex = index;
    } else if (!index.isValid()) {
        QToolTip::hideText();
        lastHoveredIndex = QModelIndex(); // Сброс, когда курсор уходит
    }
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == modelSelector->view()) {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverMove) {
            QHoverEvent *hoverEvent = static_cast<QHoverEvent*>(event);
            // ИСПРАВЛЕНО: Используем position().toPoint() для QHoverEvent::pos()
            QModelIndex currentIndex = modelSelector->view()->indexAt(hoverEvent->position().toPoint());
            if (currentIndex.isValid() && currentIndex != lastHoveredIndex) {
                lastHoveredIndex = currentIndex;
                hoverTimer->stop();
                hoverTimer->start();
            } else if (!currentIndex.isValid()) {
                hoverTimer->stop();
                lastHoveredIndex = QModelIndex(); // Сброс
                QToolTip::hideText(); // Hide tooltip if mouse leaves a valid item
            }
        } else if (event->type() == QEvent::HoverLeave) {
            hoverTimer->stop();
            QToolTip::hideText();
            lastHoveredIndex = QModelIndex(); // Сброс
        }
    }
    return QMainWindow::eventFilter(obj, event);
}


void MainWindow::openSettings()
{
    SettingsDialog settingsDialog(currentChatFont, currentChatFontSize, currentTheme, this);
    if (settingsDialog.exec() == QDialog::Accepted) {
        currentChatFont = settingsDialog.selectedFont();
        currentChatFontSize = settingsDialog.selectedFontSize();
        currentTheme = settingsDialog.selectedTheme();

        // Применяем новые настройки
        QFont newFont = currentChatFont; // Создаем новую QFont на основе сохраненной
        newFont.setPointSize(currentChatFontSize); // Устанавливаем размер
        chatDisplay->setFont(newFont); // Применяем ко всему QTextEdit и его документу

        // Применяем тему
        applyTheme(currentTheme);

        // Сохраняем настройки
        saveSettings();
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("YourCompanyName", "AIAssistantQtApp"); // Замените на свои данные
    currentChatFont = settings.value("chatFont", QFont("Arial", 10)).value<QFont>();
    currentChatFontSize = settings.value("chatFontSize", 10).toInt();
    currentTheme = settings.value("appTheme", "dark").toString(); // По умолчанию темная тема

    // Применяем загруженные настройки сразу
    QFont initialFont = currentChatFont;
    initialFont.setPointSize(currentChatFontSize);
    chatDisplay->setFont(initialFont);
}

void MainWindow::saveSettings()
{
    QSettings settings("YourCompanyName", "AIAssistantQtApp"); // Замените на свои данные
    settings.setValue("chatFont", currentChatFont);
    settings.setValue("chatFontSize", currentChatFontSize);
    settings.setValue("appTheme", currentTheme);
}

void MainWindow::applyTheme(const QString &theme)
{
    QString styleSheet;
    if (theme == "dark") {
        styleSheet = R"(
            QMainWindow {
                background-color: #2e2e2e;
                color: #ffffff;
            }
            QTextEdit {
                background-color: #1e1e1e;
                color: #ffffff;
                border: 1px solid #3a3a3a;
                selection-background-color: #007acc;
            }
            QLineEdit {
                background-color: #3a3a3a;
                color: #ffffff;
                border: 1px solid #555555;
            }
            QPushButton {
                background-color: #007acc;
                color: #ffffff;
                border: none;
                padding: 8px 16px;
                border-radius: 4px;
            }
            QPushButton:hover {
                background-color: #005f99;
            }
            QComboBox {
                background-color: #3a3a3a;
                color: #ffffff;
                border: 1px solid #555555;
            }
            QComboBox::drop-down {
                border: none;
            }
            QComboBox::down-arrow {
                image: url(:/icons/down_arrow_white.png); /* Замените на путь к вашей иконке */
            }
            QComboBox QAbstractItemView {
                background-color: #3a3a3a;
                color: #ffffff;
                selection-background-color: #007acc;
            }
            QLabel {
                color: #ffffff;
            }
            QMenuBar {
                background-color: #3a3a3a;
                color: #ffffff;
            }
            QMenuBar::item {
                background-color: transparent;
                color: #ffffff;
            }
            QMenuBar::item:selected {
                background-color: #007acc;
                color: #ffffff;
            }
            QMenu {
                background-color: #3a3a3a;
                color: #ffffff;
                border: 1px solid #555555;
            }
            QMenu::item:selected {
                background-color: #007acc;
                color: #ffffff;
            }
            QDialog {
                background-color: #2e2e2e;
                color: #ffffff;
            }
            QSpinBox, QFontComboBox {
                background-color: #3a3a3a;
                color: #ffffff;
                border: 1px solid #555555;
            }
            QDialogButtonBox QPushButton {
                background-color: #007acc;
                color: #ffffff;
            }
            QDialogButtonBox QPushButton:hover {
                background-color: #005f99;
            }
        )";
    } else { // Светлая тема
        styleSheet = R"(
            QMainWindow {
                background-color: #f0f0f0;
                color: #333333;
            }
            QTextEdit {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #cccccc;
                selection-background-color: #007bff;
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
                padding: 8px 16px;
                border-radius: 4px;
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
                border: none;
            }
            QComboBox::down-arrow {
                image: url(:/icons/down_arrow_black.png); /* Замените на путь к вашей иконке */
            }
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
    qApp->setStyleSheet(styleSheet); // Использование qApp для установки глобального стиля
}

QString MainWindow::getSystemInfo()
{
    QString info = "=== System Information ===\n";
    info += "OS Type: " + QSysInfo::prettyProductName() + "\n";
    info += "CPU Architecture: " + QSysInfo::currentCpuArchitecture() + "\n";

    // CPU Cores/Threads (Linux/macOS example, Windows needs specific WMI/registry query)
    QProcess cpuProcess;
#ifdef Q_OS_LINUX
    cpuProcess.start("nproc --all");
#elif defined(Q_OS_MAC)
    cpuProcess.start("sysctl -n hw.ncpu");
#elif defined(Q_OS_WIN)
    // For Windows, a simpler approach for logical processors
    cpuProcess.start("powershell.exe -Command \"(Get-WmiObject -class Win32_Processor).NumberOfLogicalProcessors | Measure-Object -Sum | Select -ExpandProperty Sum\"");
#else
    cpuProcess.start("echo 'CPU info not available'");
#endif
    cpuProcess.waitForFinished();
    QString cpuThreads = cpuProcess.readAllStandardOutput().trimmed();
    info += "CPU Threads: " + cpuThreads + "\n";

    // RAM Info (Linux/macOS example, Windows needs WMI/registry query)
    QProcess ramProcess;
#ifdef Q_OS_LINUX
    ramProcess.start("free -h | grep Mem | awk '{print $2}'");
#elif defined(Q_OS_MAC)
    ramProcess.start("sysctl -n hw.memsize | awk '{print $1/1073741824\" GB\"}'");
#elif defined(Q_OS_WIN)
    // For Windows, total physical memory
    ramProcess.start("powershell.exe -Command \"(Get-WmiObject -class Win32_ComputerSystem).TotalPhysicalMemory | Foreach-Object { ($ / 1GB).ToString('N2') + ' GB' }\"");
#else
    ramProcess.start("echo 'RAM info not available'");
#endif
    ramProcess.waitForFinished();
    QString ramOutput = ramProcess.readAllStandardOutput().trimmed();
    info += "Total RAM: " + ramOutput + "\n";

    // Disk Space (Example for C: drive on Windows, or root on Linux/macOS)
    QProcess diskProcess;
#ifdef Q_OS_LINUX
    diskProcess.start("df -h / | awk 'NR==2 {print $2}'"); // Total disk space for root
#elif defined(Q_OS_MAC)
    diskProcess.start("df -h / | awk 'NR==2 {print $2}'"); // Total disk space for root
#elif defined(Q_OS_WIN)
    // For Windows, get total size of C: drive
    diskProcess.start("powershell.exe -Command \"(Get-WmiObject -Class Win32_LogicalDisk -Filter 'DriveType=3 and DeviceID=\"C:\"').Size | Foreach-Object { ($ / 1GB).ToString('N2') + ' GB' }\"");
#else
    diskProcess.start("echo 'Disk info not available'");
#endif
    diskProcess.waitForFinished();
    QString diskOutput = diskProcess.readAllStandardOutput().trimmed();
    info += "Total Disk Space (C:/): " + diskOutput + "\n"; // Or root for Linux/macOS

    info += "Current Time: " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n";

    return info;
}
