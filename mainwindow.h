// mainwindow.h
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
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QDebug>
#include <QTimer>
#include <QModelIndex>
#include <QListView>
#include <QToolTip>
#include <QEvent>
#include <QHoverEvent>
#include <QCursor>
#include <QTextOption>
#include <QSettings>
#include <QAction>
#include <QFont>
#include <QClipboard>
#include <QScrollArea>
#include <QLabel>

#include "SyntaxHighlighter.h"
#include "settingsdialog.h"
#include "codeblockwidget.h"

struct ModelData {
    QString id;
    QString description;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendMessage();
    void showModelInfo();
    void openSettings();
    void startNewChat();

    void onStreamReadyRead();
    void onStreamFinished();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void loadSettings();
    void saveSettings();
    void applyTheme(const QString &theme);

    QString getSystemInfo();
    QString systemInfoContext;

    QVBoxLayout *chatContentLayout;
    QScrollArea *chatScrollArea;

    QLineEdit *messageInput;
    QPushButton *sendButton;
    QComboBox *modelSelector;
    QNetworkAccessManager *networkManager;

    QMap<QString, ModelData> availableModels;

    QTimer *hoverTimer;
    QModelIndex lastHoveredIndex;

    QAction *settingsAction;
    QAction *newChatAction;

    QFont currentChatFont;
    int currentChatFontSize;
    QString currentTheme;

    QNetworkReply *currentReply = nullptr;
    QString replyBuffer;
    bool firstChunkReceived = false;

    const QString OPENROUTER_API_KEY = "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"; // Замените на ваш API-ключ OpenRouter
    const QString OPENROUTER_BASE_URL = "https://openrouter.ai/api/v1";
    const QString OPENROUTER_CHAT_COMPLETIONS_URL = OPENROUTER_BASE_URL + "/chat/completions";

    // ИЗМЕНЕНО: Добавляем новый указатель на QLabel для временного отображения ответа AI
    // Это будет тот QLabel, который будет обновляться в real-time
    QLabel *aiResponseLabel = nullptr;
    // ИЗМЕНЕНО: Обновленный addChatMessage для поддержки временного сообщения
    void addChatMessage(const QString &sender, const QString &text, bool isCode = false, const QString &language = QString(), QLabel *targetLabel = nullptr);
};

#endif // MAINWINDOW_H
