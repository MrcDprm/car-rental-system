#include "ui/FleetPage.h"

#include "app/Theme.h"
#include "data/VehicleRepository.h"
#include "services/CsvExport.h"
#include "services/PhotoStore.h"
#include "ui/CarArt.h"
#include "ui/MaintenanceDialog.h"
#include "ui/UiHelpers.h"
#include "ui/VehicleDialog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

constexpr int SERVICE_WARNING_KM = 1000;
constexpr int MILEAGE_COLUMN = 9;
const QSize THUMBNAIL(72, 32);

QStringList headers()
{
    return {I18n::t("plate"),     I18n::t("vehicle"),      I18n::t("year"),     I18n::t("class"),
            I18n::t("body_type"), I18n::t("color"),        I18n::t("transmission"), I18n::t("fuel"),
            I18n::t("daily_price"), I18n::t("mileage"),    I18n::t("status")};
}

QStringList vehicleCells(const Vehicle &v)
{
    return {v.plate, v.brand + " " + v.model, QString::number(v.year), I18n::vehicleClass(v.vehicleClass),
            I18n::bodyType(v.bodyType), I18n::color(v.color), I18n::transmission(v.transmission),
            I18n::fuel(v.fuel), I18n::money(v.dailyPrice), I18n::number(v.mileage), I18n::vehicleStatus(v.status)};
}

} // namespace

FleetPage::FleetPage(Database &db, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    auto *title = new QLabel(I18n::t("fleet"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    auto *toolbar = new QHBoxLayout;
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(I18n::t("search"));
    m_search->setClearButtonEnabled(true);
    m_filter = new QComboBox(this);
    m_filter->addItem(I18n::t("all"));
    for (int i = 0; i <= static_cast<int>(VehicleStatus::Maintenance); ++i)
        m_filter->addItem(I18n::vehicleStatus(static_cast<VehicleStatus>(i)));
    auto *add = Ui::accentButton(I18n::t("add"), this);
    m_edit = new QPushButton(I18n::t("edit"), this);
    m_delete = new QPushButton(I18n::t("delete"), this);
    m_maintenance = new QPushButton(I18n::t("start_maintenance"), this);
    auto *csv = new QPushButton(I18n::t("export_csv"), this);
    toolbar->addWidget(m_search, 1);
    toolbar->addWidget(m_filter);
    for (QPushButton *button : {add, m_edit, m_delete, m_maintenance, csv})
        toolbar->addWidget(button);
    layout->addLayout(toolbar);

    m_table = Ui::makeTable(headers(), this);
    m_table->setIconSize(THUMBNAIL);
    m_table->verticalHeader()->setDefaultSectionSize(THUMBNAIL.height() + 10);
    layout->addWidget(m_table, 1);

    connect(m_search, &QLineEdit::textChanged, this, &FleetPage::refresh);
    connect(m_filter, &QComboBox::currentIndexChanged, this, &FleetPage::refresh);
    connect(add, &QPushButton::clicked, this, &FleetPage::addVehicle);
    connect(m_edit, &QPushButton::clicked, this, &FleetPage::editVehicle);
    connect(m_delete, &QPushButton::clicked, this, &FleetPage::deleteVehicle);
    connect(m_maintenance, &QPushButton::clicked, this, &FleetPage::maintenance);
    connect(csv, &QPushButton::clicked, this, &FleetPage::exportCsv);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &FleetPage::updateButtons);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &FleetPage::editVehicle);
}

void FleetPage::refresh()
{
    m_vehicles = VehicleRepository(m_db).all();
    const QString search = m_search->text().trimmed();
    const int filter = m_filter->currentIndex() - 1; // -1 = tümü

    m_table->setRowCount(0);
    for (const Vehicle &v : m_vehicles) {
        if (filter >= 0 && static_cast<int>(v.status) != filter)
            continue;
        const QStringList cells = vehicleCells(v);
        if (!search.isEmpty() && !cells.join(' ').contains(search, Qt::CaseInsensitive))
            continue;
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        // Bakımı yaklaşan araç kırmızı, bakımdaki soluk görünür
        QColor color;
        if (v.status == VehicleStatus::Maintenance)
            color = Theme::muted();
        else if (v.nextServiceKm - v.mileage < SERVICE_WARNING_KM)
            color = Theme::danger();
        Ui::setRow(m_table, row, cells, v.id, color);
        m_table->item(row, 0)->setIcon(QIcon(CarArt::image(v, THUMBNAIL)));
        // Fareyle üzerine gelince donanım listesi görünür
        m_table->item(row, 1)->setToolTip(I18n::features(v.features).join(", "));
        if (color == Theme::danger())
            m_table->item(row, MILEAGE_COLUMN)->setToolTip(I18n::t("service_warning"));
    }
    m_table->resizeColumnsToContents();
    updateButtons();
}

void FleetPage::updateButtons()
{
    const qint64 id = Ui::selectedId(m_table);
    const auto it = std::find_if(m_vehicles.cbegin(), m_vehicles.cend(), [id](const Vehicle &v) { return v.id == id; });
    const bool selected = it != m_vehicles.cend();
    m_edit->setEnabled(selected);
    m_delete->setEnabled(selected);
    // Kirada olan araç bakıma alınamaz; bakımdaki araç için düğme "bakımdan çıkar" olur
    m_maintenance->setEnabled(selected && it->status != VehicleStatus::Rented);
    m_maintenance->setText(I18n::t(selected && it->status == VehicleStatus::Maintenance ? "finish_maintenance"
                                                                                           : "start_maintenance"));
}

void FleetPage::addVehicle()
{
    if (VehicleDialog(m_db, Vehicle{}, this).exec() == QDialog::Accepted)
        refresh();
}

void FleetPage::editVehicle()
{
    const auto vehicle = VehicleRepository(m_db).find(Ui::selectedId(m_table));
    if (vehicle && VehicleDialog(m_db, *vehicle, this).exec() == QDialog::Accepted)
        refresh();
}

void FleetPage::deleteVehicle()
{
    const auto vehicle = VehicleRepository(m_db).find(Ui::selectedId(m_table));
    if (!vehicle || !Ui::ask(this, I18n::t("confirm_delete")))
        return;
    if (Ui::showResult(this, VehicleRepository(m_db).remove(vehicle->id))) {
        PhotoStore::remove(vehicle->photo); // silinen aracın fotoğrafı da gider
        refresh();
    }
}

void FleetPage::maintenance()
{
    const auto vehicle = VehicleRepository(m_db).find(Ui::selectedId(m_table));
    if (vehicle && MaintenanceDialog(m_db, *vehicle, this).exec() == QDialog::Accepted)
        refresh();
}

void FleetPage::exportCsv()
{
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, I18n::t("export_csv"), folder + "/fleet.csv", "CSV (*.csv)");
    if (path.isEmpty())
        return;
    QList<QStringList> rows;
    for (const Vehicle &v : m_vehicles)
        rows << vehicleCells(v) + QStringList{QString::number(v.seats), QString::number(v.luggage),
                                              I18n::features(v.features).join(", ")};
    const QStringList header = headers() + QStringList{I18n::t("seats"), I18n::t("luggage"), I18n::t("features")};
    if (CsvExport::write(path, header, rows, CsvExport::separatorFor(I18n::language())))
        Ui::inform(this, I18n::t("saved_csv").replace("{0}", QString::number(rows.size())));
    else
        Ui::warn(this, I18n::t("csv_failed"));
}
