#pragma once

#include "core/Models.h"

#include <QString>

// Arayüz metinleri (Türkçe / İngilizce). Metinler anahtarla istenir: I18n::t("fleet").
// {0}, {1}… yer tutucuları QString::arg ile doldurulur.
namespace I18n {

void setLanguage(const QString &language); // "tr" ya da "en"
QString language();

QString t(const char *key);
QString error(const QString &key); // Result ve Rules'tan gelen hata anahtarları

QString vehicleClass(VehicleClass value);
QString transmission(Transmission value);
QString fuel(Fuel value);
QString vehicleStatus(VehicleStatus value);
QString rentalStatus(RentalStatus value);
QString date(const QDate &value);
QString money(qint64 kurus);
QString number(int value);            // 12650 → "12.650" (TR) / "12,650" (EN)
QString phone(const QString &digits); // 5321234567 → "0532 123 45 67"

} // namespace I18n
