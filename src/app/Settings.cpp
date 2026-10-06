#include "app/Settings.h"

#include <QSettings>

Settings::Settings(const QString &dataFolder)
    : m_path(dataFolder + "/settings.ini")
{
}

QString Settings::language() const
{
    const QString value = QSettings(m_path, QSettings::IniFormat).value("language", "tr").toString();
    return value == "en" ? "en" : "tr";
}

void Settings::setLanguage(const QString &language)
{
    QSettings(m_path, QSettings::IniFormat).setValue("language", language == "en" ? "en" : "tr");
}

bool Settings::darkTheme() const
{
    return QSettings(m_path, QSettings::IniFormat).value("theme", "dark").toString() != "light";
}

void Settings::setDarkTheme(bool dark)
{
    QSettings(m_path, QSettings::IniFormat).setValue("theme", dark ? "dark" : "light");
}

bool Settings::demoOffered() const
{
    return QSettings(m_path, QSettings::IniFormat).value("demo_offered", false).toBool();
}

void Settings::setDemoOffered(bool offered)
{
    QSettings(m_path, QSettings::IniFormat).setValue("demo_offered", offered);
}
