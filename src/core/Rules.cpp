#include "core/Rules.h"

#include <QRegularExpression>

namespace Rules {

constexpr int MAX_TEXT = 100;
constexpr qint64 MAX_DAILY_PRICE = 100'000'00; // 100.000 TL; daha büyüğü yazım hatasıdır
constexpr int MAX_MILEAGE = 2'000'000;
constexpr int MAX_RENTAL_DAYS = 365;
constexpr int MAX_LUGGAGE = 9;

int fullYears(const QDate &from, const QDate &to)
{
    // Doğum gününe göre tam yıl: 15 Mart 2005 → 14 Mart 2026'da 20, 15 Mart 2026'da 21
    int years = to.year() - from.year();
    if (to.month() < from.month() || (to.month() == from.month() && to.day() < from.day()))
        --years;
    return years;
}

QString normalizePlate(const QString &text)
{
    // "34abc123", "34 abc 123", "34-ABC-123" → "34 ABC 123"
    static const QRegularExpression parts("^(\\d{2})\\s*-?\\s*([A-Za-z]{1,3})\\s*-?\\s*(\\d{2,4})$");
    const QRegularExpressionMatch match = parts.match(text.trimmed());
    if (!match.hasMatch())
        return text.trimmed().toUpper();
    return match.captured(1) + " " + match.captured(2).toUpper() + " " + match.captured(3);
}

bool isValidPlate(const QString &plate)
{
    // İl kodu 01-81, 1-3 harf, 2-4 rakam
    static const QRegularExpression format("^(0[1-9]|[1-7][0-9]|8[01]) [A-Z]{1,3} \\d{2,4}$");
    return format.match(plate).hasMatch();
}

bool isValidNationalId(const QString &id)
{
    // T.C. kimlik numarası: 11 hane, ilk hane 0 değil, son iki hane sağlama basamağı
    static const QRegularExpression digits("^[1-9]\\d{10}$");
    if (!digits.match(id).hasMatch())
        return false;
    int d[11];
    for (int i = 0; i < 11; ++i)
        d[i] = id.at(i).digitValue();
    const int odd = d[0] + d[2] + d[4] + d[6] + d[8];
    const int even = d[1] + d[3] + d[5] + d[7];
    const int tenth = ((odd * 7 - even) % 10 + 10) % 10;
    int sum = 0;
    for (int i = 0; i < 10; ++i)
        sum += d[i];
    return d[9] == tenth && d[10] == sum % 10;
}

bool isValidEmail(const QString &email)
{
    static const QRegularExpression format("^[^@\\s]+@[^@\\s]+\\.[^@\\s]{2,}$");
    return email.size() <= MAX_TEXT && format.match(email).hasMatch();
}

QString normalizePhone(const QString &text)
{
    // Boşluk, tire ve parantezler atılır; +90 ya da baştaki 0 kaldırılır: "0 (532) 123 45 67" → "5321234567"
    QString digits;
    for (const QChar c : text)
        if (c.isDigit())
            digits += c;
    if (digits.startsWith("90") && digits.size() == 12)
        digits.remove(0, 2);
    if (digits.startsWith('0') && digits.size() == 11)
        digits.remove(0, 1);
    return digits;
}

bool isValidPhone(const QString &phone)
{
    static const QRegularExpression format("^[2-5]\\d{9}$");
    return format.match(phone).hasMatch();
}

bool isValidPhotoName(const QString &name)
{
    // Uygulamanın kendi verdiği ad: 32 onaltılık karakter + .jpg. Veritabanından gelen "../../x.exe"
    // gibi bir yol bu kalıba uymaz; böylece fotoğraf klasörünün dışındaki bir dosyaya hiç erişilmez.
    static const QRegularExpression format("^[0-9a-f]{32}\\.jpg$");
    return format.match(name).hasMatch();
}

bool overlaps(const QDate &aStart, const QDate &aEnd, const QDate &bStart, const QDate &bEnd)
{
    return aStart < bEnd && bStart < aEnd;
}

QStringList vehicleProblems(const Vehicle &vehicle, const QDate &today)
{
    QStringList problems;
    if (!isValidPlate(vehicle.plate))
        problems << "invalid_plate";
    if (vehicle.brand.trimmed().isEmpty() || vehicle.model.trimmed().isEmpty()
        || vehicle.brand.size() > MAX_TEXT || vehicle.model.size() > MAX_TEXT)
        problems << "missing_brand_model";
    if (vehicle.year < MIN_YEAR || vehicle.year > today.year() + 1)
        problems << "invalid_year";
    if (vehicle.seats < 2 || vehicle.seats > 9)
        problems << "invalid_seats";
    if (vehicle.dailyPrice <= 0 || vehicle.dailyPrice > MAX_DAILY_PRICE)
        problems << "invalid_price";
    if (vehicle.mileage < 0 || vehicle.mileage > MAX_MILEAGE)
        problems << "invalid_mileage";
    if (vehicle.nextServiceKm < 0 || vehicle.nextServiceKm > MAX_MILEAGE)
        problems << "invalid_service_km";
    if (vehicle.luggage < 0 || vehicle.luggage > MAX_LUGGAGE)
        problems << "invalid_luggage";
    if ((vehicle.features & ~Feature::ALL) != 0)
        problems << "invalid_features";
    if (!vehicle.photo.isEmpty() && !isValidPhotoName(vehicle.photo))
        problems << "invalid_photo";
    return problems;
}

QStringList customerProblems(const Customer &customer, const QDate &today)
{
    QStringList problems;
    if (customer.fullName.trimmed().size() < 3 || customer.fullName.size() > MAX_TEXT)
        problems << "invalid_name";
    if (!isValidPhone(customer.phone))
        problems << "invalid_phone";
    if (!customer.email.isEmpty() && !isValidEmail(customer.email))
        problems << "invalid_email";
    if (!isValidNationalId(customer.nationalId))
        problems << "invalid_national_id";
    if (customer.licenseNumber.trimmed().isEmpty() || customer.licenseNumber.size() > 20)
        problems << "invalid_license_number";
    if (!customer.birthDate.isValid() || customer.birthDate > today)
        problems << "invalid_birth_date";
    else if (fullYears(customer.birthDate, today) < MIN_AGE)
        problems << "too_young";
    if (!customer.licenseDate.isValid() || customer.licenseDate > today
        || (customer.birthDate.isValid() && customer.licenseDate < customer.birthDate.addYears(17)))
        problems << "invalid_license_date";
    else if (fullYears(customer.licenseDate, today) < MIN_LICENSE_YEARS)
        problems << "license_too_new";
    return problems;
}

QStringList rentalProblems(const QDate &start, const QDate &end, const QDate &today)
{
    QStringList problems;
    if (!start.isValid() || !end.isValid() || end <= start)
        problems << "invalid_dates";
    else if (start < today)
        problems << "start_in_past";
    else if (start.daysTo(end) > MAX_RENTAL_DAYS)
        problems << "too_long";
    return problems;
}

} // namespace Rules
