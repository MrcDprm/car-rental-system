#include "ui/RentalActions.h"

#include "data/CustomerRepository.h"
#include "data/RentalRepository.h"
#include "data/VehicleRepository.h"
#include "services/ContractPrinter.h"
#include "ui/ReturnDialog.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QStandardPaths>
#include <QUrl>

namespace RentalActions {

QString number(qint64 id)
{
    return QString("%1").arg(id, 5, 10, QChar('0'));
}

bool pickUp(Database &db, const Rental &rental, QWidget *parent)
{
    const auto vehicle = VehicleRepository(db).find(rental.vehicleId);
    if (!vehicle)
        return false;

    // Teslim anında aracın gerçek km'si ve yakıtı yazılır; km göstergedeki değerden düşük olamaz
    QDialog dialog(parent);
    dialog.setWindowTitle(I18n::t("pick_up") + " · " + vehicle->plate);
    auto *form = new QFormLayout(&dialog);
    auto *kmBox = new QSpinBox(&dialog);
    kmBox->setRange(vehicle->mileage, vehicle->mileage + 100'000);
    kmBox->setSuffix(" km");
    kmBox->setGroupSeparatorShown(true);
    kmBox->setValue(vehicle->mileage);
    auto *fuelBox = new QComboBox(&dialog);
    for (int level = FUEL_FULL; level >= 0; --level)
        fuelBox->addItem(QString("%1/%2").arg(level).arg(FUEL_FULL), level);
    form->addRow(I18n::t("km_out"), kmBox);
    form->addRow(I18n::t("fuel_level"), fuelBox);
    auto *buttons = new QDialogButtonBox(&dialog);
    buttons->addButton(Ui::accentButton(I18n::t("pick_up"), &dialog), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    return Ui::showResult(parent, RentalRepository(db).pickUp(rental.id, kmBox->value(), fuelBox->currentData().toInt()));
}

bool giveBack(Database &db, const Rental &rental, QWidget *parent)
{
    return ReturnDialog(db, rental, parent).exec() == QDialog::Accepted;
}

void printContract(Database &db, const Rental &rental, QWidget *parent)
{
    const auto vehicle = VehicleRepository(db).find(rental.vehicleId);
    const auto customer = CustomerRepository(db).find(rental.customerId);
    if (!vehicle || !customer)
        return;
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString name = (rental.status == RentalStatus::Returned ? "receipt-" : "contract-") + number(rental.id);
    const QString path = QFileDialog::getSaveFileName(parent, I18n::t("save_pdf"), folder + "/" + name + ".pdf",
                                                      "PDF (*.pdf)");
    if (path.isEmpty())
        return;
    if (!ContractPrinter::savePdf(ContractPrinter::html(rental, *vehicle, *customer), path)) {
        Ui::warn(parent, I18n::error("save_failed"));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path)); // kaydedilen PDF varsayılan görüntüleyicide açılır
}

} // namespace RentalActions
