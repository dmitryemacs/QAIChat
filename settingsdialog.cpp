#include "settingsdialog.h"
#include <QLabel>
#include <QSpinBox>
#include <QFontComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox> // Добавлено, если ранее отсутствовало

SettingsDialog::SettingsDialog(const QFont &currentFont, int currentFontSize, const QString &currentTheme, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Настройки");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Выбор шрифта
    QHBoxLayout *fontLayout = new QHBoxLayout();
    fontLayout->addWidget(new QLabel("Шрифт:"));
    fontComboBox = new QFontComboBox(this);
    fontComboBox->setCurrentFont(currentFont); // Устанавливаем текущий шрифт
    fontLayout->addWidget(fontComboBox);
    mainLayout->addLayout(fontLayout);

    // Выбор размера шрифта
    QHBoxLayout *fontSizeLayout = new QHBoxLayout();
    fontSizeLayout->addWidget(new QLabel("Размер шрифта:"));
    fontSizeSpinBox = new QSpinBox(this);
    fontSizeSpinBox->setRange(8, 72); // Задаем разумный диапазон размеров
    fontSizeSpinBox->setValue(currentFontSize); // Устанавливаем текущий размер
    fontSizeLayout->addWidget(fontSizeSpinBox);
    mainLayout->addLayout(fontSizeLayout);

    // Выбор темы
    QHBoxLayout *themeLayout = new QHBoxLayout();
    themeLayout->addWidget(new QLabel("Тема:"));
    themeComboBox = new QComboBox(this);
    themeComboBox->addItem("Светлая", "light"); // Отображаемое имя, данные (ID темы)
    themeComboBox->addItem("Темная", "dark");
    // Находим и устанавливаем текущую тему
    int themeIndex = themeComboBox->findData(currentTheme);
    if (themeIndex != -1) {
        themeComboBox->setCurrentIndex(themeIndex);
    }
    themeLayout->addWidget(themeComboBox);
    mainLayout->addLayout(themeLayout);

    // Кнопки OK и Cancel
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

SettingsDialog::~SettingsDialog()
{
    // Дочерние виджеты удаляются автоматически, так как у них есть родитель.
}

QFont SettingsDialog::selectedFont() const
{
    return fontComboBox->currentFont();
}

int SettingsDialog::selectedFontSize() const
{
    return fontSizeSpinBox->value();
}

QString SettingsDialog::selectedTheme() const
{
    return themeComboBox->currentData().toString();
}
