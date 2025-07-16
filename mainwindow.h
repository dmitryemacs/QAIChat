#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply> // Включить QNetworkReply
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
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

#include "SyntaxHighlighter.h"
#include "settingsdialog.h"

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
    void copyCodeBlock();
    void onCustomContextMenuRequested(const QPoint &pos);

    // НОВЫЕ СЛОТЫ ДЛЯ СТРИМИНГА
    void onStreamReadyRead(); // Обработка входящих данных по мере их поступления
    void onStreamFinished();  // Обработка завершения стриминга

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void loadSettings();
    void saveSettings();
    void applyTheme(const QString &theme);

    QString getSystemInfo();
    QString systemInfoContext;

    QTextEdit *chatDisplay;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QComboBox *modelSelector;
    QNetworkAccessManager *networkManager;
    CodeHighlighter *codeHighlighter;

    QMap<QString, ModelData> availableModels;

    QTimer *hoverTimer;
    QModelIndex lastHoveredIndex;

    QAction *settingsAction;
    QAction *copyCodeAction;
    QAction *newChatAction;

    QFont currentChatFont;
    int currentChatFontSize;
    QString currentTheme;

    QString lastHoveredCodeBlockContent;

    QNetworkReply *currentReply = nullptr; // НОВОЕ: Указатель на текущий QNetworkReply для стриминга
    QString replyBuffer; // НОВОЕ: Буфер для накопления частичных данных SSE
    bool firstChunkReceived = false; // НОВОЕ: Флаг для первого полученного фрагмента (чтобы удалить "AI печатает...")

    const QString OPENROUTER_API_KEY = "";
    const QString OPENROUTER_API_URL = "https://openrouter.ai/api/v1/chat/completions";
};

#endif // MAINWINDOW_H
