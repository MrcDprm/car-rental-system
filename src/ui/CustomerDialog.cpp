#include "ui/CustomerDialog.h"

#include "app/Theme.h"
#include "data/CustomerRepository.h"
#include "ui/UiHelpers.h"

#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>

namespace {

QDateEdit *dateEdit(const QDate &value, const QDate &fallback, QWidget *parent)
{
    auto *edit = new QDateEdit(value.isValid() ? value : fallback, parent);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat("dd.MM.yyyy");
    edit->setMaximumDate(QDate::currentDate());
    return edit;
}

} // namespace

CustomerDialog::CustomerDialog(Database &db, const Customer &customer, QWidget *parent)
    : QDialog(parent), m_db(db), m_customer(customer)
{
    setWindowTitle(I18n::t(customer.id ? "edit_customer" : "new_customer"));
    auto *form = new QFormLayout(this);
    m_name = new QLineEdit(customer.fullName, this);
    m_name->setMaxLength(100);
    m_phone = new QLineEdit(customer.phone, this);
    m_phone->setMaxLength(20);
    m_phone->setPlaceholderText("0532 123 45 67");
    m_email = new QLineEdit(customer.email, this);
    m_email->setMaxLength(100);
    m_nationalId = new QLineEdit(customer.nationalId, this);
    // Kutuya sadece rakam yazılabilir; asıl kontrol (sağlama basamakları) kayıtta yapılır
    m_nationalId->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{0,11}"), m_nationalId));
    m_license = new QLineEdit(customer.licenseNumber, this);
    m_license->setMaxLength(20);
    const QDate today = QDate::currentDate();
    m_birthDate = dateEdit(customer.birthDate, today.addYears(-30), this);
    m_licenseDate = dateEdit(customer.licenseDate, today.addYears(-5), this);

    form->addRow(I18n::t("full_name"), m_name);
    form->addRow(I18n::t("phone"), m_phone);
    form->addRow(I18n::t("email"), m_email);
    form->addRow(I18n::t("national_id"), m_nationalId);
    form->addRow(I18n::t("license_number"), m_license);
    form->addRow(I18n::t("birth_date"), m_birthDate);
    form->addRow(I18n::t("license_date"), m_licenseDate);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    form->addRow(m_error);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &CustomerDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(buttons);
    setMinimumWidth(420);
}

void CustomerDialog::save()
{
    Customer c = m_customer;
    c.fullName = m_name->text();
    c.phone = m_phone->text();
    c.email = m_email->text();
    c.nationalId = m_nationalId->text();
    c.licenseNumber = m_license->text();
    c.birthDate = m_birthDate->date();
    c.licenseDate = m_licenseDate->date();

    CustomerRepository repo(m_db);
    const Result result = c.id ? repo.update(c) : repo.add(c);
    if (result.ok())
        accept();
    else
        m_error->setText(I18n::error(result.error));
}