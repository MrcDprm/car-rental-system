#pragma once

#include <QString>

// Kullanıcı ayarları (dil ve tema) kullanıcı klasöründeki settings.ini dosyasında tutulur.
// Dosyadan okunan değerlere güvenilmez: bilinmeyen değer yerine varsayılan kullanılır.
class Settings
{
public:
    explicit Settings(const QString &dataFolder);

    QString language() const;
    void setLanguage(const QString &language);
    bool darkTheme() const;
    void setDarkTheme(bool dark);
    bool demoOffered() const; // örnek veri bir kez sorulur
    void setDemoOffered(bool offered);

private:
    QString m_path;
};
