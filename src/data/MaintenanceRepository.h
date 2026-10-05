#pragma once

#include "core/Models.h"
#include "data/Database.h"

#include <QList>

// Bakım kayıtları: araç bakıma alınınca "bakımda" olur ve kiralanamaz; bakımdan çıkınca maliyet ve
// bir sonraki bakım km'si kaydedilir.
class MaintenanceRepository
{
public:
    explicit MaintenanceRepository(Database &db) : m_db(db) {}

    QList<Maintenance> forVehicle(qint64 vehicleId) const;
    Result start(qint64 vehicleId, const QString &description, const QDate &today = QDate::currentDate());
    Result finish(qint64 vehicleId, qint64 cost, int nextServiceKm, const QDate &today = QDate::currentDate());

private:
    Database &m_db;
};