#include "data/ReportRepository.h"

#include "core/Models.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

constexpr int SERVICE_WARNING_KM = 1000;

int value(RentalStatus status)
{
    return static_cast<int>(status);
}

int value(VehicleStatus status)
{
    return static_cast<int>(status);
}

} // namespace

int ReportRepository::count(const QString &sql, const QVariantList &values) const
{
    QSqlQuery query(m_db.connection());
    query.prepare(sql);
    for (const QVariant &v : values)
        query.addBindValue(v);
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}

Summary ReportRepository::summary(const QDate &today) const
{
    const QString day = today.toString(Qt::ISODate);
    Summary s;
    s.vehicles = count("SELECT COUNT(*) FROM vehicles", {});
    s.available = count("SELECT COUNT(*) FROM vehicles WHERE status = ?", {value(VehicleStatus::Available)});
    s.rented = count("SELECT COUNT(*) FROM vehicles WHERE status = ?", {value(VehicleStatus::Rented)});
    s.inMaintenance = count("SELECT COUNT(*) FROM vehicles WHERE status = ?", {value(VehicleStatus::Maintenance)});
    s.pickUpsToday = count("SELECT COUNT(*) FROM rentals WHERE status = ? AND start_date <= ?",
                           {value(RentalStatus::Reserved), day});
    s.returnsToday = count("SELECT COUNT(*) FROM rentals WHERE status = ? AND end_date = ?",
                           {value(RentalStatus::Active), day});
    s.overdue = count("SELECT COUNT(*) FROM rentals WHERE status = ? AND end_date < ?",
                      {value(RentalStatus::Active), day});
    s.serviceDue = count("SELECT COUNT(*) FROM vehicles WHERE status != ? AND next_service_km - mileage < ?",
                         {value(VehicleStatus::Maintenance), SERVICE_WARNING_KM});
    return s;
}

QList<qint64> ReportRepository::monthlyRevenue(int year) const
{
    QList<qint64> months(12, 0);
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT CAST(substr(return_date, 6, 2) AS INTEGER), SUM(total + extra_fees) FROM rentals "
                  "WHERE status = ? AND substr(return_date, 1, 4) = ? GROUP BY 1");
    query.addBindValue(value(RentalStatus::Returned));
    query.addBindValue(QString::number(year));
    if (query.exec())
        while (query.next()) {
            const int month = query.value(0).toInt();
            if (month >= 1 && month <= 12)
                months[month - 1] = query.value(1).toLongLong();
        }
    return months;
}

QList<VehicleStat> ReportRepository::topVehicles(int limit) const
{
    QList<VehicleStat> stats;
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT v.plate, v.brand || ' ' || v.model, COUNT(r.id), SUM(r.total + r.extra_fees) "
                  "FROM rentals r JOIN vehicles v ON v.id = r.vehicle_id WHERE r.status = ? "
                  "GROUP BY v.id ORDER BY COUNT(r.id) DESC, 4 DESC LIMIT ?");
    query.addBindValue(value(RentalStatus::Returned));
    query.addBindValue(limit);
    if (query.exec())
        while (query.next())
            stats << VehicleStat{query.value(0).toString(), query.value(1).toString(), query.value(2).toInt(),
                                 query.value(3).toLongLong()};
    return stats;
}

QList<int> ReportRepository::years() const
{
    QList<int> years;
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT DISTINCT CAST(substr(return_date, 1, 4) AS INTEGER) FROM rentals "
                  "WHERE status = ? ORDER BY 1 DESC");
    query.addBindValue(value(RentalStatus::Returned));
    if (query.exec())
        while (query.next())
            years << query.value(0).toInt();
    return years;
}