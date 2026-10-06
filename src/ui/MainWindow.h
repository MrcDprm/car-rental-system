#pragma once

#include <QMainWindow>

class Database;
class Settings;
class QListWidget;
class QStackedWidget;
class QPushButton;

// Ana pencere: solda gezinme menüsü, sağda sayfalar. Dil ya da tema değişince sayfalar yeniden kurulur.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Database &db, Settings &settings, const QString &dataFolder);

private:
    void buildPages();
    void showPage(int index);
    void toggleTheme();
    void toggleLanguage();

    Database &m_db;
    Settings &m_settings;
    QString m_dataFolder;
    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
    QPushButton *m_themeButton = nullptr;
    QPushButton *m_languageButton = nullptr;
    QPushButton *m_aboutButton = nullptr;
};