#include "data/RentalRepository.h"

#include "core/Pricing.h"
#include "core/Rules.h"
#include "data/CustomerRepository.h"
#include "data/VehicleRepository.h"

#include <QSqlQuery>
#include <QVariant>

namespace {

const QString COLUMNS = "id, vehicle_id, customer_id, start_date, end_date, return_date, start_km, end_km, "
                        "fuel_out, fuel_in, daily_price, total, extra_fees, deposit, status, notes";
constexpr qint64 MAX_DEPOSIT = 1'000'000'00; // 1 milyon TL
constexpr int MAX_NOTES = 500;

QString iso(const QDate &date)
{
    return date.toString(Qt::ISODate);
}

// Teslimde ve iadede aracın durumu ve km'si kiralamayla birlikte güncellenir
bool setVehicle(QSqlDatabase db, qint64 vehicleId, VehicleStatus status, int mileage)
{
    QSqlQuery query(db);
    query.prepare("UPDATE vehicles SET status = ?, mileage = ? WHERE id = ?");
    query.addBindValue(static_cast<int>(status));
    query.addBindValue(mileage);
    query.addBindValue(vehicleId);
    return query.exec() && query.numRowsAffected() == 1;
}

} // namespace

Rental RentalRepository::fromQuery(const QSqlQuery &query)
{
    Rental r;
    r.id = query.value(0).toLongLong();
    r.vehicleId = query.value(1).toLongLong();
    r.customerId = query.value(2).toLongLong();
    r.startDate = QDate::fromString(query.value(3).toString(), Qt::ISODate);
    r.endDate = QDate::fromString(query.value(4).toString(), Qt::ISODate);
    r.returnDate = QDate::fromString(query.value(5).toString(), Qt::ISODate);
    r.startKm = query.value(6).toInt();
    r.endKm = query.value(7).toInt();
    r.fuelOut = query.value(8).toInt();
    r.fuelIn = query.value(9).toInt();
    r.dailyPrice = query.value(10).toLongLong();
    r.total = query.value(11).toLongLong();
    r.extraFees = query.value(12).toLongLong();
    r.deposit = query.value(13).toLongLong();
    const int status = query.value(14).toInt();
    r.status = status >= 0 && status <= static_cast<int>(RentalStatus::Cancelled) ? static_cast<RentalStatus>(status)
                                                                                    : RentalStatus::Cancelled;
    r.notes = query.value(15).toString();
    return r;
}

QList<Rental> RentalRepository::all() const
{
    QList<Rental> rentals;
    QSqlQuery query(m_db.connection());
    query.exec("SELECT " + COLUMNS + " FROM rentals ORDER BY start_date DESC, id DESC");
    while (query.next())
        rentals << fromQuery(query);
    return rentals;
}

std::optional<Rental> RentalRepository::find(qint64 id) const
{
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT " + COLUMNS + " FROM rentals WHERE id = ?");
    query.addBindValue(id);
    if (query.exec() && query.next())
        return fromQuery(query);
    return std::nullopt;
}

bool RentalRepository::hasOverlap(qint64 vehicleId, const QDate &start, const QDate &end, const QDate &today) const
{
    // Aktif kiralamanın dönüş günü geçmişse araç dönene kadar en az bugün boyunca dolu sayılır
    QSqlQuery query(m_db.connection());
    query.prepare("SELECT COUNT(*) FROM rentals WHERE vehicle_id = ? AND status IN (?, ?) AND start_date < ? "
                  "AND ? < CASE WHEN status = ? THEN MAX(end_date, ?) ELSE end_date END");
    query.addBindValue(vehicleId);
    query.addBindValue(static_cast<int>(RentalStatus::Reserved));
    query.addBindValue(static_cast<int>(RentalStatus::Active));
    query.addBindValue(iso(end));
    query.addBindValue(iso(start));
    query.addBindValue(static_cast<int>(RentalStatus::Active));
    query.addBindValue(iso(today.addDays(1)));
    return !query.exec() || !query.next() || query.value(0).toInt() > 0; // hata da "çakışma" sayılır
}

