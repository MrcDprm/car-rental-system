#include "ui/RentalsPage.h"

#include "app/Theme.h"
#include "data/CustomerRepository.h"
#include "data/RentalRepository.h"
#include "data/VehicleRepository.h"
#include "services/CsvExport.h"
#include "ui/RentalActions.h"
#include "ui/RentalDialog.h"
#include "ui/UiHelpers.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

QStringList headers()
{
    return {I18n::t("number"), I18n::t("customer"), I18n::t("plate"), I18n::t("vehicle"), I18n::t("start_date"),
            I18n::t("end_date"), I18n::t("total"), I18n::t("status")};
}

} // namespace

RentalsPage::RentalsPage(Database &db, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    auto *title = new QLabel(I18n::t("rentals"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    auto *toolbar = new QHBoxLayout;
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(I18n::t("search"));
    m_search->setClearButtonEnabled(true);
    m_filter = new QComboBox(this);
    m_filter->addItem(I18n::t("all"));
    for (int i = 0; i <= static_cast<int>(RentalStatus::Cancelled); ++i)
        m_filter->addItem(I18n::rentalStatus(static_cast<RentalStatus>(i)));
    auto *add = Ui::accentButton(I18n::t("new_rental"), this);
    m_pickUp = new QPushButton(I18n::t("pick_up"), this);
    m_giveBack = new QPushButton(I18n::t("give_back"), this);
    m_cancel = new QPushButton(I18n::t("cancel_rental"), this);
    m_print = new QPushButton(I18n::t("print_contract"), this);
    auto *csv = new QPushButton(I18n::t("export_csv"), this);
    toolbar->addWidget(m_search, 1);
    toolbar->addWidget(m_filter);
    for (QPushButton *button : {add, m_pickUp, m_giveBack, m_cancel, m_print, csv})
        toolbar->addWidget(button);
    layout->addLayout(toolbar);

    m_table = Ui::makeTable(headers(), this);
    layout->addWidget(m_table, 1);

    connect(m_search, &QLineEdit::textChanged, this, &RentalsPage::refresh);
    connect(m_filter, &QComboBox::currentIndexChanged, this, &RentalsPage::refresh);
    connect(add, &QPushButton::clicked, this, &RentalsPage::newRental);
    connect(m_pickUp, &QPushButton::clicked, this, &RentalsPage::pickUp);
    connect(m_giveBack, &QPushButton::clicked, this, &RentalsPage::giveBack);
    connect(m_cancel, &QPushButton::clicked, this, &RentalsPage::cancelRental);
    connect(m_print, &QPushButton::clicked, this, &RentalsPage::printContract);
    connect(csv, &QPushButton::clicked, this, &RentalsPage::exportCsv);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &RentalsPage::updateButtons);
}

QStringList RentalsPage::cells(const Rental &r) const
{
    const Vehicle vehicle = m_vehicles.value(r.vehicleId);
    return {RentalActions::number(r.id), m_customers.value(r.customerId).fullName, vehicle.plate,
            vehicle.brand + " " + vehicle.model, I18n::date(r.startDate),
            I18n::date(r.returnDate.isValid() ? r.returnDate : r.endDate), I18n::money(r.total + r.extraFees),
            I18n::rentalStatus(r.status)};
}

void RentalsPage::refresh()
{
    m_rentals = RentalRepository(m_db).all();
    m_vehicles.clear();
    for (const Vehicle &v : VehicleRepository(m_db).all())
        m_vehicles.insert(v.id, v);
    m_customers.clear();
    for (const Customer &c : CustomerRepository(m_db).all())
        m_customers.insert(c.id, c);

    const QString search = m_search->text().trimmed();
    const int filter = m_filter->currentIndex() - 1; // -1 = tümü
    const QDate today = QDate::currentDate();
    m_table->setRowCount(0);
    for (const Rental &r : m_rentals) {
        if (filter >= 0 && static_cast<int>(r.status) != filter)
            continue;
        const QStringList row = cells(r);
        if (!search.isEmpty() && !row.join(' ').contains(search, Qt::CaseInsensitive))
            continue;
        // Dönüş günü geçmiş aktif kiralama kırmızı, iptal edilen soluk
        QColor color;
        if (r.status == RentalStatus::Active && r.endDate < today)
            color = Theme::danger();
        else if (r.status == RentalStatus::Cancelled)
            color = Theme::muted();
        const int index = m_table->rowCount();
        m_table->insertRow(index);
        Ui::setRow(m_table, index, row, r.id, color);
    }
    m_table->resizeColumnsToContents();
    updateButtons();
}

const Rental *RentalsPage::selected() const
{
    const qint64 id = Ui::selectedId(m_table);
    for (const Rental &r : m_rentals)
        if (r.id == id)
            return &r;
    return nullptr;
}

void RentalsPage::updateButtons()
{
    const Rental *rental = selected();
    const RentalStatus status = rental ? rental->status : RentalStatus::Cancelled;
    m_pickUp->setEnabled(rental && status == RentalStatus::Reserved);
    m_cancel->setEnabled(rental && status == RentalStatus::Reserved);
    m_giveBack->setEnabled(rental && status == RentalStatus::Active);
    // Sözleşme teslimden sonra, iade fişi iadeden sonra basılır
    m_print->setEnabled(rental && (status == RentalStatus::Active || status == RentalStatus::Returned));
}

void RentalsPage::newRental()
{
    if (RentalDialog(m_db, this).exec() == QDialog::Accepted)
        refresh();
}

void RentalsPage::pickUp()
{
    const Rental *rental = selected();
    if (rental && RentalActions::pickUp(m_db, *rental, this))
        refresh();
}

void RentalsPage::giveBack()
{
    const Rental *rental = selected();
    if (rental && RentalActions::giveBack(m_db, *rental, this))
        refresh();
}

void RentalsPage::cancelRental()
{
    const Rental *rental = selected();
    if (!rental || !Ui::ask(this, I18n::t("confirm_cancel")))
        return;
    if (Ui::showResult(this, RentalRepository(m_db).cancel(rental->id)))
        refresh();
}

void RentalsPage::printContract()
{
    if (const Rental *rental = selected())
        RentalActions::printContract(m_db, *rental, this);
}

void RentalsPage::exportCsv()
{
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, I18n::t("export_csv"), folder + "/rentals.csv",
                                                      "CSV (*.csv)");
    if (path.isEmpty())
        return;
    QList<QStringList> rows;
    for (const Rental &r : m_rentals)
        rows << cells(r);
    if (CsvExport::write(path, headers(), rows, CsvExport::separatorFor(I18n::language())))
        Ui::inform(this, I18n::t("saved_csv").replace("{0}", QString::number(rows.size())));
    else
        Ui::warn(this, I18n::t("csv_failed"));
}
