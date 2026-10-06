#include "ui/MaintenanceDialog.h"

#include "app/Theme.h"
#include "core/Money.h"
#include "data/MaintenanceRepository.h"
#include "ui/UiHelpers.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

namespace {

constexpr int SERVICE_INTERVAL_KM = 15000;

} // namespace

MaintenanceDialog::MaintenanceDialog(Database &db, const Vehicle &vehicle, QWidget *parent)
    : QDialog(parent), m_db(db), m_vehicle(vehicle), m_finishing(vehicle.status == VehicleStatus::Maintenance)
{
    setWindowTitle(I18n::t(m_finishing ? "finish_maintenance" : "start_maintenance") + " · " + vehicle.plate);
    auto *form = new QFormLayout(this);
    if (m_finishing) {
        m_cost = new QDoubleSpinBox(this);
        m_cost->setRange(0, 10'000'000);
        m_cost->setDecimals(2);
        m_cost->setGroupSeparatorShown(true);
        m_cost->setSuffix(" ₺");
        m_nextService = new QSpinBox(this);
        m_nextService->setRange(0, 2'000'000);
        m_nextService->setSuffix(" km");
        m_nextService->setValue(vehicle.mileage + SERVICE_INTERVAL_KM); // genelde 15.000 km'de bir
        form->addRow(I18n::t("maintenance_cost"), m_cost);
        form->addRow(I18n::t("next_service"), m_nextService);
    } else {
        m_description = new QLineEdit(this);
        m_description->setMaxLength(300);
        form->addRow(I18n::t("maintenance_description"), m_description);
    }

    // Bakım geçmişi
    auto *history = Ui::makeTable({I18n::t("start_date"), I18n::t("end_date"), I18n::t("maintenance_description"),
                                   I18n::t("maintenance_cost")}, this);
    const QList<Maintenance> records = MaintenanceRepository(db).forVehicle(vehicle.id);
    for (int row = 0; row < records.size(); ++row) {
        const Maintenance &m = records[row];
        history->insertRow(row);
        Ui::setRow(history, row, {I18n::date(m.startDate), I18n::date(m.endDate), m.description,
                                  m.endDate.isValid() ? I18n::money(m.cost) : "—"}, m.id);
    }
    history->resizeColumnsToContents();
    history->setMinimumHeight(160);
    form->addRow(new QLabel(I18n::t("maintenance_history"), this));
    form->addRow(history);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    form->addRow(m_error);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &MaintenanceDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(buttons);
    setMinimumWidth(520);
}

void MaintenanceDialog::save()
{
    MaintenanceRepository repo(m_db);
    const Result result = m_finishing
                              ? repo.finish(m_vehicle.id, Money::fromLira(m_cost->value()), m_nextService->value())
                              : repo.start(m_vehicle.id, m_description->text());
    if (result.ok())
        accept();
    else
        m_error->setText(I18n::error(result.error));
}