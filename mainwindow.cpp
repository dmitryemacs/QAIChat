// mainwindow.cpp
#include "mainwindow.h"
#include <QLabel>
#include <QTextOption>
#include <QListView>
#include <QDebug>
#include <QMenuBar>
#include <QApplication>
#include <QMenu>
#include <QScrollBar>

#include <QCoreApplication>
#include <QSysInfo>
#include <QProcess>
#include <QThread>
#include <QDateTime>
#include <QDir>
#include <QStorageInfo>
#include <QClipboard>

#include <QRegularExpression> // Для парсинга блоков кода

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Загрузка настроек приложения (шрифт, размер, тема)
    loadSettings();

    QString systemInfo = getSystemInfo();
    setMinimumSize(1280,500);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    chatScrollArea = new QScrollArea(this);
    chatScrollArea->setWidgetResizable(true);
    chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *scrollContent = new QWidget(chatScrollArea);
    chatContentLayout = new QVBoxLayout(scrollContent);
    chatContentLayout->setAlignment(Qt::AlignTop);
    chatContentLayout->setContentsMargins(10, 10, 10, 10);
    chatContentLayout->setSpacing(5);

    // НОВОЕ: Добавляем растяжку, чтобы сообщения прижимались к верху
    chatContentLayout->addStretch(); // Это заставит содержимое прижиматься к верху

    scrollContent->setLayout(chatContentLayout);
    chatScrollArea->setWidget(scrollContent);
    mainLayout->addWidget(chatScrollArea);

    this->systemInfoContext = systemInfo;

    QHBoxLayout *inputLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Введите ваше сообщение...");
    QFont inputFont = currentChatFont;
    inputFont.setPointSize(currentChatFontSize);
    messageInput->setFont(inputFont);

    sendButton = new QPushButton("Отправить", this);
    sendButton->setFont(inputFont);

    modelSelector = new QComboBox(this);
    modelSelector->setFont(inputFont);
    modelSelector->setToolTip("Выберите модель для чата.");

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);
    inputLayout->addWidget(modelSelector);
    mainLayout->addLayout(inputLayout);

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);
    connect(modelSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::showModelInfo);

    networkManager = new QNetworkAccessManager(this);

    QNetworkRequest request(OPENROUTER_BASE_URL + "/models");
    request.setRawHeader("Authorization", ("Bearer " + OPENROUTER_API_KEY).toUtf8());
    request.setRawHeader("HTTP-Referer", "https://github.com/maksimr/AIChat");
    request.setRawHeader("X-Title", "AI Chat Desktop App");

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QJsonArray data = doc.object()["data"].toArray();
                modelSelector->clear();
                availableModels.clear();
                for (const QJsonValue &value : data) {
                    QJsonObject modelObject = value.toObject();
                    QString id = modelObject["id"].toString();
                    QString description = modelObject["description"].toString();
                    modelSelector->addItem(id);
                    availableModels.insert(id, {id, description});
                }
            }
        } else {
            qDebug() << "Error fetching models:" << reply->errorString();
        }
        reply->deleteLater();
    });

    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *fileMenu = menuBar->addMenu("Файл");
    newChatAction = fileMenu->addAction("Новый чат");
    connect(newChatAction, &QAction::triggered, this, &MainWindow::startNewChat);

    QMenu *settingsMenu = menuBar->addMenu("Настройки");
    settingsAction = settingsMenu->addAction("Настройки...");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    applyTheme(currentTheme);
}

MainWindow::~MainWindow()
{
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
    }
    saveSettings();
}

void MainWindow::loadSettings()
{
    QSettings settings("YourCompany", "AIChat");
    currentChatFont = settings.value("font", QFont("Arial", 10)).value<QFont>();
    currentChatFontSize = settings.value("fontSize", 10).toInt();
    currentTheme = settings.value("theme", "light").toString();
}

void MainWindow::saveSettings()
{
    QSettings settings("YourCompany", "AIChat");
    settings.setValue("font", currentChatFont);
    settings.setValue("fontSize", currentChatFontSize);
    settings.setValue("theme", currentTheme);
}

