#include "data/MaintenanceRepository.h"

#include "data/VehicleRepository.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

constexpr int MAX_DESCRIPTION = 300;
constexpr qint64 MAX_COST = 10'000'000'00; // 10 milyon TL

} // namespace

QList<Maintenance> MaintenanceRepository::forVehicle(qint64 vehicleId) const
{
    QList<Maintenance> records;
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT id, vehicle_id, start_date, end_date, description, cost FROM maintenance "
                  "WHERE vehicle_id = ? ORDER BY start_date DESC, id DESC");
    query.addBindValue(vehicleId);
    if (!query.exec())
        return records;
    while (query.next()) {
        Maintenance m;
        m.id = query.value(0).toLongLong();
        m.vehicleId = query.value(1).toLongLong();
        m.startDate = QDate::fromString(query.value(2).toString(), Qt::ISODate);
        m.endDate = QDate::fromString(query.value(3).toString(), Qt::ISODate);
        m.description = query.value(4).toString();
        m.cost = query.value(5).toLongLong();
        records << m;
    }
    return records;
}

Result MaintenanceRepository::start(qint64 vehicleId, const QString &description, const QDate &today)
{
    const QString cleaned = description.trimmed();
    if (cleaned.isEmpty() || cleaned.size() > MAX_DESCRIPTION)
        return Result::failure("invalid_description");
    const std::optional<Vehicle> vehicle = VehicleRepository(m_db).find(vehicleId);
    if (!vehicle)
        return Result::failure("not_found");
    if (vehicle->status != VehicleStatus::Available)
        return Result::failure("vehicle_not_available"); // kiradaki araç bakıma alınamaz

    QSqlDatabase db = m_db.connection();
    Transaction transaction(db);
    QSqlQuery insert(db);
    insert.prepare("INSERT INTO maintenance (vehicle_id, start_date, end_date, description, cost) "
                   "VALUES (?, ?, NULL, ?, 0)");
    insert.addBindValue(vehicleId);
    insert.addBindValue(today.toString(Qt::ISODate));
    insert.addBindValue(cleaned);
    QSqlQuery update(db);
    update.prepare("UPDATE vehicles SET status = ? WHERE id = ? AND status = ?");
    update.addBindValue(static_cast<int>(VehicleStatus::Maintenance));
    update.addBindValue(vehicleId);
    update.addBindValue(static_cast<int>(VehicleStatus::Available));
    if (!insert.exec() || !update.exec() || update.numRowsAffected() != 1 || !transaction.commit())
        return Result::failure("save_failed");
    return {QString(), insert.lastInsertId().toLongLong()};
}

Result MaintenanceRepository::finish(qint64 vehicleId, qint64 cost, int nextServiceKm, const QDate &today)
{
    const std::optional<Vehicle> vehicle = VehicleRepository(m_db).find(vehicleId);
    if (!vehicle || vehicle->status != VehicleStatus::Maintenance)
        return Result::failure("invalid_state");
    if (cost < 0 || cost > MAX_COST)
        return Result::failure("invalid_cost");
    if (nextServiceKm <= vehicle->mileage)
        return Result::failure("invalid_service_km");

    QSqlDatabase db = m_db.connection();
    Transaction transaction(db);
    QSqlQuery close(db);
    close.prepare("UPDATE maintenance SET end_date = ?, cost = ? WHERE vehicle_id = ? AND end_date IS NULL");
    close.addBindValue(today.toString(Qt::ISODate));
    close.addBindValue(cost);
    close.addBindValue(vehicleId);
    QSqlQuery update(db);
    update.prepare("UPDATE vehicles SET status = ?, next_service_km = ? WHERE id = ?");
    update.addBindValue(static_cast<int>(VehicleStatus::Available));
    update.addBindValue(nextServiceKm);
    update.addBindValue(vehicleId);
    if (!close.exec() || close.numRowsAffected() != 1 || !update.exec() || !transaction.commit())
        return Result::failure("save_failed");
    return {QString(), vehicleId};
}