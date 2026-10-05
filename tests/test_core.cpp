#include "core/Money.h"
#include "core/Pricing.h"
#include "core/Rules.h"

#include <QTest>

class TestCore : public QObject
{
    Q_OBJECT

private slots:
    void money()
    {
        QCOMPARE(Money::format(125050, "tr"), QString("1.250,50 ₺"));
        QCOMPARE(Money::format(125050, "en"), QString("₺1,250.50"));
        QCOMPARE(Money::format(5, "tr"), QString("0,05 ₺"));
        QCOMPARE(Money::format(-1999, "en"), QString("-₺19.99"));
        QCOMPARE(Money::fromLira(1250.5), 125050);
        QCOMPARE(Money::fromLira(0.1 + 0.2), 30); // 0.30000000000000004 → 30 kuruş
    }

    void quote()
    {
        const QDate day(2026, 10, 10);
        Pricing::Quote q = Pricing::quote(150000, day, day.addDays(3));
        QCOMPARE(q.days, 3);
        QCOMPARE(q.total, 450000);
        q = Pricing::quote(150000, day, day); // aynı gün: bir gün
        QCOMPARE(q.days, 1);
        q = Pricing::quote(100000, day, day.addDays(7)); // haftalık %10
        QCOMPARE(q.discountPercent, 10);
        QCOMPARE(q.total, 630000);
        q = Pricing::quote(100000, day, day.addDays(30)); // aylık %20
        QCOMPARE(q.total, 2400000);
        QCOMPARE(Pricing::quote(100000, QDate(), day).days, 0);
    }

    void returnCharges()
    {
        Rental r;
        r.startDate = QDate(2026, 10, 1);
        r.endDate = QDate(2026, 10, 4); // 3 gün → 900 km hak
        r.dailyPrice = 100000;
        r.startKm = 10000;
        r.fuelOut = 8;
        auto c = Pricing::returnCharges(r, r.endDate, 10900, 8); // zamanında, km hakkı içinde, dolu
        QCOMPARE(c.total, 0);
        c = Pricing::returnCharges(r, r.endDate.addDays(2), 11800, 6); // 2 gün geç: 3 + 2 gün → 1500 km hak
        QCOMPARE(c.lateDays, 2);
        QCOMPARE(c.lateFee, 300000);
        QCOMPARE(c.extraKm, 300);
        QCOMPARE(c.extraKmFee, 150000);
        QCOMPARE(c.missingFuel, 2);
        QCOMPARE(c.total, 300000 + 150000 + 50000);
        c = Pricing::returnCharges(r, r.endDate.addDays(-1), 10100, 8); // erken dönüş: ücret yok, iade de yok
        QCOMPARE(c.total, 0);
    }

    void plates()
    {
        QCOMPARE(Rules::normalizePlate("34abc123"), QString("34 ABC 123"));
        QCOMPARE(Rules::normalizePlate(" 06-a-1234 "), QString("06 A 1234"));
        QVERIFY(Rules::isValidPlate("34 ABC 123"));
        QVERIFY(Rules::isValidPlate("81 AB 12"));
        QVERIFY(!Rules::isValidPlate("82 ABC 123"));
        QVERIFY(!Rules::isValidPlate("00 ABC 123"));
        QVERIFY(!Rules::isValidPlate("34 ABCD 123"));
        QVERIFY(!Rules::isValidPlate("34ABC123"));
    }

    void identity()
    {
        QVERIFY(Rules::isValidNationalId("10000000146"));
        QVERIFY(!Rules::isValidNationalId("10000000147"));
        QVERIFY(!Rules::isValidNationalId("01234567890"));
        QVERIFY(!Rules::isValidNationalId("1234"));
        QCOMPARE(Rules::normalizePhone("0 (532) 123 45 67"), QString("5321234567"));
        QCOMPARE(Rules::normalizePhone("+90 532 123 4567"), QString("5321234567"));
        QVERIFY(Rules::isValidPhone("5321234567"));
        QVERIFY(!Rules::isValidPhone("123"));
        QVERIFY(Rules::isValidEmail("ali@ornek.com"));
        QVERIFY(!Rules::isValidEmail("ali@ornek"));
    }

    void overlaps()
    {
        const QDate d(2026, 10, 1);
        QVERIFY(Rules::overlaps(d, d.addDays(5), d.addDays(4), d.addDays(8)));
        QVERIFY(Rules::overlaps(d, d.addDays(10), d.addDays(2), d.addDays(3)));
        QVERIFY(!Rules::overlaps(d, d.addDays(5), d.addDays(5), d.addDays(8))); // aynı gün devir
        QVERIFY(!Rules::overlaps(d.addDays(5), d.addDays(8), d, d.addDays(5)));
    }

    void customers()
    {
        const QDate today(2026, 10, 10);
        Customer c{0, "Ayşe Yılmaz", "5321234567", "", "10000000146", "B123456", QDate(2000, 1, 1), QDate(2020, 1, 1)};
        QVERIFY(Rules::customerProblems(c, today).isEmpty());
        Customer young = c;
        young.birthDate = QDate(2005, 10, 11); // yarın 21
        young.licenseDate = QDate(2023, 1, 1);
        QCOMPARE(Rules::customerProblems(young, today), QStringList{"too_young"});
        young.birthDate = QDate(2005, 10, 10); // bugün 21
        QVERIFY(Rules::customerProblems(young, today).isEmpty());
        Customer fresh = c;
        fresh.licenseDate = QDate(2025, 1, 1);
        QCOMPARE(Rules::customerProblems(fresh, today), QStringList{"license_too_new"});
        Customer bad{0, "A", "1", "x", "1", "", QDate(), QDate()};
        QCOMPARE(Rules::customerProblems(bad, today).size(), 7);
    }

    void vehicles()
    {
        const QDate today(2026, 10, 10);
        Vehicle v;
        v.plate = "34 ABC 123";
        v.brand = "Renault";
        v.model = "Clio";
        v.year = 2024;
        v.dailyPrice = 150000;
        v.mileage = 12000;
        v.nextServiceKm = 20000;
        QVERIFY(Rules::vehicleProblems(v, today).isEmpty());
        v.year = 2030;
        v.dailyPrice = 0;
        QCOMPARE(Rules::vehicleProblems(v, today), (QStringList{"invalid_year", "invalid_price"}));
    }

    void rentals()
    {
        const QDate today(2026, 10, 10);
        QVERIFY(Rules::rentalProblems(today, today.addDays(2), today).isEmpty());
        QCOMPARE(Rules::rentalProblems(today, today, today), QStringList{"invalid_dates"});
        QCOMPARE(Rules::rentalProblems(today.addDays(-1), today.addDays(2), today), QStringList{"start_in_past"});
        QCOMPARE(Rules::rentalProblems(today, today.addDays(400), today), QStringList{"too_long"});
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "test_core.moc"
