#include "ui/ReportsPage.h"

#include "data/ReportRepository.h"
#include "ui/RevenueChart.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <numeric>

namespace {

constexpr int TOP_LIMIT = 10;

} // namespace

ReportsPage::ReportsPage(Database &db, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    auto *title = new QLabel(I18n::t("reports"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    auto *header = new QHBoxLayout;
    auto *chartTitle = new QLabel(I18n::t("monthly_revenue"), this);
    chartTitle->setStyleSheet("font-size: 12pt; font-weight: 600;");
    m_year = new QComboBox(this);
    m_total = new QLabel(this);
    m_total->setObjectName("muted");
    header->addWidget(chartTitle);
    header->addWidget(m_year);
    header->addStretch();
    header->addWidget(m_total);
    layout->addLayout(header);

    m_chart = new RevenueChart(this);
    layout->addWidget(m_chart, 1);
    m_empty = new QLabel(I18n::t("no_data"), this);
    m_empty->setObjectName("muted");
    layout->addWidget(m_empty);

    auto *topTitle = new QLabel(I18n::t("top_vehicles"), this);
    topTitle->setStyleSheet("font-size: 12pt; font-weight: 600; margin-top: 12px;");
    layout->addWidget(topTitle);
    m_top = Ui::makeTable({I18n::t("plate"), I18n::t("vehicle"), I18n::t("rental_count"), I18n::t("revenue")}, this);
    layout->addWidget(m_top, 1);

    connect(m_year, &QComboBox::currentIndexChanged, this, &ReportsPage::showYear);
}

void ReportsPage::refresh()
{
    ReportRepository reports(m_db);
    // Yıl listesi yenilenirken seçim korunur; içinde iade olmayan yıl için bu yıl gösterilir
    const int selectedYear = m_year->currentData().isValid() ? m_year->currentData().toInt() : QDate::currentDate().year();
    QList<int> years = reports.years();
    if (!years.contains(QDate::currentDate().year()))
        years.prepend(QDate::currentDate().year());
    m_year->blockSignals(true);
    m_year->clear();
    for (int year : years)
        m_year->addItem(QString::number(year), year);
    m_year->setCurrentIndex(qMax(0, m_year->findData(selectedYear)));
    m_year->blockSignals(false);
    showYear();

    const QList<VehicleStat> top = reports.topVehicles(TOP_LIMIT);
    m_top->setRowCount(0);
    for (const VehicleStat &stat : top) {
        const int row = m_top->rowCount();
        m_top->insertRow(row);
        Ui::setRow(m_top, row, {stat.plate, stat.name, QString::number(stat.rentals), I18n::money(stat.revenue)}, row);
    }
    m_top->resizeColumnsToContents();
}

void ReportsPage::showYear()
{
    const QList<qint64> months = ReportRepository(m_db).monthlyRevenue(m_year->currentData().toInt());
    const qint64 total = std::accumulate(months.cbegin(), months.cend(), qint64(0));
    m_chart->setValues(months);
    m_total->setText(I18n::t("year_total").replace("{0}", I18n::money(total)));
    m_empty->setVisible(total == 0);
}