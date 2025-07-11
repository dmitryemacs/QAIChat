#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QFont>
#include <QFontComboBox>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    // Конструктор принимает текущие настройки, чтобы инициализировать поля
    explicit SettingsDialog(const QFont &currentFont, int currentFontSize, const QString &currentTheme, QWidget *parent = nullptr);
    ~SettingsDialog();

    // Методы для получения выбранных настроек
    QFont selectedFont() const;
    int selectedFontSize() const;
    QString selectedTheme() const;

private:
    QFontComboBox *fontComboBox;
    QSpinBox *fontSizeSpinBox;
    QComboBox *themeComboBox;
    QDialogButtonBox *buttonBox;
};

#endif // SETTINGSDIALOG_H