void MainWindow::applyTheme(const QString &theme)
{
    QPalette palette = this->palette();
    if (theme == "dark") {
        palette.setColor(QPalette::Window, QColor("#2b2b2b"));
        palette.setColor(QPalette::WindowText, QColor("#cccccc"));
        palette.setColor(QPalette::Base, QColor("#3c3c3c"));
        palette.setColor(QPalette::AlternateBase, QColor("#2b2b2b"));
        palette.setColor(QPalette::Text, QColor("#cccccc"));
        palette.setColor(QPalette::Button, QColor("#4a4a4a"));
        palette.setColor(QPalette::ButtonText, QColor("#cccccc"));
        palette.setColor(QPalette::Highlight, QColor("#569cd6"));
        palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    } else { // Light theme
        palette.setColor(QPalette::Window, QColor("#f0f0f0"));
        palette.setColor(QPalette::WindowText, QColor("#333333"));
        palette.setColor(QPalette::Base, QColor("#ffffff"));
        palette.setColor(QPalette::AlternateBase, QColor("#f0f0f0"));
        palette.setColor(QPalette::Text, QColor("#333333"));
        palette.setColor(QPalette::Button, QColor("#e0e0e0"));
        palette.setColor(QPalette::ButtonText, QColor("#333333"));
        palette.setColor(QPalette::Highlight, QColor("#aed6f1"));
        palette.setColor(QPalette::HighlightedText, QColor("#000000"));
    }
    setPalette(palette);

    messageInput->setPalette(palette);
    sendButton->setPalette(palette);
    modelSelector->setPalette(palette);
    menuBar()->setPalette(palette);

    chatScrollArea->setPalette(palette);
    chatScrollArea->widget()->setPalette(palette);

    QFont inputFont = currentChatFont;
    inputFont.setPointSize(currentChatFontSize);
    messageInput->setFont(inputFont);
    sendButton->setFont(inputFont);
    modelSelector->setFont(inputFont);

    for (int i = 0; i < chatContentLayout->count(); ++i) {
        QLayoutItem *item = chatContentLayout->itemAt(i);
        if (QWidget *widget = item->widget()) {
            widget->setPalette(palette);
            if (QLabel *label = qobject_cast<QLabel*>(widget)) {
                label->setFont(currentChatFont);
            }
            // CodeBlockWidget должен был применить тему при создании,
            // но для динамического обновления можно добавить метод
        }
    }
}

