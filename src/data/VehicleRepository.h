#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <QList>
#include <optional>

class QSqlQuery;

// Araç kayıtları. Bütün sorgular parametrelidir (SQL injection'a kapalı); yazmadan önce kurallar kontrol edilir.
class VehicleRepository
{
public:
    explicit VehicleRepository(Database &db) : m_db(db) {}

    QList<Vehicle> all() const;
    std::optional<Vehicle> find(qint64 id) const;
    QList<Vehicle> availableBetween(const QDate &start, const QDate &end,
                                    const QDate &today = QDate::currentDate()) const;
    Result add(Vehicle vehicle, const QDate &today = QDate::currentDate());
    Result update(Vehicle vehicle, const QDate &today = QDate::currentDate());
    Result remove(qint64 id);

    static Vehicle fromQuery(const QSqlQuery &query);

private:
    Database &m_db;
};
