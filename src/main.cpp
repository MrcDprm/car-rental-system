#include "app/I18n.h"
#include "app/Settings.h"
#include "app/Theme.h"
#include "data/Database.h"
#include "services/DemoData.h"
#include "services/PhotoStore.h"
#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CarRental");
    app.setOrganizationName("MrcDprm");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/icon.png"));

    // Veriler program klasörüne değil kullanıcı klasörüne yazılır: %APPDATA%\MrcDprm\CarRental
    const QString dataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataFolder);

    Settings settings(dataFolder);
    I18n::setLanguage(settings.language());
    Theme::apply(app, settings.darkTheme());

    Database db;
    if (!db.open(dataFolder + "/car-rental.db")) {
        // Ayrıntılı hata kullanıcıya gösterilmez; sadece ne olduğu ve dosyanın yeri söylenir
        Ui::message(nullptr, QMessageBox::Critical,
                    I18n::t("db_open_failed").replace("{0}", QDir::toNativeSeparators(dataFolder)));
        return 1;
    }
    PhotoStore::setFolder(dataFolder + "/photos");

    // İlk açılışta veritabanı boşsa örnek filo önerilir (bir kez sorulur)
    if (!settings.demoOffered() && DemoData::isEmpty(db)) {
        settings.setDemoOffered(true);
        if (Ui::ask(nullptr, I18n::t("demo_question"), true, I18n::t("demo_title")) && !DemoData::load(db))
            Ui::warn(nullptr, I18n::t("demo_failed"));
    }

    MainWindow window(db, settings, dataFolder);
    window.show();
    return app.exec();
}