Result RentalRepository::reserve(Rental rental, const QDate &today)
{
    const QStringList dateProblems = Rules::rentalProblems(rental.startDate, rental.endDate, today);
    if (!dateProblems.isEmpty())
        return Result::failure(dateProblems.first());
    if (rental.deposit < 0 || rental.deposit > MAX_DEPOSIT)
        return Result::failure("invalid_deposit");
    if (rental.notes.size() > MAX_NOTES)
        return Result::failure("notes_too_long");

    const std::optional<Vehicle> vehicle = VehicleRepository(m_db).find(rental.vehicleId);
    if (!vehicle)
        return Result::failure("not_found");
    if (vehicle->status == VehicleStatus::Maintenance)
        return Result::failure("vehicle_in_maintenance");
    const std::optional<Customer> customer = CustomerRepository(m_db).find(rental.customerId);
    if (!customer)
        return Result::failure("not_found");
    // Yaş ve ehliyet kuralları teslim gününe göre kontrol edilir
    const QStringList customerProblems = Rules::customerProblems(*customer, rental.startDate);
    if (!customerProblems.isEmpty())
        return Result::failure(customerProblems.first());

    Transaction transaction(m_db.connection());
    // Çakışma kontrolü ve kayıt aynı işlemde: arada başka bir rezervasyon araya giremez
    if (hasOverlap(rental.vehicleId, rental.startDate, rental.endDate, today))
        return Result::failure("vehicle_not_available");

    const Pricing::Quote quote = Pricing::quote(vehicle->dailyPrice, rental.startDate, rental.endDate);
    QSqlQuery query(m_db.connection());
    query.prepare("INSERT INTO rentals (vehicle_id, customer_id, start_date, end_date, return_date, start_km, "
                  "end_km, fuel_out, fuel_in, daily_price, total, extra_fees, deposit, status, notes) "
                  "VALUES (?, ?, ?, ?, NULL, ?, ?, ?, ?, ?, ?, 0, ?, ?, ?)");
    query.addBindValue(rental.vehicleId);
    query.addBindValue(rental.customerId);
    query.addBindValue(iso(rental.startDate));
    query.addBindValue(iso(rental.endDate));
    query.addBindValue(vehicle->mileage);
    query.addBindValue(vehicle->mileage);
    query.addBindValue(FUEL_FULL);
    query.addBindValue(FUEL_FULL);
    query.addBindValue(vehicle->dailyPrice);
    query.addBindValue(quote.total);
    query.addBindValue(rental.deposit);
    query.addBindValue(static_cast<int>(RentalStatus::Reserved));
    query.addBindValue(text(rental.notes.trimmed()));
    if (!query.exec() || !transaction.commit())
        return Result::failure("save_failed");
    return {QString(), query.lastInsertId().toLongLong()};
}

Result RentalRepository::pickUp(qint64 id, int startKm, int fuelOut, const QDate &today)
{
    const std::optional<Rental> rental = find(id);
    if (!rental || rental->status != RentalStatus::Reserved)
        return Result::failure("invalid_state");
    if (rental->startDate > today)
        return Result::failure("not_started_yet");
    const std::optional<Vehicle> vehicle = VehicleRepository(m_db).find(rental->vehicleId);
    if (!vehicle || vehicle->status != VehicleStatus::Available)
        return Result::failure("vehicle_not_available");
    if (startKm < vehicle->mileage)
        return Result::failure("km_below_odometer");
    if (fuelOut < 0 || fuelOut > FUEL_FULL)
        return Result::failure("invalid_fuel");

    Transaction transaction(m_db.connection());
    QSqlQuery query(m_db.connection());
    query.prepare("UPDATE rentals SET status = ?, start_km = ?, end_km = ?, fuel_out = ? WHERE id = ? AND status = ?");
    query.addBindValue(static_cast<int>(RentalStatus::Active));
    query.addBindValue(startKm);
    query.addBindValue(startKm);
    query.addBindValue(fuelOut);
    query.addBindValue(id);
    query.addBindValue(static_cast<int>(RentalStatus::Reserved));
    if (!query.exec() || query.numRowsAffected() != 1
        || !setVehicle(m_db.connection(), rental->vehicleId, VehicleStatus::Rented, startKm)
        || !transaction.commit())
        return Result::failure("save_failed");
    return {QString(), id};
}

Result RentalRepository::giveBack(qint64 id, const QDate &returnDate, int endKm, int fuelIn)
{
    const std::optional<Rental> rental = find(id);
    if (!rental || rental->status != RentalStatus::Active)
        return Result::failure("invalid_state");
    if (!returnDate.isValid() || returnDate < rental->startDate)
        return Result::failure("invalid_dates");
    if (endKm < rental->startKm || endKm - rental->startKm > 100'000)
        return Result::failure("invalid_km");
    if (fuelIn < 0 || fuelIn > FUEL_FULL)
        return Result::failure("invalid_fuel");

    const Pricing::ReturnCharges charges = Pricing::returnCharges(*rental, returnDate, endKm, fuelIn);
    Transaction transaction(m_db.connection());
    QSqlQuery query(m_db.connection());
    query.prepare("UPDATE rentals SET status = ?, return_date = ?, end_km = ?, fuel_in = ?, extra_fees = ? "
                  "WHERE id = ? AND status = ?");
    query.addBindValue(static_cast<int>(RentalStatus::Returned));
    query.addBindValue(iso(returnDate));
    query.addBindValue(endKm);
    query.addBindValue(fuelIn);
    query.addBindValue(charges.total);
    query.addBindValue(id);
    query.addBindValue(static_cast<int>(RentalStatus::Active));
    if (!query.exec() || query.numRowsAffected() != 1
        || !setVehicle(m_db.connection(), rental->vehicleId, VehicleStatus::Available, endKm)
        || !transaction.commit())
        return Result::failure("save_failed");
    return {QString(), id};
}

Result RentalRepository::cancel(qint64 id)
{
    // Sadece henüz teslim edilmemiş rezervasyon iptal edilir; aktif kiralama iade ile kapanır
    QSqlQuery query(m_db.connection());
    query.prepare("UPDATE rentals SET status = ? WHERE id = ? AND status = ?");
    query.addBindValue(static_cast<int>(RentalStatus::Cancelled));
    query.addBindValue(id);
    query.addBindValue(static_cast<int>(RentalStatus::Reserved));
    if (!query.exec() || query.numRowsAffected() != 1)
        return Result::failure("invalid_state");
    return {QString(), id};
}
