#include "ui/DashboardPage.h"

#include "app/Theme.h"
#include "data/CustomerRepository.h"
#include "data/RentalRepository.h"
#include "data/ReportRepository.h"
#include "data/VehicleRepository.h"
#include "ui/CarArt.h"
#include "ui/MaintenanceDialog.h"
#include "ui/RentalActions.h"
#include "ui/RentalDialog.h"
#include "ui/UiHelpers.h"
#include "ui/VehicleDialog.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

const char *const CARD_KEYS[] = {"vehicles_total", "available_now", "rented_now", "in_maintenance",
                                 "pickups_today",  "returns_today", "overdue",    "service_due"};
constexpr int SERVICE_WARNING_KM = 1000;
const QSize THUMBNAIL(72, 32);

} // namespace

DashboardPage::DashboardPage(Database &db, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    auto *title = new QLabel(I18n::t("dashboard"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    // Kartlar tıklanabilir düğmelerdir; aynı anda en fazla biri seçili olur
    m_cards = new QButtonGroup(this);
    m_cards->setExclusive(false); // seçili karta tekrar tıklayınca seçim kalkabilsin
    auto *grid = new QGridLayout;
    grid->setSpacing(12);
    for (int i = 0; i < 8; ++i) {
        auto *card = new QPushButton(this);
        card->setObjectName("card");
        card->setCheckable(true);
        card->setCursor(Qt::PointingHandCursor);
        card->setMinimumHeight(92);
        auto *cardLayout = new QVBoxLayout(card);
        auto *value = new QLabel("0", card);
        value->setObjectName("cardValue");
        auto *label = new QLabel(I18n::t(CARD_KEYS[i]), card);
        label->setObjectName("cardLabel");
        // Tıklama etiketlere değil düğmeye gitsin
        value->setAttribute(Qt::WA_TransparentForMouseEvents);
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        cardLayout->addWidget(value);
        cardLayout->addWidget(label);
        grid->addWidget(card, i / 4, i % 4);
        m_cards->addButton(card, i);
        m_values << value;
    }
    layout->addLayout(grid);

    auto *header = new QHBoxLayout;
    m_listTitle = new QLabel(this);
    m_listTitle->setStyleSheet("font-size: 12pt; font-weight: 600; margin-top: 12px;");
    m_rent = Ui::accentButton(I18n::t("rent_this"), this);
    m_edit = new QPushButton(I18n::t("edit"), this);
    m_maintenance = new QPushButton(I18n::t("start_maintenance"), this);
    m_pickUp = Ui::accentButton(I18n::t("pick_up"), this);
    m_giveBack = Ui::accentButton(I18n::t("give_back"), this);
    m_print = new QPushButton(I18n::t("print_contract"), this);
    header->addWidget(m_listTitle);
    header->addStretch();
    for (QPushButton *button : {m_rent, m_edit, m_maintenance, m_pickUp, m_giveBack, m_print})
        header->addWidget(button);
    layout->addLayout(header);

    m_table = Ui::makeTable({}, this);
    m_table->setIconSize(THUMBNAIL);
    m_table->verticalHeader()->setDefaultSectionSize(THUMBNAIL.height() + 10);
    layout->addWidget(m_table, 1);
    m_empty = new QLabel(this);
    m_empty->setObjectName("muted");
    layout->addWidget(m_empty);

    connect(m_cards, &QButtonGroup::idClicked, this, &DashboardPage::selectCard);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &DashboardPage::updateButtons);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this] {
        if (showsVehicles())
            editVehicle();
    });
    connect(m_rent, &QPushButton::clicked, this, &DashboardPage::rent);
    connect(m_edit, &QPushButton::clicked, this, &DashboardPage::editVehicle);
    connect(m_maintenance, &QPushButton::clicked, this, &DashboardPage::maintenance);
    connect(m_pickUp, &QPushButton::clicked, this, &DashboardPage::pickUp);
    connect(m_giveBack, &QPushButton::clicked, this, &DashboardPage::giveBack);
    connect(m_print, &QPushButton::clicked, this, &DashboardPage::printContract);
}

