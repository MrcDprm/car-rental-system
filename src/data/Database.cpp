#include "data/Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {

// Her tablo ayrı bir komut; QSqlQuery tek seferde tek komut çalıştırır
const char *const SCHEMA[] = {
    R"(CREATE TABLE IF NOT EXISTS vehicles (
        id INTEGER PRIMARY KEY,
        plate TEXT NOT NULL UNIQUE,
        brand TEXT NOT NULL,
        model TEXT NOT NULL,
        year INTEGER NOT NULL,
        vehicle_class INTEGER NOT NULL,
        transmission INTEGER NOT NULL,
        fuel INTEGER NOT NULL,
        seats INTEGER NOT NULL,
        body_type INTEGER NOT NULL,
        color INTEGER NOT NULL,
        luggage INTEGER NOT NULL,
        features INTEGER NOT NULL,
        photo TEXT NOT NULL,
        daily_price INTEGER NOT NULL CHECK (daily_price > 0),
        mileage INTEGER NOT NULL CHECK (mileage >= 0),
        next_service_km INTEGER NOT NULL,
        status INTEGER NOT NULL))",
    R"(CREATE TABLE IF NOT EXISTS customers (
        id INTEGER PRIMARY KEY,
        full_name TEXT NOT NULL,
        phone TEXT NOT NULL,
        email TEXT NOT NULL,
        national_id TEXT NOT NULL UNIQUE,
        license_number TEXT NOT NULL,
        birth_date TEXT NOT NULL,
        license_date TEXT NOT NULL))",
    R"(CREATE TABLE IF NOT EXISTS rentals (
        id INTEGER PRIMARY KEY,
        vehicle_id INTEGER NOT NULL REFERENCES vehicles(id),
        customer_id INTEGER NOT NULL REFERENCES customers(id),
        start_date TEXT NOT NULL,
        end_date TEXT NOT NULL,
        return_date TEXT,
        start_km INTEGER NOT NULL,
        end_km INTEGER NOT NULL,
        fuel_out INTEGER NOT NULL,
        fuel_in INTEGER NOT NULL,
        daily_price INTEGER NOT NULL,
        total INTEGER NOT NULL,
        extra_fees INTEGER NOT NULL,
        deposit INTEGER NOT NULL,
        status INTEGER NOT NULL,
        notes TEXT NOT NULL,
        CHECK (end_date > start_date)))",
    R"(CREATE TABLE IF NOT EXISTS maintenance (
        id INTEGER PRIMARY KEY,
        vehicle_id INTEGER NOT NULL REFERENCES vehicles(id),
        start_date TEXT NOT NULL,
        end_date TEXT,
        description TEXT NOT NULL,
        cost INTEGER NOT NULL))",
    "CREATE INDEX IF NOT EXISTS rentals_vehicle ON rentals(vehicle_id, start_date, end_date)",
};

} // namespace

Database::Database()
    : m_name(QUuid::createUuid().toString()) // her Database nesnesinin kendi bağlantı adı olur
{
}

Database::~Database()
{
    QSqlDatabase::database(m_name, false).close();
    QSqlDatabase::removeDatabase(m_name);
}

bool Database::open(const QString &path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_name);
    db.setDatabaseName(path);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }
    QSqlQuery pragma(db);
    pragma.exec("PRAGMA foreign_keys = ON"); // SQLite'ta yabancı anahtar kontrolü varsayılan olarak kapalıdır
    return createSchema();
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_name, false);
}

bool Database::createSchema()
{
    QSqlQuery query(connection());
    for (const char *statement : SCHEMA) {
        if (!query.exec(statement)) {
            m_lastError = query.lastError().text();
            return false;
        }
    }
    return true;
}
