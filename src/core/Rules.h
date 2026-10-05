#pragma once

#include "core/Models.h"

#include <QStringList>

// İş kuralları ve girdi doğrulama. Hatalar metin değil anahtar olarak döner ("too_young");
// ekranda gösterilecek metni arayüz, seçili dile göre seçer.
namespace Rules {

constexpr int MIN_AGE = 21;
constexpr int MIN_LICENSE_YEARS = 2;
constexpr int MIN_YEAR = 1990;

int fullYears(const QDate &from, const QDate &to);
QString normalizePlate(const QString &text);
bool isValidPlate(const QString &plate);
bool isValidNationalId(const QString &id);
bool isValidEmail(const QString &email);
QString normalizePhone(const QString &text);
bool isValidPhone(const QString &phone);

// Yarı açık aralıklar [başlangıç, bitiş): bir araç 10'unda dönüyorsa 10'unda yeniden kiralanabilir
bool overlaps(const QDate &aStart, const QDate &aEnd, const QDate &bStart, const QDate &bEnd);

QStringList vehicleProblems(const Vehicle &vehicle, const QDate &today);
QStringList customerProblems(const Customer &customer, const QDate &today);
QStringList rentalProblems(const QDate &start, const QDate &end, const QDate &today);

} // namespace Rules