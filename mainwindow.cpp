#include "mainwindow.h"
#include <QLabel>
#include <QTextOption>
#include <QListView>
#include <QDebug>
#include <QMenuBar>
#include <QApplication>
#include <QMenu>

#include <QCoreApplication>
#include <QSysInfo>
#include <QProcess>
#include <QThread>
#include <QDateTime>
#include <QDir>
#include <QStorageInfo>
#include <QClipboard>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QString systemInfo = getSystemInfo();
    setMinimumSize(800,400);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setAcceptRichText(true);
    chatDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(chatDisplay, &QTextEdit::customContextMenuRequested, this, &MainWindow::onCustomContextMenuRequested);

    this->systemInfoContext = systemInfo;

    QTextOption textOption = chatDisplay->document()->defaultTextOption();
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    chatDisplay->document()->setDefaultTextOption(textOption);

    codeHighlighter = new CodeHighlighter(chatDisplay->document());

    messageInput = new QLineEdit(this);
    sendButton = new QPushButton("Отправить", this);
    modelSelector = new QComboBox(this);

    availableModels["mistralai/devstral-small-2505:free"] = {"mistralai/devstral-small-2505:free", "Mistral: Devstral"};
    availableModels["qwen/qwen3-235b-a22b"] = {"qwen/qwen3-235b-a22b", "Qwen: Qwen3 235B A22B"};
    availableModels["deepseek/deepseek-r1-0528:free"] = {"deepseek/deepseek-r1-0528:free", "DeepSeek: R1 0528"};
    availableModels["google/gemini-2.0-flash-exp:free"] = {"google/gemini-2.0-flash-exp:free", "Google: Gemini 2.5 Flash"};
    availableModels["mistralai/mistral-nemo:free"] = {"mistralai/mistral-nemo:free", "Mistral: Mistral Nemo"};
    availableModels["qwen/qwq-32b:free"] = {"qwen/qwq-32b:free", "Qwen: QwQ 32B"};
    availableModels["meta-llama/llama-3.3-70b-instruct:free"] = {"meta-llama/llama-3.3-70b-instruct:free", "Meta: Llama 3.3 70B"};
    availableModels["google/gemma-3-12b-it:free"] = {"google/gemma-3-12b-it:free", "Google: Gemma 3"};
    availableModels["meta-llama/llama-3.1-405b-instruct:free"] = {"meta-llama/llama-3.1-405b-instruct:free", "Meta: Llama 3.1 405B"};
    availableModels["mistralai/mistral-small-24b-instruct-2501:free"] = {"mistralai/mistral-small-24b-instruct-2501:free", "Mistral: Mistral Small"};

    for (const auto& model : availableModels) {
        modelSelector->addItem(model.description, model.id);
    }

    modelSelector->view()->setMinimumWidth(modelSelector->minimumSizeHint().width() + 50);

    hoverTimer = new QTimer(this);
    hoverTimer->setInterval(300);
    connect(hoverTimer, &QTimer::timeout, this, &MainWindow::showModelInfo);
    modelSelector->view()->installEventFilter(this);

    networkManager = new QNetworkAccessManager(this);

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);
    // Отключаем старое соединение, так как будем использовать стриминг для ответов AI
    // connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    QHBoxLayout *topButtonsLayout = new QHBoxLayout();
    topButtonsLayout->addWidget(modelSelector);

    mainLayout->addWidget(chatDisplay);
    mainLayout->addLayout(topButtonsLayout);
    mainLayout->addLayout(inputLayout);

    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("Файл");

    newChatAction = fileMenu->addAction("Новый чат");
    connect(newChatAction, &QAction::triggered, this, &MainWindow::startNewChat);

    settingsAction = fileMenu->addAction("Настройки");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    fileMenu->addAction("Выход", this, &QApplication::quit);
    setMenuBar(menuBar);

    copyCodeAction = new QAction("Копировать блок кода", this);
    connect(copyCodeAction, &QAction::triggered, this, &MainWindow::copyCodeBlock);

    loadSettings();
    applyTheme(currentTheme);

    chatDisplay->append("<b style='color: #27ae60;'>AI:</b> Привет! Чем могу помочь сегодня?");
}

MainWindow::~MainWindow()
{
    // Убедитесь, что reply удаляется при закрытии, если оно все еще активно
    if (currentReply) {
        currentReply->abort(); // Прервать текущий запрос
        currentReply->deleteLater();
    }
    saveSettings();
}

