#pragma once

#include "data/Database.h"

#include <QDate>
#include <QList>

// Özet ekranı ve raporlar için hesaplanan değerler. Sadece okur, hiçbir şey yazmaz.
struct Summary {
    int vehicles = 0;
    int available = 0;
    int rented = 0;
    int inMaintenance = 0;
    int pickUpsToday = 0;  // bugün teslim edilecek rezervasyonlar
    int returnsToday = 0;  // bugün dönmesi gereken kiralamalar
    int overdue = 0;       // dönüş tarihi geçmiş ama dönmemiş
    int serviceDue = 0;    // bakım km'sine 1.000 km'den az kalmış
};

struct VehicleStat {
    QString plate;
    QString name;
    int rentals = 0;
    qint64 revenue = 0;
};

class ReportRepository
{
public:
    explicit ReportRepository(Database &db) : m_db(db) {}

    Summary summary(const QDate &today = QDate::currentDate()) const;
    QList<qint64> monthlyRevenue(int year) const; // 12 ay, iade tarihine göre (kira + ek ücretler)
    QList<VehicleStat> topVehicles(int limit) const;
    QList<int> years() const; // içinde iade bulunan yıllar

private:
    int count(const QString &sql, const QVariantList &values) const;

    Database &m_db;
};