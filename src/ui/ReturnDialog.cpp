#include "ui/ReturnDialog.h"

#include "app/Theme.h"
#include "core/Pricing.h"
#include "data/RentalRepository.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

namespace {

// Yakıt seçimi sekizde bir adımlarla: "Dolu (8/8)", "7/8" … "Boş (0/8)"
QComboBox *fuelCombo(int selected, QWidget *parent)
{
    auto *combo = new QComboBox(parent);
    for (int level = FUEL_FULL; level >= 0; --level)
        combo->addItem(QString("%1/%2").arg(level).arg(FUEL_FULL), level);
    combo->setCurrentIndex(combo->findData(selected));
    return combo;
}

} // namespace

ReturnDialog::ReturnDialog(Database &db, const Rental &rental, QWidget *parent)
    : QDialog(parent), m_db(db), m_rental(rental)
{
    setWindowTitle(I18n::t("give_back"));
    auto *form = new QFormLayout(this);
    m_date = new QDateEdit(QDate::currentDate(), this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("dd.MM.yyyy");
    m_date->setMinimumDate(rental.startDate);
    m_km = new QSpinBox(this);
    m_km->setRange(rental.startKm, rental.startKm + 100'000);
    m_km->setSuffix(" km");
    m_km->setGroupSeparatorShown(true);
    m_km->setValue(rental.startKm);
    m_fuel = fuelCombo(rental.fuelOut, this);
    form->addRow(I18n::t("return_date"), m_date);
    form->addRow(I18n::t("km_in"), m_km);
    form->addRow(I18n::t("fuel_level"), m_fuel);

    m_charges = new QLabel(this);
    m_charges->setTextFormat(Qt::PlainText);
    form->addRow(m_charges);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    form->addRow(m_error);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("give_back"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &ReturnDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(buttons);

    connect(m_date, &QDateEdit::dateChanged, this, &ReturnDialog::updateCharges);
    connect(m_km, &QSpinBox::valueChanged, this, &ReturnDialog::updateCharges);
    connect(m_fuel, &QComboBox::currentIndexChanged, this, &ReturnDialog::updateCharges);
    updateCharges();
    setMinimumWidth(420);
}

void ReturnDialog::updateCharges()
{
    // Kayıtta kullanılan hesaplamanın aynısı: çalışan, müşteriye ödenecek tutarı iadeyi onaylamadan görür
    const Pricing::ReturnCharges c = Pricing::returnCharges(m_rental, m_date->date(), m_km->value(),
                                                            m_fuel->currentData().toInt());
    QStringList lines;
    lines << I18n::t("rental_fee") + ":  " + I18n::money(m_rental.total);
    if (c.lateFee > 0)
        lines << I18n::t("late_fee").replace("{0}", QString::number(c.lateDays)) + ":  " + I18n::money(c.lateFee);
    if (c.extraKmFee > 0)
        lines << I18n::t("extra_km_fee").replace("{0}", QString::number(c.extraKm)) + ":  " + I18n::money(c.extraKmFee);
    if (c.fuelFee > 0)
        lines << I18n::t("fuel_fee").replace("{0}", QString::number(c.missingFuel)) + ":  " + I18n::money(c.fuelFee);
    lines << I18n::t("grand_total") + ":  " + I18n::money(m_rental.total + c.total);
    m_charges->setText(lines.join('\n'));
}

void ReturnDialog::save()
{
    const Result result =
        RentalRepository(m_db).giveBack(m_rental.id, m_date->date(), m_km->value(), m_fuel->currentData().toInt());
    if (result.ok())
        accept();
    else
        m_error->setText(I18n::error(result.error));
}