void MainWindow::sendMessage()
{
    QString message = messageInput->text().trimmed();
    if (message.isEmpty())
        return;

    chatDisplay->append("<b style='color: #3498db;'>Вы:</b> " + message);
    messageInput->clear();

    // Удаляем старую надпись "AI печатает..." если она осталась
    QTextCursor cursor = chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.select(QTextCursor::BlockUnderCursor);
    QString lastBlockText = cursor.selectedText();
    if (lastBlockText.trimmed() == "AI печатает...") {
        cursor.removeSelectedText();
        if (cursor.positionInBlock() == 0 && cursor.blockNumber() > 0) {
            cursor.deletePreviousChar(); // Удалить лишний перенос строки
        }
    }


    chatDisplay->append("AI печатает...");
    chatDisplay->repaint(); // Обновляем отображение сразу

    // Деактивируем ввод и кнопку отправки, пока идет ответ
    messageInput->setEnabled(false);
    sendButton->setEnabled(false);

    firstChunkReceived = false; // Сбрасываем флаг для нового ответа

    QString selectedModelId = modelSelector->currentData().toString();
    QUrl url(OPENROUTER_API_URL); // Используем константу OPENROUTER_API_URL
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + OPENROUTER_API_KEY).toUtf8());
    request.setRawHeader("HTTP-Referer", "https://github.com/maksimb/AIAssistant");
    request.setRawHeader("X-Title", "AI Assistant Qt App");

    QJsonObject messageUser;
    messageUser["role"] = "user";
    messageUser["content"] = message;

    QJsonObject messageSystem;
    messageSystem["role"] = "system";
    messageSystem["content"] = systemInfoContext;

    QJsonArray messages;
    messages.append(messageSystem);
    messages.append(messageUser);

    QJsonObject requestBody;
    requestBody["model"] = selectedModelId;
    requestBody["messages"] = messages;
    requestBody["stream"] = true; // ВАЖНО: Включаем стриминг

    QJsonDocument doc(requestBody);

    // Удаляем предыдущий reply, если он есть
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
        currentReply = nullptr;
    }

    currentReply = networkManager->post(request, doc.toJson());

    // Подключаем новые слоты для стриминга
    connect(currentReply, &QNetworkReply::readyRead, this, &MainWindow::onStreamReadyRead);
    connect(currentReply, &QNetworkReply::finished, this, &MainWindow::onStreamFinished);
}

// Этот слот больше не нужен для обработки ответов AI, так как мы используем стриминг.
// Вы можете удалить его или переиспользовать для не-стриминговых запросов, если они будут.
// void MainWindow::onNetworkReply(QNetworkReply *reply) { ... }


void MainWindow::onStreamReadyRead()
{
    if (!currentReply) return;

    replyBuffer.append(currentReply->readAll());

    // Process each Server-Sent Event (SSE)
    // SSE events are delimited by two newline characters (\n\n)
    while (replyBuffer.contains("\n\n")) {
        int endOfEvent = replyBuffer.indexOf("\n\n");
        QString eventData = replyBuffer.left(endOfEvent + 2); // +2 to include \n\n
        replyBuffer.remove(0, endOfEvent + 2);

        // Check for "data: " prefix
        if (eventData.startsWith("data: ")) {
            QString jsonString = eventData.mid(6).trimmed(); // Remove "data: " and trim whitespace

            if (jsonString == "[DONE]") {
                // End of stream
                break; // Exit loop, finished will handle cleanup
            }

            QJsonDocument jsonResponse = QJsonDocument::fromJson(jsonString.toUtf8());
            QJsonObject jsonObject = jsonResponse.object();

            QString content = "";
            if (jsonObject.contains("choices") && jsonObject["choices"].isArray()) {
                QJsonArray choices = jsonObject["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices[0].toObject();
                    if (firstChoice.contains("delta") && firstChoice["delta"].isObject()) {
                        QJsonObject delta = firstChoice["delta"].toObject();
                        if (delta.contains("content") && delta["content"].isString()) {
                            content = delta["content"].toString();
                        }
                    }
                }
            }

            // Удаляем "AI печатает..." при первом получении контента
            if (!firstChunkReceived && !content.isEmpty()) {
                QTextCursor cursor = chatDisplay->textCursor();
                cursor.movePosition(QTextCursor::End);
                cursor.select(QTextCursor::BlockUnderCursor);
                QString lastBlockText = cursor.selectedText();
                if (lastBlockText.trimmed() == "AI печатает...") {
                    cursor.removeSelectedText();
                    if (cursor.positionInBlock() == 0 && cursor.blockNumber() > 0) {
                        cursor.deletePreviousChar(); // Удалить лишний перенос строки
                    }
                }
                chatDisplay->insertHtml("<b style='color: #27ae60;'>AI:</b> "); // Добавляем префикс "AI:"
                firstChunkReceived = true;
            }

            // Добавляем новый контент
            if (!content.isEmpty()) {
                chatDisplay->insertPlainText(content);
                // Прокручиваем до конца, чтобы видеть новый текст
                QTextCursor tc = chatDisplay->textCursor();
                tc.movePosition(QTextCursor::End);
                chatDisplay->setTextCursor(tc);
            }
        }
    }
}


