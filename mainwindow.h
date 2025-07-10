#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QDebug> // Для отладки

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendMessage();
    void onNetworkReply(QNetworkReply *reply);

private:
    QTextEdit *chatDisplay;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QNetworkAccessManager *networkManager;

    // ВНИМАНИЕ: API_KEY должен храниться безопасно в реальном приложении.
    // Для примера он здесь. Замените на ваш реальный ключ OpenRouter.
    const QString OPENROUTER_API_KEY = "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";
    const QString OPENROUTER_API_URL = "https://openrouter.ai/api/v1/chat/completions";
};
#endif // MAINWINDOW_H