void DashboardPage::selectCard(int index)
{
    // Seçili karta tekrar tıklamak listeyi "Yapılacaklar"a döndürür
    const View clicked = static_cast<View>(index + 1);
    m_view = m_view == clicked ? View::Todo : clicked;
    for (QAbstractButton *card : m_cards->buttons())
        card->setChecked(m_cards->id(card) + 1 == static_cast<int>(m_view));
    refresh();
}

bool DashboardPage::showsVehicles() const
{
    return m_view == View::AllVehicles || m_view == View::Available || m_view == View::Maintenance
           || m_view == View::ServiceDue;
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

    m_vehicles = VehicleRepository(m_db).all();
    m_rentals = RentalRepository(m_db).all();
    m_listTitle->setText(m_view == View::Todo ? I18n::t("todo") : I18n::t(CARD_KEYS[static_cast<int>(m_view) - 1]));
    if (showsVehicles())
        fillVehicles();
    else
        fillRentals();
    m_table->resizeColumnsToContents();
    m_empty->setText(I18n::t(m_view == View::Todo ? "nothing_todo" : "nothing_here"));
    m_empty->setVisible(m_table->rowCount() == 0);
    updateButtons();
}

void DashboardPage::fillVehicles()
{
    m_table->clear();
    m_table->setRowCount(0);
    const QStringList headers = {I18n::t("plate"), I18n::t("vehicle"), I18n::t("class"), I18n::t("daily_price"),
                                 I18n::t("mileage"), I18n::t("next_service"), I18n::t("status")};
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    for (const Vehicle &v : m_vehicles) {
        const bool serviceDue = v.nextServiceKm - v.mileage < SERVICE_WARNING_KM;
        if ((m_view == View::Available && v.status != VehicleStatus::Available)
            || (m_view == View::Maintenance && v.status != VehicleStatus::Maintenance)
            || (m_view == View::ServiceDue && (!serviceDue || v.status == VehicleStatus::Maintenance)))
            continue;
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        Ui::setRow(m_table, row,
                   {v.plate, v.brand + " " + v.model, I18n::vehicleClass(v.vehicleClass), I18n::money(v.dailyPrice),
                    I18n::number(v.mileage), I18n::number(v.nextServiceKm), I18n::vehicleStatus(v.status)},
                   v.id, serviceDue && v.status != VehicleStatus::Maintenance ? Theme::danger() : QColor());
        m_table->item(row, 0)->setIcon(QIcon(CarArt::image(v, THUMBNAIL)));
    }
}

void DashboardPage::fillRentals()
{
    m_table->clear();
    m_table->setRowCount(0);
    const QStringList headers = {I18n::t("status"), I18n::t("customer"), I18n::t("vehicle"), I18n::t("start_date"),
                                 I18n::t("end_date"), I18n::t("total")};
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);

    QHash<qint64, Vehicle> vehicles;
    for (const Vehicle &v : m_vehicles)
        vehicles.insert(v.id, v);
    QHash<qint64, QString> customers;
    for (const Customer &c : CustomerRepository(m_db).all())
        customers.insert(c.id, c.fullName);

    const QDate today = QDate::currentDate();
    for (const Rental &r : m_rentals) {
        // Teslim günü gelmiş rezervasyonlar, bugün dönecekler ve gecikenler
        const bool pickUp = r.status == RentalStatus::Reserved && r.startDate <= today;
        const bool active = r.status == RentalStatus::Active;
        const bool overdue = active && r.endDate < today;
        const bool returnsToday = active && r.endDate == today;
        const bool show = m_view == View::Todo           ? pickUp || overdue || returnsToday
                          : m_view == View::Rented       ? active
                          : m_view == View::PickUps      ? pickUp
                          : m_view == View::ReturnsToday ? returnsToday
                                                         : overdue;
        if (!show)
            continue;
        const QString state = pickUp ? I18n::t("todo_pickup") : overdue ? I18n::t("todo_overdue")
                              : returnsToday ? I18n::t("todo_return") : I18n::rentalStatus(r.status);
        const Vehicle vehicle = vehicles.value(r.vehicleId);
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        Ui::setRow(m_table, row,
                   {state, customers.value(r.customerId, "—"),
                    vehicle.plate + " · " + vehicle.brand + " " + vehicle.model, I18n::date(r.startDate),
                    I18n::date(r.endDate), I18n::money(r.total)},
                   r.id, overdue ? Theme::danger() : QColor());
        m_table->item(row, 2)->setIcon(QIcon(CarArt::image(vehicle, THUMBNAIL)));
    }
}

