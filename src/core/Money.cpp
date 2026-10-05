#include "core/Money.h"

#include <QLocale>
#include <cmath>

namespace Money {

QString format(qint64 kurus, const QString &language)
{
    const bool turkish = language == "tr";
    const QLocale locale(turkish ? QLocale::Turkish : QLocale::English, turkish ? QLocale::Turkey : QLocale::UnitedStates);
    const QString sign = kurus < 0 ? "-" : "";
    const qint64 absolute = kurus < 0 ? -kurus : kurus;
    // Tam kısım binlik ayırıcıyla, kuruş her zaman iki hane: 125050 → "1.250,50"
    const QString amount = locale.toString(absolute / 100) + locale.decimalPoint()
                           + QString::number(absolute % 100).rightJustified(2, '0');
    return turkish ? sign + amount + " ₺" : sign + "₺" + amount;
}

qint64 fromLira(double lira)
{
    return std::llround(lira * 100.0);
}

double toLira(qint64 kurus)
{
    return static_cast<double>(kurus) / 100.0;
}

} // namespace Money