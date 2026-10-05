#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <QList>
#include <optional>

class QSqlQuery;

// Müşteri kayıtları. T.C. kimlik numarası tekildir; yazmadan önce yaş ve ehliyet kuralları kontrol edilir.
class CustomerRepository
{
public:
    explicit CustomerRepository(Database &db) : m_db(db) {}

    QList<Customer> all() const;
    std::optional<Customer> find(qint64 id) const;
    Result add(Customer customer, const QDate &today = QDate::currentDate());
    Result update(Customer customer, const QDate &today = QDate::currentDate());
    Result remove(qint64 id);

private:
    static Customer fromQuery(const QSqlQuery &query);
    static Customer cleaned(Customer customer);

    Database &m_db;
};