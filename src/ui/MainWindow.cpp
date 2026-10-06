#include "ui/MainWindow.h"

#include "app/I18n.h"
#include "app/Settings.h"
#include "app/Theme.h"
#include "ui/AboutDialog.h"
#include "ui/CustomersPage.h"
#include "ui/DashboardPage.h"
#include "ui/FleetPage.h"
#include "ui/RentalsPage.h"
#include "ui/ReportsPage.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(Database &db, Settings &settings, const QString &dataFolder)
    : m_db(db), m_settings(settings), m_dataFolder(dataFolder)
{
    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sidebar = new QWidget(central);
    sidebar->setFixedWidth(200);
    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(10, 16, 10, 12);
    m_navigation = new QListWidget(sidebar);
    m_navigation->setObjectName("navigation");
    m_navigation->setFocusPolicy(Qt::NoFocus);
    side->addWidget(m_navigation, 1);
    for (QPushButton **button : {&m_themeButton, &m_languageButton, &m_aboutButton}) {
        *button = new QPushButton(sidebar);
        (*button)->setFlat(true);
        side->addWidget(*button);
    }

    m_pages = new QStackedWidget(central);
    layout->addWidget(sidebar);
    layout->addWidget(m_pages, 1);
    setCentralWidget(central);

    connect(m_navigation, &QListWidget::currentRowChanged, this, &MainWindow::showPage);
    connect(m_themeButton, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(m_languageButton, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(m_aboutButton, &QPushButton::clicked, this, [this] { AboutDialog(m_dataFolder, this).exec(); });

    buildPages();
    resize(1180, 720);
}

void MainWindow::buildPages()
{
    // Sayfalar dile göre metinlerle kurulduğu için dil değişince baştan oluşturulur
    const int current = qMax(0, m_navigation->currentRow());
    while (m_pages->count() > 0) {
        QWidget *page = m_pages->widget(0);
        m_pages->removeWidget(page);
        page->deleteLater();
    }
    m_pages->addWidget(new DashboardPage(m_db, m_pages));
    m_pages->addWidget(new FleetPage(m_db, m_pages));
    m_pages->addWidget(new CustomersPage(m_db, m_pages));
    m_pages->addWidget(new RentalsPage(m_db, m_pages));
    m_pages->addWidget(new ReportsPage(m_db, m_pages));

    m_navigation->blockSignals(true); // liste yeniden doldurulurken sayfa değişimi tetiklenmesin
    m_navigation->clear();
    m_navigation->addItems({"🏠  " + I18n::t("dashboard"), "🚗  " + I18n::t("fleet"), "👤  " + I18n::t("customers"),
                            "🔑  " + I18n::t("rentals"), "📊  " + I18n::t("reports")});
    m_navigation->blockSignals(false);
    m_themeButton->setText((Theme::isDark() ? "☀  " : "☾  ") + I18n::t("theme"));
    m_languageButton->setText("🌐  " + I18n::t("language"));
    m_aboutButton->setText("ⓘ  " + I18n::t("about"));
    setWindowTitle(I18n::t("app_name"));
    m_navigation->setCurrentRow(current);
    showPage(current);
}

void MainWindow::showPage(int index)
{
    if (index < 0)
        return;
    m_pages->setCurrentIndex(index);
    // Her sayfa açıldığında veriyi yeniden okur: başka sayfada yapılan değişiklik hemen görünür
    QMetaObject::invokeMethod(m_pages->currentWidget(), "refresh");
}

void MainWindow::toggleTheme()
{
    const bool dark = !Theme::isDark();
    m_settings.setDarkTheme(dark);
    Theme::apply(*qApp, dark);
    buildPages(); // grafik ve renkli hücreler yeni renklerle çizilsin
}

void MainWindow::toggleLanguage()
{
    const QString language = I18n::language() == "tr" ? "en" : "tr";
    m_settings.setLanguage(language);
    I18n::setLanguage(language);
    buildPages();
}