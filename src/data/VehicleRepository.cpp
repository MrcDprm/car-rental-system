#include "data/VehicleRepository.h"

#include "core/Rules.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

const QString COLUMNS = "id, plate, brand, model, year, vehicle_class, transmission, fuel, seats, body_type, "
                        "color, luggage, features, photo, daily_price, mileage, next_service_km, status";

// Dosyadan okunan sayı bilinmeyen bir enum değeriyse varsayılana döner (dosya elle bozulmuş olabilir)
template <typename Enum>
Enum toEnum(const QVariant &value, Enum last, Enum fallback)
{
    const int number = value.toInt();
    return number >= 0 && number <= static_cast<int>(last) ? static_cast<Enum>(number) : fallback;
}

void bindVehicle(QSqlQuery &query, const Vehicle &v)
{
    query.addBindValue(text(v.plate));
    query.addBindValue(text(v.brand.trimmed()));
    query.addBindValue(text(v.model.trimmed()));
    query.addBindValue(v.year);
    query.addBindValue(static_cast<int>(v.vehicleClass));
    query.addBindValue(static_cast<int>(v.transmission));
    query.addBindValue(static_cast<int>(v.fuel));
    query.addBindValue(v.seats);
    query.addBindValue(static_cast<int>(v.bodyType));
    query.addBindValue(static_cast<int>(v.color));
    query.addBindValue(v.luggage);
    query.addBindValue(v.features);
    query.addBindValue(text(v.photo));
    query.addBindValue(v.dailyPrice);
    query.addBindValue(v.mileage);
    query.addBindValue(v.nextServiceKm);
}

} // namespace

Vehicle VehicleRepository::fromQuery(const QSqlQuery &query)
{
    Vehicle v;
    v.id = query.value(0).toLongLong();
    v.plate = query.value(1).toString();
    v.brand = query.value(2).toString();
    v.model = query.value(3).toString();
    v.year = query.value(4).toInt();
    v.vehicleClass = toEnum(query.value(5), VehicleClass::Luxury, VehicleClass::Economy);
    v.transmission = toEnum(query.value(6), Transmission::Automatic, Transmission::Manual);
    v.fuel = toEnum(query.value(7), Fuel::Lpg, Fuel::Petrol);
    v.seats = query.value(8).toInt();
    v.bodyType = toEnum(query.value(9), BodyType::Pickup, BodyType::Hatchback);
    v.color = toEnum(query.value(10), CarColor::Brown, CarColor::White);
    v.luggage = query.value(11).toInt();
    v.features = query.value(12).toInt() & Feature::ALL; // bilinmeyen bitler atılır
    v.photo = query.value(13).toString();
    v.dailyPrice = query.value(14).toLongLong();
    v.mileage = query.value(15).toInt();
    v.nextServiceKm = query.value(16).toInt();
    v.status = toEnum(query.value(17), VehicleStatus::Maintenance, VehicleStatus::Available);
    return v;
}

QList<Vehicle> VehicleRepository::all() const
{
    QList<Vehicle> vehicles;
    QSqlQuery query(m_db.connection());
    query.exec("SELECT " + COLUMNS + " FROM vehicles ORDER BY plate");
    while (query.next())
        vehicles << fromQuery(query);
    return vehicles;
}

std::optional<Vehicle> VehicleRepository::find(qint64 id) const
{
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT " + COLUMNS + " FROM vehicles WHERE id = ?");
    query.addBindValue(id);
    if (query.exec() && query.next())
        return fromQuery(query);
    return std::nullopt;
}

QList<Vehicle> VehicleRepository::availableBetween(const QDate &start, const QDate &end, const QDate &today) const
{
    // Bakımda olmayan ve bu aralıkla çakışan rezervasyonu ya da kiralaması bulunmayan araçlar.
    // Dönüş günü geçtiği hâlde dönmemiş araç en az bugün boyunca dolu sayılır (MAX ile bitiş ertesi güne uzar).
    QList<Vehicle> vehicles;
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT " + COLUMNS + " FROM vehicles v WHERE status != ? AND NOT EXISTS ("
                  "SELECT 1 FROM rentals r WHERE r.vehicle_id = v.id AND r.status IN (?, ?) AND r.start_date < ? "
                  "AND ? < CASE WHEN r.status = ? THEN MAX(r.end_date, ?) ELSE r.end_date END) "
                  "ORDER BY daily_price, plate");
    query.addBindValue(static_cast<int>(VehicleStatus::Maintenance));
    query.addBindValue(static_cast<int>(RentalStatus::Reserved));
    query.addBindValue(static_cast<int>(RentalStatus::Active));
    query.addBindValue(end.toString(Qt::ISODate));
    query.addBindValue(start.toString(Qt::ISODate));
    query.addBindValue(static_cast<int>(RentalStatus::Active));
    query.addBindValue(today.addDays(1).toString(Qt::ISODate));
    if (query.exec())
        while (query.next())
            vehicles << fromQuery(query);
    return vehicles;
}

Result VehicleRepository::add(Vehicle vehicle, const QDate &today)
{
    vehicle.plate = Rules::normalizePlate(vehicle.plate);
    const QStringList problems = Rules::vehicleProblems(vehicle, today);
    if (!problems.isEmpty())
        return Result::failure(problems.first());

    QSqlQuery query(m_db.connection());
    query.prepare("INSERT INTO vehicles (plate, brand, model, year, vehicle_class, transmission, fuel, seats, "
                  "body_type, color, luggage, features, photo, daily_price, mileage, next_service_km, status) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    bindVehicle(query, vehicle);
    query.addBindValue(static_cast<int>(VehicleStatus::Available));
    if (!query.exec())
        return Result::failure("duplicate_plate"); // tek UNIQUE alan plaka
    return {QString(), query.lastInsertId().toLongLong()};
}

Result VehicleRepository::update(Vehicle vehicle, const QDate &today)
{
    vehicle.plate = Rules::normalizePlate(vehicle.plate);
    const QStringList problems = Rules::vehicleProblems(vehicle, today);
    if (!problems.isEmpty())
        return Result::failure(problems.first());

    // Durum (müsait/kirada/bakımda) burada değişmez; onu kiralama ve bakım işlemleri yönetir
    QSqlQuery query(m_db.connection());
    query.prepare("UPDATE vehicles SET plate = ?, brand = ?, model = ?, year = ?, vehicle_class = ?, "
                  "transmission = ?, fuel = ?, seats = ?, body_type = ?, color = ?, luggage = ?, features = ?, "
                  "photo = ?, daily_price = ?, mileage = ?, next_service_km = ? "
                  "WHERE id = ?");
    bindVehicle(query, vehicle);
    query.addBindValue(vehicle.id);
    if (!query.exec())
        return Result::failure("duplicate_plate");
    if (query.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), vehicle.id};
}

Result VehicleRepository::remove(qint64 id)
{
    // Geçmişi olan araç silinmez: eski sözleşmeler ve raporlar bozulmasın
    QSqlQuery used(m_db.connection());
    used.prepare("SELECT (SELECT COUNT(*) FROM rentals WHERE vehicle_id = ?) "
                 "+ (SELECT COUNT(*) FROM maintenance WHERE vehicle_id = ?)");
    used.addBindValue(id);
    used.addBindValue(id);
    if (!used.exec() || !used.next() || used.value(0).toInt() > 0)
        return Result::failure("has_history");

    QSqlQuery query(m_db.connection());
    query.prepare("DELETE FROM vehicles WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec() || query.numRowsAffected() == 0)
        return Result::failure("not_found");
    return {QString(), id};
}