const Vehicle *DashboardPage::selectedVehicle() const
{
    if (!showsVehicles())
        return nullptr;
    const qint64 id = Ui::selectedId(m_table);
    for (const Vehicle &v : m_vehicles)
        if (v.id == id)
            return &v;
    return nullptr;
}

const Rental *DashboardPage::selectedRental() const
{
    if (showsVehicles())
        return nullptr;
    const qint64 id = Ui::selectedId(m_table);
    for (const Rental &r : m_rentals)
        if (r.id == id)
            return &r;
    return nullptr;
}

void DashboardPage::updateButtons()
{
    // Sadece bu listeye uyan düğmeler görünür (ör. "Kirada" listesinde "Teslim et" yoktur);
    // görünenler de seçili kaydın durumuna göre açılır
    const bool vehicles = showsVehicles();
    const bool todo = m_view == View::Todo;
    m_rent->setVisible(vehicles && m_view != View::Maintenance);
    m_edit->setVisible(vehicles);
    m_maintenance->setVisible(vehicles);
    m_pickUp->setVisible(todo || m_view == View::PickUps);
    m_giveBack->setVisible(todo || m_view == View::Rented || m_view == View::ReturnsToday || m_view == View::Overdue);
    m_print->setVisible(!vehicles);

    const Vehicle *v = selectedVehicle();
    m_rent->setEnabled(v && v->status == VehicleStatus::Available);
    m_edit->setEnabled(v);
    m_maintenance->setEnabled(v && v->status != VehicleStatus::Rented);
    m_maintenance->setText(I18n::t(v && v->status == VehicleStatus::Maintenance ? "finish_maintenance"
                                                                                  : "start_maintenance"));
    const Rental *r = selectedRental();
    m_pickUp->setEnabled(r && r->status == RentalStatus::Reserved && r->startDate <= QDate::currentDate());
    m_giveBack->setEnabled(r && r->status == RentalStatus::Active);
    m_print->setEnabled(r && r->status != RentalStatus::Reserved && r->status != RentalStatus::Cancelled);
}

void DashboardPage::rent()
{
    if (const Vehicle *v = selectedVehicle(); v && RentalDialog(m_db, this, v->id).exec() == QDialog::Accepted)
        refresh();
}

void DashboardPage::editVehicle()
{
    if (const Vehicle *v = selectedVehicle(); v && VehicleDialog(m_db, *v, this).exec() == QDialog::Accepted)
        refresh();
}

void DashboardPage::maintenance()
{
    if (const Vehicle *v = selectedVehicle(); v && MaintenanceDialog(m_db, *v, this).exec() == QDialog::Accepted)
        refresh();
}

void DashboardPage::pickUp()
{
    if (const Rental *r = selectedRental(); r && RentalActions::pickUp(m_db, *r, this))
        refresh();
}

void DashboardPage::giveBack()
{
    if (const Rental *r = selectedRental(); r && RentalActions::giveBack(m_db, *r, this))
        refresh();
}

void DashboardPage::printContract()
{
    if (const Rental *r = selectedRental())
        RentalActions::printContract(m_db, *r, this);
}
