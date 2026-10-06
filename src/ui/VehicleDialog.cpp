#include "ui/VehicleDialog.h"

#include "app/Theme.h"
#include "core/Money.h"
#include "data/VehicleRepository.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

VehicleDialog::VehicleDialog(Database &db, const Vehicle &vehicle, QWidget *parent)
    : QDialog(parent), m_db(db), m_vehicle(vehicle)
{
    setWindowTitle(I18n::t(vehicle.id ? "edit_vehicle" : "new_vehicle"));
    auto *form = new QFormLayout(this);

    m_plate = new QLineEdit(vehicle.plate, this);
    m_plate->setMaxLength(12);
    m_plate->setPlaceholderText("34 ABC 123");
    m_brand = new QLineEdit(vehicle.brand, this);
    m_brand->setMaxLength(100);
    m_model = new QLineEdit(vehicle.model, this);
    m_model->setMaxLength(100);
    m_year = new QSpinBox(this);
    m_year->setRange(1990, QDate::currentDate().year() + 1);
    m_year->setValue(vehicle.year ? vehicle.year : QDate::currentDate().year());

    m_class = new QComboBox(this);
    for (int i = 0; i <= static_cast<int>(VehicleClass::Luxury); ++i)
        m_class->addItem(I18n::vehicleClass(static_cast<VehicleClass>(i)));
    m_class->setCurrentIndex(static_cast<int>(vehicle.vehicleClass));
    m_transmission = new QComboBox(this);
    m_transmission->addItems({I18n::transmission(Transmission::Manual), I18n::transmission(Transmission::Automatic)});
    m_transmission->setCurrentIndex(static_cast<int>(vehicle.transmission));
    m_fuel = new QComboBox(this);
    for (int i = 0; i <= static_cast<int>(Fuel::Lpg); ++i)
        m_fuel->addItem(I18n::fuel(static_cast<Fuel>(i)));
    m_fuel->setCurrentIndex(static_cast<int>(vehicle.fuel));

    m_seats = new QSpinBox(this);
    m_seats->setRange(2, 9);
    m_seats->setValue(vehicle.seats);
    m_price = new QDoubleSpinBox(this);
    m_price->setRange(0, 100000);
    m_price->setDecimals(2);
    m_price->setGroupSeparatorShown(true);
    m_price->setSuffix(" ₺");
    m_price->setValue(Money::toLira(vehicle.dailyPrice));
    m_mileage = new QSpinBox(this);
    m_mileage->setRange(0, 2'000'000);
    m_mileage->setSuffix(" km");
    m_mileage->setGroupSeparatorShown(true);
    m_mileage->setValue(vehicle.mileage);
    m_nextService = new QSpinBox(this);
    m_nextService->setRange(0, 2'000'000);
    m_nextService->setSuffix(" km");
    m_nextService->setGroupSeparatorShown(true);
    m_nextService->setValue(vehicle.nextServiceKm ? vehicle.nextServiceKm : 15000);

    form->addRow(I18n::t("plate"), m_plate);
    form->addRow(I18n::t("brand"), m_brand);
    form->addRow(I18n::t("model"), m_model);
    form->addRow(I18n::t("year"), m_year);
    form->addRow(I18n::t("class"), m_class);
    form->addRow(I18n::t("transmission"), m_transmission);
    form->addRow(I18n::t("fuel"), m_fuel);
    form->addRow(I18n::t("seats"), m_seats);
    form->addRow(I18n::t("daily_price"), m_price);
    form->addRow(I18n::t("mileage"), m_mileage);
    form->addRow(I18n::t("next_service"), m_nextService);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    form->addRow(m_error);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &VehicleDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(buttons);
    setMinimumWidth(420);
}

void VehicleDialog::save()
{
    Vehicle v = m_vehicle;
    v.plate = m_plate->text();
    v.brand = m_brand->text();
    v.model = m_model->text();
    v.year = m_year->value();
    v.vehicleClass = static_cast<VehicleClass>(m_class->currentIndex());
    v.transmission = static_cast<Transmission>(m_transmission->currentIndex());
    v.fuel = static_cast<Fuel>(m_fuel->currentIndex());
    v.seats = m_seats->value();
    v.dailyPrice = Money::fromLira(m_price->value());
    v.mileage = m_mileage->value();
    v.nextServiceKm = m_nextService->value();

    VehicleRepository repo(m_db);
    const Result result = v.id ? repo.update(v) : repo.add(v);
    if (result.ok())
        accept();
    else
        m_error->setText(I18n::error(result.error));
}