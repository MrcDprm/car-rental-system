#pragma once

#include "core/Models.h"

// Kiralama ücreti ve iadede eklenen ücretler. Tüm tutarlar kuruş.
namespace Pricing {

constexpr int WEEKLY_DAYS = 7;
constexpr int MONTHLY_DAYS = 30;
constexpr int WEEKLY_DISCOUNT = 10;  // yüzde
constexpr int MONTHLY_DISCOUNT = 20; // yüzde
constexpr int KM_PER_DAY = 300;      // günlük ücretsiz km
constexpr qint64 EXTRA_KM_FEE = 500; // aşan her km için 5 TL
constexpr int LATE_DAY_PERCENT = 150; // geciken her gün, günlük fiyatın %150'si
constexpr qint64 FUEL_FEE_PER_EIGHTH = 25000; // eksik her sekizde bir depo için 250 TL

struct Quote {
    int days = 0;
    qint64 base = 0;
    int discountPercent = 0;
    qint64 discount = 0;
    qint64 total = 0;
};

struct ReturnCharges {
    int lateDays = 0;
    qint64 lateFee = 0;
    int extraKm = 0;
    qint64 extraKmFee = 0;
    int missingFuel = 0; // sekizde bir
    qint64 fuelFee = 0;
    qint64 total = 0;
};

int rentalDays(const QDate &start, const QDate &end);
int discountPercent(int days);
Quote quote(qint64 dailyPrice, const QDate &start, const QDate &end);
ReturnCharges returnCharges(const Rental &rental, const QDate &returnDate, int endKm, int fuelIn);

} // namespace Pricing