QString MainWindow::getSystemInfo() {
    QString info;
    info += "OS: " + QSysInfo::prettyProductName() + "\n";
    info += "Kernel: " + QSysInfo::kernelType() + " " + QSysInfo::kernelVersion() + "\n";
    info += "Architecture: " + QSysInfo::currentCpuArchitecture() + "\n";
    info += "Hostname: " + QSysInfo::machineHostName() + "\n";
    //nfo += "User: " + QSysInfo::userName() + "\n";
    info += "CPU Cores: " + QString::number(QThread::idealThreadCount()) + "\n";

    QStorageInfo storage = QStorageInfo::root();
    info += "Root Disk Total: " + QString::number(storage.bytesTotal() / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB\n";
    info += "Root Disk Free: " + QString::number(storage.bytesAvailable() / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB\n";

    info += "Current Dir: " + QDir::currentPath() + "\n";
    //info += "Qt Version: " + qVersion() + "\n";

    return info;
}

// новый параметр targetLabel
void MainWindow::addChatMessage(const QString &sender, const QString &text, bool isCode, const QString &language, QLabel *targetLabel)
{
    if (targetLabel) {
        targetLabel->setText(text);
        // Прокрутка вниз при обновлении
        QScrollBar *vScrollBar = chatScrollArea->verticalScrollBar();
        vScrollBar->setValue(vScrollBar->maximum());
        return;
    }

    if (sender == "AI" && aiResponseLabel) {
        aiResponseLabel->hide(); // чтобы он не мерцал перед удалением
    }


    QWidget *messageContainer = new QWidget(chatScrollArea->widget());
    QVBoxLayout *messageLayout = new QVBoxLayout(messageContainer);
    messageLayout->setContentsMargins(5, 5, 5, 5);
    messageLayout->setSpacing(2);

    QLabel *senderLabel = new QLabel(sender, messageContainer);
    QFont senderFont = currentChatFont;
    senderFont.setPointSize(currentChatFontSize - 2);
    senderFont.setBold(true);
    senderLabel->setFont(senderFont);
    messageLayout->addWidget(senderLabel);

    if (isCode) {
        CodeBlockWidget *codeBlock = new CodeBlockWidget(language, text, messageContainer);
        QFont codeFont = currentChatFont;
        codeFont.setFamily("Cascadia Code");
        codeFont.setPointSize(currentChatFontSize - 1);
        codeBlock->findChild<QTextEdit*>()->setFont(codeFont); // шрифт для внутреннего QTextEdit
        messageLayout->addWidget(codeBlock);
        codeBlock->setPalette(this->palette());
    } else {
        QLabel *textLabel = new QLabel(text, messageContainer);
        textLabel->setFont(currentChatFont);
        textLabel->setWordWrap(true);
        textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        textLabel->setPalette(this->palette());
        messageLayout->addWidget(textLabel);
    }

    messageContainer->setLayout(messageLayout);
    chatContentLayout->addWidget(messageContainer);

    // Прокручиваем до конца после добавления сообщения
    QScrollBar *vScrollBar = chatScrollArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
}


void MainWindow::sendMessage()
{
    QString messageText = messageInput->text().trimmed();
    if (messageText.isEmpty())
        return;

    addChatMessage("Вы", messageText); // Добавляем сообщение пользователя в чат

    messageInput->clear();

    // ИНИЦИАЛИЗАЦИЯ ВРЕМЕННОГО QLabel ДЛЯ ОТВЕТА AI
    if (!aiResponseLabel) {
        QWidget *aiMessageContainer = new QWidget(chatScrollArea->widget());
        QVBoxLayout *aiMessageLayout = new QVBoxLayout(aiMessageContainer);
        aiMessageLayout->setContentsMargins(5, 5, 5, 5);
        aiMessageLayout->setSpacing(2);

        QLabel *senderLabel = new QLabel("AI", aiMessageContainer);
        QFont senderFont = currentChatFont;
        senderFont.setPointSize(currentChatFontSize - 2);
        senderFont.setBold(true);
        senderLabel->setFont(senderFont);
        aiMessageLayout->addWidget(senderLabel);

        aiResponseLabel = new QLabel("AI печатает...", aiMessageContainer); // Инициализируем aiResponseLabel
        aiResponseLabel->setFont(currentChatFont);
        aiResponseLabel->setWordWrap(true);
        aiResponseLabel->setPalette(this->palette());
        aiResponseLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        aiMessageLayout->addWidget(aiResponseLabel);

        aiMessageContainer->setLayout(aiMessageLayout);
        chatContentLayout->addWidget(aiMessageContainer);
    } else {
        // Если уже есть, просто очистим и покажем "AI печатает..."
        aiResponseLabel->setText("AI печатает...");
        aiResponseLabel->parentWidget()->show();
    }
    QScrollBar *vScrollBar = chatScrollArea->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());


    QNetworkRequest request(OPENROUTER_CHAT_COMPLETIONS_URL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + OPENROUTER_API_KEY).toUtf8());
    request.setRawHeader("HTTP-Referer", "https://github.com/maksimr/AIChat");
    request.setRawHeader("X-Title", "AI Chat Desktop App");

    QJsonObject requestBody;
    requestBody["model"] = modelSelector->currentText();
    requestBody["stream"] = true;
    QJsonArray messages;

    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = systemInfoContext;
    messages.append(systemMessage);

    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = messageText;
    messages.append(userMessage);

    requestBody["messages"] = messages;

    currentReply = networkManager->post(request, QJsonDocument(requestBody).toJson());
    connect(currentReply, &QNetworkReply::readyRead, this, &MainWindow::onStreamReadyRead);
    connect(currentReply, &QNetworkReply::finished, this, &MainWindow::onStreamFinished);
    replyBuffer.clear(); // Очищаем буфер для нового ответа
}

void MainWindow::onStreamReadyRead() {
    if (!currentReply) return;

    replyBuffer.append(currentReply->readAll());

    QStringList lines = replyBuffer.split('\n', Qt::SkipEmptyParts);
    replyBuffer.clear();

    QString currentAssistantContent;
    for (const QString &line : lines) {
        if (line.startsWith("data: ")) {
            QString jsonData = line.mid(6).trimmed();
            if (jsonData == "[DONE]") {
                continue;
            }

            QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("choices") && obj["choices"].isArray()) {
                    QJsonArray choices = obj["choices"].toArray();
                    if (!choices.isEmpty()) {
                        QJsonObject firstChoice = choices.first().toObject();
                        if (firstChoice.contains("delta") && firstChoice["delta"].isObject()) {
                            QJsonObject delta = firstChoice["delta"].toObject();
                            if (delta.contains("content") && delta["content"].isString()) {
                                currentAssistantContent.append(delta["content"].toString());
                            }
                        }
                    }
                }
            }
        } else {
            replyBuffer.append(line + "\n");
        }
    }

    // Обновляем текст временного QLabel с накопленным содержимым
    if (aiResponseLabel && !currentAssistantContent.isEmpty()) {
        QString currentText = aiResponseLabel->text();
        // Убираем "AI печатает..." если есть, чтобы не дублировать
        if (currentText == "AI печатает...") {
            currentText.clear();
        }
        addChatMessage("AI", currentText + currentAssistantContent, false, QString(), aiResponseLabel);
    }
}


