#include "ui/CustomersPage.h"

#include "data/CustomerRepository.h"
#include "services/CsvExport.h"
#include "ui/CustomerDialog.h"
#include "ui/UiHelpers.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

QStringList customerCells(const Customer &c)
{
    return {c.fullName, I18n::phone(c.phone), c.email, c.nationalId, c.licenseNumber, I18n::date(c.birthDate),
            I18n::date(c.licenseDate)};
}

QStringList headers()
{
    return {I18n::t("full_name"), I18n::t("phone"), I18n::t("email"), I18n::t("national_id"),
            I18n::t("license_number"), I18n::t("birth_date"), I18n::t("license_date")};
}

} // namespace

CustomersPage::CustomersPage(Database &db, QWidget *parent)
    : QWidget(parent), m_db(db)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    auto *title = new QLabel(I18n::t("customers"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    auto *toolbar = new QHBoxLayout;
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(I18n::t("search"));
    m_search->setClearButtonEnabled(true);
    auto *add = Ui::accentButton(I18n::t("add"), this);
    m_edit = new QPushButton(I18n::t("edit"), this);
    m_delete = new QPushButton(I18n::t("delete"), this);
    auto *csv = new QPushButton(I18n::t("export_csv"), this);
    toolbar->addWidget(m_search, 1);
    for (QPushButton *button : {add, m_edit, m_delete, csv})
        toolbar->addWidget(button);
    layout->addLayout(toolbar);

    m_table = Ui::makeTable(headers(), this);
    layout->addWidget(m_table, 1);

    connect(m_search, &QLineEdit::textChanged, this, &CustomersPage::refresh);
    connect(add, &QPushButton::clicked, this, &CustomersPage::addCustomer);
    connect(m_edit, &QPushButton::clicked, this, &CustomersPage::editCustomer);
    connect(m_delete, &QPushButton::clicked, this, &CustomersPage::deleteCustomer);
    connect(csv, &QPushButton::clicked, this, &CustomersPage::exportCsv);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &CustomersPage::editCustomer);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const bool selected = Ui::selectedId(m_table) != 0;
        m_edit->setEnabled(selected);
        m_delete->setEnabled(selected);
    });
}

void CustomersPage::refresh()
{
    m_customers = CustomerRepository(m_db).all();
    const QString search = m_search->text().trimmed();
    m_table->setRowCount(0);
    for (const Customer &c : m_customers) {
        const QStringList cells = customerCells(c);
        if (!search.isEmpty() && !cells.join(' ').contains(search, Qt::CaseInsensitive))
            continue;
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        Ui::setRow(m_table, row, cells, c.id);
    }
    m_table->resizeColumnsToContents();
    m_edit->setEnabled(false);
    m_delete->setEnabled(false);
}

void CustomersPage::addCustomer()
{
    if (CustomerDialog(m_db, Customer{}, this).exec() == QDialog::Accepted)
        refresh();
}

void CustomersPage::editCustomer()
{
    const auto customer = CustomerRepository(m_db).find(Ui::selectedId(m_table));
    if (customer && CustomerDialog(m_db, *customer, this).exec() == QDialog::Accepted)
        refresh();
}

void CustomersPage::deleteCustomer()
{
    const qint64 id = Ui::selectedId(m_table);
    if (!id || QMessageBox::question(this, I18n::t("app_name"), I18n::t("confirm_delete")) != QMessageBox::Yes)
        return;
    if (Ui::showResult(this, CustomerRepository(m_db).remove(id)))
        refresh();
}

void CustomersPage::exportCsv()
{
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, I18n::t("export_csv"), folder + "/customers.csv",
                                                      "CSV (*.csv)");
    if (path.isEmpty())
        return;
    QList<QStringList> rows;
    for (const Customer &c : m_customers)
        rows << customerCells(c);
    if (CsvExport::write(path, headers(), rows, CsvExport::separatorFor(I18n::language())))
        QMessageBox::information(this, I18n::t("app_name"), I18n::t("saved_csv").replace("{0}", QString::number(rows.size())));
    else
        QMessageBox::warning(this, I18n::t("app_name"), I18n::t("csv_failed"));
}