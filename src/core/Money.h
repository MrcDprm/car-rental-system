#pragma once

#include <QString>

// Para kuruş olarak tutulur; ekranda dile göre yazılır: "1.250,50 ₺" ya da "₺1,250.50".
namespace Money {

QString format(qint64 kurus, const QString &language);
qint64 fromLira(double lira); // giriş kutusundan gelen değer → kuruş (en yakın kuruşa yuvarlanır)
double toLira(qint64 kurus);

} // namespace Money