void MainWindow::onStreamFinished() {
    if (!currentReply) return;

    QString finalFullResponse = replyBuffer; // Сохраняем все, что осталось в буфере

    if (currentReply->error() != QNetworkReply::NoError) {
        qDebug() << "Network Error (Stream):" << currentReply->errorString();
        // Если была ошибка, отображаем её вместо ответа AI
        if (aiResponseLabel) {
            aiResponseLabel->setText("Ошибка: " + currentReply->errorString());
        } else {
            addChatMessage("AI", "Ошибка: " + currentReply->errorString());
        }
    } else {
        // проверка что aiResponseLabel содержит весь финальный текст
        if (aiResponseLabel && aiResponseLabel->text() == "AI печатает...") {
            aiResponseLabel->setText(finalFullResponse);
        }
        else if (aiResponseLabel) {
            // Если aiResponseLabel уже имеет какой-то текст, убедимся, что finalFullResponse добавится
            aiResponseLabel->setText(aiResponseLabel->text() + finalFullResponse);
        } else {
            addChatMessage("AI", finalFullResponse);
        }
        finalFullResponse = aiResponseLabel ? aiResponseLabel->text() : finalFullResponse; // Берем окончательный текст из QLabel

        if (aiResponseLabel) {
            // Удаляем родительский контейнер, чтобы удалить и AI Label
            if (aiResponseLabel->parentWidget()) {
                chatContentLayout->removeWidget(aiResponseLabel->parentWidget());
                aiResponseLabel->parentWidget()->deleteLater();
            }
            aiResponseLabel = nullptr;
        }


        // Парсим и отображаем окончательный ответ
        QRegularExpression codeBlockRegex("^```(\\w+)?\\s*$(.*)^```\\s*$", QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);

        int lastIndex = 0;
        QRegularExpressionMatchIterator i = codeBlockRegex.globalMatch(finalFullResponse);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            // Добавляем обычный текст перед блоком кода
            if (match.capturedStart() > lastIndex) {
                QString plainText = finalFullResponse.mid(lastIndex, match.capturedStart() - lastIndex).trimmed();
                if (!plainText.isEmpty()) {
                    addChatMessage("AI", plainText);
                }
            }

            // Добавляем блок кода
            QString language = match.captured(1); // Язык, если указан (group 1)
            QString codeContent = match.captured(2); // Содержимое кода (group 2)
            addChatMessage("AI", codeContent, true, language);

            lastIndex = match.capturedEnd();
        }

        // Добавляем оставшийся обычный текст после последнего блока кода
        if (lastIndex < finalFullResponse.length()) {
            QString plainText = finalFullResponse.mid(lastIndex).trimmed();
            if (!plainText.isEmpty()) {
                addChatMessage("AI", plainText);
            }
        }
    }

    currentReply->deleteLater();
    currentReply = nullptr;
    replyBuffer.clear();
    firstChunkReceived = false; // Сброс флага
}

void MainWindow::showModelInfo() {
    QString selectedModelId = modelSelector->currentText();
    if (availableModels.contains(selectedModelId)) {
        ModelData data = availableModels[selectedModelId];
        QToolTip::showText(QCursor::pos(), data.description, modelSelector);
    }
}

void MainWindow::openSettings() {
    SettingsDialog settingsDialog(currentChatFont, currentChatFontSize, currentTheme, this);
    if (settingsDialog.exec() == QDialog::Accepted) {
        currentChatFont = settingsDialog.selectedFont();
        currentChatFontSize = settingsDialog.selectedFontSize();
        currentTheme = settingsDialog.selectedTheme();
        applyTheme(currentTheme);
    }
}

void MainWindow::startNewChat() {
    while (QLayoutItem *item = chatContentLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    // После очистки, снова добавляем растяжку
    chatContentLayout->addStretch();

    replyBuffer.clear();
    firstChunkReceived = false;
    if (aiResponseLabel) { // Убедимся, что временный лейбл тоже очищен
        if (aiResponseLabel->parentWidget()) {
            chatContentLayout->removeWidget(aiResponseLabel->parentWidget());
            aiResponseLabel->parentWidget()->deleteLater();
        }
        aiResponseLabel = nullptr;
    }
    qDebug() << "New chat started.";
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    return QMainWindow::eventFilter(obj, event);
}