void MainWindow::onStreamFinished()
{
    if (currentReply->error() != QNetworkReply::NoError) {
        chatDisplay->append(QString("<b style='color: red;'>Ошибка:</b> %1").arg(currentReply->errorString()));
        qDebug() << "Stream error:" << currentReply->errorString();
    }
    // Удаляем reply после завершения
    currentReply->deleteLater();
    currentReply = nullptr;

    // Снова активируем ввод и кнопку отправки
    messageInput->setEnabled(true);
    sendButton->setEnabled(true);
    messageInput->setFocus();
    replyBuffer.clear(); // Очищаем буфер после завершения стриминга
    firstChunkReceived = false; // Сбрасываем флаг
}


void MainWindow::showModelInfo()
{
    QPoint pos = modelSelector->mapFromGlobal(QCursor::pos());
    QModelIndex index = modelSelector->view()->indexAt(pos);

    if (index.isValid() && index != lastHoveredIndex) {
        QString modelId = index.data(Qt::UserRole).toString();
        if (availableModels.contains(modelId)) {
            QToolTip::showText(QCursor::pos(), availableModels[modelId].description, modelSelector);
        }
        lastHoveredIndex = index;
    } else if (!index.isValid()) {
        QToolTip::hideText();
        lastHoveredIndex = QModelIndex();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == modelSelector->view()) {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverMove) {
            QHoverEvent *hoverEvent = static_cast<QHoverEvent*>(event);
            QModelIndex currentIndex = modelSelector->view()->indexAt(hoverEvent->position().toPoint());
            if (currentIndex.isValid() && currentIndex != lastHoveredIndex) {
                lastHoveredIndex = currentIndex;
                hoverTimer->stop();
                hoverTimer->start();
            } else if (!currentIndex.isValid()) {
                hoverTimer->stop();
                lastHoveredIndex = QModelIndex();
                QToolTip::hideText();
            }
        } else if (event->type() == QEvent::HoverLeave) {
            hoverTimer->stop();
            QToolTip::hideText();
            lastHoveredIndex = QModelIndex();
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

        QFont newFont = currentChatFont;
        newFont.setPointSize(currentChatFontSize);
        chatDisplay->setFont(newFont);

        applyTheme(currentTheme);
        saveSettings();
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("YourCompanyName", "AIAssistantQtApp");
    currentChatFont = settings.value("chatFont", QFont("Arial", 10)).value<QFont>();
    currentChatFontSize = settings.value("chatFontSize", 10).toInt();
    currentTheme = settings.value("appTheme", "dark").toString();

    QFont initialFont = currentChatFont;
    initialFont.setPointSize(currentChatFontSize);
    chatDisplay->setFont(initialFont);
}

void MainWindow::saveSettings()
{
    QSettings settings("YourCompanyName", "AIAssistantQtApp");
    settings.setValue("chatFont", currentChatFont);
    settings.setValue("chatFontSize", currentChatFontSize);
    settings.setValue("appTheme", currentTheme);
}

void MainWindow::applyTheme(const QString &theme)
{
    QString styleSheet;
    if (theme == "dark") {
        styleSheet = R"(
            QMainWindow { background-color: #2e2e2e; color: #ffffff; }
            QTextEdit { background-color: #1e1e1e; color: #ffffff; border: 1px solid #3a3a3a; selection-background-color: #007acc; }
            QLineEdit { background-color: #3a3a3a; color: #ffffff; border: 1px solid #555555; }
            QPushButton { background-color: #007acc; color: #ffffff; border: none; padding: 8px 16px; border-radius: 4px; }
            QPushButton:hover { background-color: #005f99; }
            QComboBox { background-color: #3a3a3a; color: #ffffff; border: 1px solid #555555; }
            QComboBox::drop-down { border: none; }
            QComboBox::down-arrow { image: url(:/icons/down_arrow_white.png); }
            QComboBox QAbstractItemView { background-color: #3a3a3a; color: #ffffff; selection-background-color: #007acc; }
            QLabel { color: #ffffff; }
            QMenuBar { background-color: #3a3a3a; color: #ffffff; }
            QMenuBar::item { background-color: transparent; color: #ffffff; }
            QMenuBar::item:selected { background-color: #007acc; color: #ffffff; }
            QMenu { background-color: #3a3a3a; color: #ffffff; border: 1px solid #555555; }
            QMenu::item:selected { background-color: #007acc; color: #ffffff; }
            QDialog { background-color: #2e2e2e; color: #ffffff; }
            QSpinBox, QFontComboBox { background-color: #3a3a3a; color: #ffffff; border: 1px solid #555555; }
            QDialogButtonBox QPushButton { background-color: #007acc; color: #ffffff; }
            QDialogButtonBox QPushButton:hover { background-color: #005f99; }
        )";
    } else {
        styleSheet = R"(
            QMainWindow { background-color: #f0f0f0; color: #333333; }
            QTextEdit { background-color: #ffffff; color: #333333; border: 1px solid #cccccc; selection-background-color: #007bff; }
            QLineEdit { background-color: #ffffff; color: #333333; border: 1px solid #cccccc; }
            QPushButton { background-color: #007bff; color: #ffffff; border: none; padding: 8px 16px; border-radius: 4px; }
            QPushButton:hover { background-color: #0056b3; }
            QComboBox { background-color: #ffffff; color: #333333; border: 1px solid #cccccc; }
            QComboBox::drop-down { border: none; }
            QComboBox::down-arrow { image: url(:/icons/down_arrow_black.png); }
            QComboBox QAbstractItemView { background-color: #ffffff; color: #333333; selection-background-color: #007bff; }
            QLabel { color: #333333; }
            QMenuBar { background-color: #e0e0e0; color: #333333; }
            QMenuBar::item { background-color: transparent; color: #333333; }
            QMenuBar::item:selected { background-color: #007bff; color: #ffffff; }
            QMenu { background-color: #e0e0e0; color: #333333; border: 1px solid #cccccc; }
            QMenu::item:selected { background-color: #007bff; color: #ffffff; }
            QDialog { background-color: #f0f0f0; color: #333333; }
            QSpinBox, QFontComboBox { background-color: #ffffff; color: #333333; border: 1px solid #cccccc; }
            QDialogButtonBox QPushButton { background-color: #007bff; color: #ffffff; }
            QDialogButtonBox QPushButton:hover { background-color: #0056b3; }
        )";
    }
    qApp->setStyleSheet(styleSheet);
}

QString MainWindow::getSystemInfo()
{
    QString info = "=== System Information ===\n";
    info += "OS Type: " + QSysInfo::prettyProductName() + "\n";
    info += "CPU Architecture: " + QSysInfo::currentCpuArchitecture() + "\n";

    QProcess cpuProcess;
#ifdef Q_OS_LINUX
    cpuProcess.start("nproc --all");
#elif defined(Q_OS_MAC)
    cpuProcess.start("sysctl -n hw.ncpu");
#elif defined(Q_OS_WIN)
    cpuProcess.start("powershell.exe -Command \"(Get-WmiObject -class Win32_Processor).NumberOfLogicalProcessors | Measure-Object -Sum | Select -ExpandProperty Sum\"");
#else
    cpuProcess.start("echo 'CPU info not available'");
#endif
    cpuProcess.waitForFinished();
    QString cpuThreads = cpuProcess.readAllStandardOutput().trimmed();
    info += "CPU Threads: " + cpuThreads + "\n";

    QProcess ramProcess;
#ifdef Q_OS_LINUX
    ramProcess.start("free -h | grep Mem | awk '{print $2}'");
#elif defined(Q_OS_MAC)
    ramProcess.start("sysctl -n hw.memsize | awk '{print $1/1073741824\" GB\"}'");
#elif defined(Q_OS_WIN)
    ramProcess.start("powershell.exe -Command \"(Get-WmiObject -class Win32_ComputerSystem).TotalPhysicalMemory | Foreach-Object { ($ / 1GB).ToString('N2') + ' GB' }\"");
#else
    ramProcess.start("echo 'RAM info not available'");
#endif
    ramProcess.waitForFinished();
    QString ramOutput = ramProcess.readAllStandardOutput().trimmed();
    info += "Total RAM: " + ramOutput + "\n";

    QProcess diskProcess;
#ifdef Q_OS_LINUX
    diskProcess.start("df -h / | awk 'NR==2 {print $2}'");
#elif defined(Q_OS_MAC)
    diskProcess.start("df -h / | awk 'NR==2 {print $2}'");
#elif defined(Q_OS_WIN)
    diskProcess.start("powershell.exe -Command \"(Get-WmiObject -Class Win32_LogicalDisk -Filter 'DriveType=3 and DeviceID=\"C:\"').Size | Foreach-Object { ($ / 1GB).ToString('N2') + ' GB' }\"");
#else
    diskProcess.start("echo 'Disk info not available'");
#endif
    diskProcess.waitForFinished();
    QString diskOutput = diskProcess.readAllStandardOutput().trimmed();
    info += "Total Disk Space (C:/): " + diskOutput + "\n";

    info += "Current Time: " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n";

    return info;
}

void MainWindow::startNewChat()
{
    chatDisplay->clear();
    chatDisplay->append("<b style='color: #27ae60;'>AI:</b> Привет! Чем могу помочь сегодня?");
}

void MainWindow::copyCodeBlock()
{
    QTextCursor cursor = chatDisplay->textCursor();
    QTextDocument *doc = chatDisplay->document();
    int currentBlockNumber = cursor.blockNumber();

    QString codeContent;
    int startBlock = -1;
    int endBlock = -1;

    for (int i = currentBlockNumber; i >= 0; --i) {
        QTextBlock block = doc->findBlockByNumber(i);
        if (block.text().trimmed().startsWith("```")) {
            startBlock = i;
            break;
        }
    }

    if (startBlock != -1) {
        for (int i = startBlock + 1; i < doc->blockCount(); ++i) {
            QTextBlock block = doc->findBlockByNumber(i);
            if (block.text().trimmed() == "```") {
                endBlock = i;
                break;
            }
        }
    }

    if (startBlock != -1 && endBlock != -1) {
        for (int i = startBlock + 1; i < endBlock; ++i) {
            QTextBlock block = doc->findBlockByNumber(i);
            codeContent += block.text() + "\n";
        }
        if (!codeContent.isEmpty() && codeContent.endsWith("\n")) {
            codeContent.chop(1);
        }

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(codeContent);
        qDebug() << "Code copied to clipboard:" << codeContent;
    } else {
        qDebug() << "No complete code block found at cursor position.";
    }
}

void MainWindow::onCustomContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(copyCodeAction);

    QTextCursor cursor = chatDisplay->cursorForPosition(pos);
    chatDisplay->setTextCursor(cursor);

    bool isInCodeBlock = false;
    int currentBlockNumber = cursor.blockNumber();
    QTextDocument *doc = chatDisplay->document();

    int startBlock = -1;
    int endBlock = -1;

    for (int i = currentBlockNumber; i >= 0; --i) {
        QTextBlock block = doc->findBlockByNumber(i);
        if (block.text().trimmed().startsWith("```")) {
            startBlock = i;
            break;
        }
    }

    if (startBlock != -1) {
        for (int i = startBlock + 1; i < doc->blockCount(); ++i) {
            QTextBlock block = doc->findBlockByNumber(i);
            if (block.text().trimmed() == "```") {
                endBlock = i;
                break;
            }
        }
    }

    if (startBlock != -1 && endBlock != -1 && currentBlockNumber > startBlock && currentBlockNumber < endBlock) {
        isInCodeBlock = true;
    }

    copyCodeAction->setEnabled(isInCodeBlock);

    menu.exec(chatDisplay->mapToGlobal(pos));
}
