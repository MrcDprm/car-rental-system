#include "core/Pricing.h"

#include <algorithm>

namespace Pricing {

int rentalDays(const QDate &start, const QDate &end)
{
    // Aynı gün teslim alınıp bırakılsa bile en az bir gün ücretlendirilir
    if (!start.isValid() || !end.isValid())
        return 0;
    return std::max<qint64>(1, start.daysTo(end));
}

int discountPercent(int days)
{
    if (days >= MONTHLY_DAYS)
        return MONTHLY_DISCOUNT;
    if (days >= WEEKLY_DAYS)
        return WEEKLY_DISCOUNT;
    return 0;
}

Quote quote(qint64 dailyPrice, const QDate &start, const QDate &end)
{
    Quote result;
    result.days = rentalDays(start, end);
    result.base = dailyPrice * result.days;
    result.discountPercent = discountPercent(result.days);
    result.discount = result.base * result.discountPercent / 100; // tam sayı bölme: kuruşun altı atılır
    result.total = result.base - result.discount;
    return result;
}

ReturnCharges returnCharges(const Rental &rental, const QDate &returnDate, int endKm, int fuelIn)
{
    ReturnCharges result;
    result.lateDays = std::max<qint64>(0, rental.endDate.daysTo(returnDate));
    result.lateFee = rental.dailyPrice * LATE_DAY_PERCENT / 100 * result.lateDays;

    // Ücretsiz km hakkı planlanan günlere göre; geciken günler de hakka eklenir
    const int allowedKm = (rentalDays(rental.startDate, rental.endDate) + result.lateDays) * KM_PER_DAY;
    result.extraKm = std::max(0, (endKm - rental.startKm) - allowedKm);
    result.extraKmFee = result.extraKm * EXTRA_KM_FEE;

    result.missingFuel = std::max(0, rental.fuelOut - fuelIn);
    result.fuelFee = result.missingFuel * FUEL_FEE_PER_EIGHTH;

    result.total = result.lateFee + result.extraKmFee + result.fuelFee;
    return result;
}

} // namespace Pricing