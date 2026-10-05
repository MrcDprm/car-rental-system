#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <QList>
#include <optional>

class QSqlQuery;

// Kiralamanın yaşam döngüsü: rezervasyon → teslim (aktif) → iade, ya da iptal.
// Her adım bir veritabanı işlemi (transaction) içinde aracın durumu ve km'siyle birlikte güncellenir.
class RentalRepository
{
public:
    explicit RentalRepository(Database &db) : m_db(db) {}

    QList<Rental> all() const;
    std::optional<Rental> find(qint64 id) const;
    Result reserve(Rental rental, const QDate &today = QDate::currentDate());
    Result pickUp(qint64 id, int startKm, int fuelOut, const QDate &today = QDate::currentDate());
    Result giveBack(qint64 id, const QDate &returnDate, int endKm, int fuelIn);
    Result cancel(qint64 id);

private:
    static Rental fromQuery(const QSqlQuery &query);
    bool hasOverlap(qint64 vehicleId, const QDate &start, const QDate &end) const;

    Database &m_db;
};