#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Настройка UI
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true); // Пользователь не может редактировать историю чата
    mainLayout->addWidget(chatDisplay);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Введите ваше сообщение...");
    inputLayout->addWidget(messageInput);

    sendButton = new QPushButton("Отправить", this);
    inputLayout->addWidget(sendButton);

    mainLayout->addLayout(inputLayout);

    // Инициализация QNetworkAccessManager
    networkManager = new QNetworkAccessManager(this);

    // Соединение сигналов и слотов
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::sendMessage); // Отправка по Enter
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    setWindowTitle("OpenRouter AI Chat");
    resize(600, 400);
}

MainWindow::~MainWindow()
{
    // Деструктор автоматически удалит дочерние объекты, если они имеют родителя.
}

void MainWindow::sendMessage()
{
    QString userMessage = messageInput->text().trimmed();
    if (userMessage.isEmpty()) {
        return;
    }

    // Отображаем сообщение пользователя в чате
    chatDisplay->append("<b>Вы:</b> " + userMessage);
    messageInput->clear(); // Очищаем поле ввода

    // Создаем JSON-объект для запроса к API
    QJsonObject requestBody;
    requestBody["model"] = "deepseek/deepseek-r1-0528:free";

    QJsonArray messagesArray;
    // Для простоты примера, отправляем только текущее сообщение
    QJsonObject userMessageObj;
    userMessageObj["role"] = "user";
    userMessageObj["content"] = userMessage;
    messagesArray.append(userMessageObj);

    requestBody["messages"] = messagesArray;

    QJsonDocument doc(requestBody);
    QByteArray postData = doc.toJson(QJsonDocument::Compact); // Используем Compact для более плотного JSON

    // Создаем сетевой запрос
    // ИСПРАВЛЕНИЕ: Используем фигурные скобки для инициализации QNetworkRequest
    // Это предотвращает "most vexing parse"
    QNetworkRequest request{QUrl(OPENROUTER_API_URL)};

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + OPENROUTER_API_KEY).toUtf8());

    // Замените "https://your-app-domain.com" на URL вашего сайта/приложения
    // Это необязательные заголовки для OpenRouter, но рекомендуется их указывать.
    request.setRawHeader("HTTP-Referer", "https://your-app-domain.com");
    // Замените "Your App Name" на название вашего приложения
    request.setRawHeader("X-Title", "AIChat");

    // Отправляем POST-запрос
    networkManager->post(request, postData);

    // Можно добавить индикатор загрузки
    chatDisplay->append("<i>AI печатает...</i>");
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    // Удаляем индикатор загрузки
    QString htmlContent = chatDisplay->toHtml();
    htmlContent.replace("<i>AI печатает...</i>", "");
    chatDisplay->setHtml(htmlContent);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isObject()) {
            QJsonObject jsonObject = jsonDoc.object();
            if (jsonObject.contains("choices") && jsonObject["choices"].isArray()) {
                QJsonArray choices = jsonObject["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject firstChoice = choices.first().toObject();
                    if (firstChoice.contains("message") && firstChoice["message"].isObject()) {
                        QJsonObject message = firstChoice["message"].toObject();
                        if (message.contains("content") && message["content"].isString()) {
                            QString aiResponse = message["content"].toString();
                            chatDisplay->append("<b>AI:</b> " + aiResponse);
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
        chatDisplay->append(QString("<b style='color: red;'>Ошибка сети:</b> %1").arg(reply->errorString()));
        qDebug() << "Network Error:" << reply->errorString();
        qDebug() << "Response Data:" << reply->readAll(); // Для дополнительной отладки
    }

    reply->deleteLater(); // Важно: удалить объект QNetworkReply после использования
}
