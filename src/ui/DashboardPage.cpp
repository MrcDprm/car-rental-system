#include "ui/DashboardPage.h"

#include "app/Theme.h"
#include "data/CustomerRepository.h"
#include "data/RentalRepository.h"
#include "data/ReportRepository.h"
#include "data/VehicleRepository.h"
#include "ui/UiHelpers.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

const char *const CARD_KEYS[] = {"vehicles_total", "available_now", "rented_now", "in_maintenance",
                                 "pickups_today",  "returns_today", "overdue",    "service_due"};

} // namespace

DashboardPage::DashboardPage(Database &db, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    auto *title = new QLabel(I18n::t("dashboard"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    auto *grid = new QGridLayout;
    grid->setSpacing(12);
    for (int i = 0; i < 8; ++i) {
        auto *card = new QFrame(this);
        card->setObjectName("card");
        auto *cardLayout = new QVBoxLayout(card);
        auto *value = new QLabel("0", card);
        value->setObjectName("cardValue");
        auto *label = new QLabel(I18n::t(CARD_KEYS[i]), card);
        label->setObjectName("cardLabel");
        cardLayout->addWidget(value);
        cardLayout->addWidget(label);
        grid->addWidget(card, i / 4, i % 4);
        m_values << value;
    }
    layout->addLayout(grid);

    auto *todoTitle = new QLabel(I18n::t("todo"), this);
    todoTitle->setStyleSheet("font-size: 12pt; font-weight: 600; margin-top: 12px;");
    layout->addWidget(todoTitle);
    m_todo = Ui::makeTable({I18n::t("status"), I18n::t("customer"), I18n::t("vehicle"), I18n::t("start_date"),
                            I18n::t("end_date")}, this);
    layout->addWidget(m_todo, 1);
    m_empty = new QLabel(I18n::t("nothing_todo"), this);
    m_empty->setObjectName("muted");
    layout->addWidget(m_empty);
}

void DashboardPage::refresh()
{
    const QDate today = QDate::currentDate();
    const Summary s = ReportRepository(m_db).summary(today);
    const int values[] = {s.vehicles, s.available, s.rented, s.inMaintenance,
                          s.pickUpsToday, s.returnsToday, s.overdue, s.serviceDue};
    for (int i = 0; i < 8; ++i)
        m_values[i]->setText(QString::number(values[i]));
    // Geciken ve bakımı yaklaşan varsa sayı kırmızı görünür
    m_values[6]->setStyleSheet(s.overdue > 0 ? "color: " + Theme::danger().name() : "");
    m_values[7]->setStyleSheet(s.serviceDue > 0 ? "color: " + Theme::danger().name() : "");

    // Yapılacaklar: teslim günü gelmiş rezervasyonlar, bugün dönecekler ve gecikenler
    m_todo->setRowCount(0);
    VehicleRepository vehicles(m_db);
    CustomerRepository customers(m_db);
    for (const Rental &r : RentalRepository(m_db).all()) {
        QString state;
        QColor color;
        if (r.status == RentalStatus::Reserved && r.startDate <= today) {
            state = I18n::t("todo_pickup");
        } else if (r.status == RentalStatus::Active && r.endDate < today) {
            state = I18n::t("todo_overdue");
            color = Theme::danger();
        } else if (r.status == RentalStatus::Active && r.endDate == today) {
            state = I18n::t("todo_return");
        } else {
            continue;
        }
        const auto vehicle = vehicles.find(r.vehicleId);
        const auto customer = customers.find(r.customerId);
        const int row = m_todo->rowCount();
        m_todo->insertRow(row);
        Ui::setRow(m_todo, row, {state, customer ? customer->fullName : "—",
                                 vehicle ? vehicle->plate + " · " + vehicle->brand + " " + vehicle->model : "—",
                                 I18n::date(r.startDate), I18n::date(r.endDate)}, r.id, color);
    }
    m_todo->resizeColumnsToContents();
    m_empty->setVisible(m_todo->rowCount() == 0);
}