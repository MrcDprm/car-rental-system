#include "data/CustomerRepository.h"

#include "core/Rules.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

const QString COLUMNS = "id, full_name, phone, email, national_id, license_number, birth_date, license_date";

void bindCustomer(QSqlQuery &query, const Customer &c)
{
    query.addBindValue(text(c.fullName));
    query.addBindValue(text(c.phone));
    query.addBindValue(text(c.email));
    query.addBindValue(text(c.nationalId));
    query.addBindValue(text(c.licenseNumber));
    query.addBindValue(c.birthDate.toString(Qt::ISODate));
    query.addBindValue(c.licenseDate.toString(Qt::ISODate));
}

} // namespace

Customer CustomerRepository::cleaned(Customer customer)
{
    // Kayıttan önce tek biçime getirilir: arama ve "zaten kayıtlı" kontrolü güvenilir olsun
    customer.fullName = customer.fullName.simplified();
    customer.phone = Rules::normalizePhone(customer.phone);
    customer.email = customer.email.trimmed().toLower();
    customer.nationalId = customer.nationalId.trimmed();
    customer.licenseNumber = customer.licenseNumber.trimmed().toUpper();
    return customer;
}

Customer CustomerRepository::fromQuery(const QSqlQuery &query)
{
    Customer c;
    c.id = query.value(0).toLongLong();
    c.fullName = query.value(1).toString();
    c.phone = query.value(2).toString();
    c.email = query.value(3).toString();
    c.nationalId = query.value(4).toString();
    c.licenseNumber = query.value(5).toString();
    c.birthDate = QDate::fromString(query.value(6).toString(), Qt::ISODate);
    c.licenseDate = QDate::fromString(query.value(7).toString(), Qt::ISODate);
    return c;
}

QList<Customer> CustomerRepository::all() const
{
    QList<Customer> customers;
    QSqlQuery query(m_db.connection());
    query.exec("SELECT " + COLUMNS + " FROM customers ORDER BY full_name");
    while (query.next())
        customers << fromQuery(query);
    return customers;
}

std::optional<Customer> CustomerRepository::find(qint64 id) const
{
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT " + COLUMNS + " FROM customers WHERE id = ?");
    query.addBindValue(id);
    if (query.exec() && query.next())
        return fromQuery(query);
    return std::nullopt;
}

Result CustomerRepository::add(Customer customer, const QDate &today)
{
    customer = cleaned(customer);
    const QStringList problems = Rules::customerProblems(customer, today);
    if (!problems.isEmpty())
        return Result::failure(problems.first());

    QSqlQuery query(m_db.connection());
    query.prepare("INSERT INTO customers (full_name, phone, email, national_id, license_number, birth_date, "
                  "license_date) VALUES (?, ?, ?, ?, ?, ?, ?)");
    bindCustomer(query, customer);
    if (!query.exec())
        return Result::failure("duplicate_national_id");
    return {QString(), query.lastInsertId().toLongLong()};
}

Result CustomerRepository::update(Customer customer, const QDate &today)
{
    customer = cleaned(customer);
    const QStringList problems = Rules::customerProblems(customer, today);
    if (!problems.isEmpty())
        return Result::failure(problems.first());

    QSqlQuery query(m_db.connection());
    query.prepare("UPDATE customers SET full_name = ?, phone = ?, email = ?, national_id = ?, license_number = ?, "
                  "birth_date = ?, license_date = ? WHERE id = ?");
    bindCustomer(query, customer);
    query.addBindValue(customer.id);
    if (!query.exec())
        return Result::failure("duplicate_national_id");
    if (query.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), customer.id};
}

Result CustomerRepository::remove(qint64 id)
{
    QSqlQuery used(m_db.connection());
    used.prepare("SELECT COUNT(*) FROM rentals WHERE customer_id = ?");
    used.addBindValue(id);
    if (!used.exec() || !used.next() || used.value(0).toInt() > 0)
        return Result::failure("has_history");

    QSqlQuery query(m_db.connection());
    query.prepare("DELETE FROM customers WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec() || query.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), id};
}