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
#include <QSettings> // Для сохранения/загрузки настроек
#include <QAction>   // Для пункта меню "Настройки"
#include <QFont>     // Для работы со шрифтами
#include <QClipboard> // Для работы с буфером обмена (нужен для будущей кнопки копирования)


#include "SyntaxHighlighter.h"
#include "settingsdialog.h" // Включаем новый диалог настроек

// Структура для хранения данных о модели
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
    void onNetworkReply(QNetworkReply *reply);
    void showModelInfo();
    void openSettings(); // Новый слот для открытия настроек

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void loadSettings();   // Новый метод для загрузки настроек
    void saveSettings();   // Новый метод для сохранения настроек
    void applyTheme(const QString &theme); // Новый метод для применения темы

    // SYSTEM INFO
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

    QAction *settingsAction; // Действие для пункта меню "Настройки"

    // Переменные для хранения текущих настроек
    QFont currentChatFont;
    int currentChatFontSize;
    QString currentTheme;

    const QString OPENROUTER_API_KEY = "";
    const QString OPENROUTER_API_URL = "https://openrouter.ai/api/v1/chat/completions";
};

#endif // MAINWINDOW_H

