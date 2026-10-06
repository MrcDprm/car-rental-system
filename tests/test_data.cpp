#include "data/CustomerRepository.h"
#include "core/Rules.h"
#include "data/Database.h"
#include "data/MaintenanceRepository.h"
#include "data/RentalRepository.h"
#include "data/ReportRepository.h"
#include "data/VehicleRepository.h"
#include "services/DemoData.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

// Her test kendi geçici veritabanıyla başlar; gerçek kullanıcı verisine dokunulmaz.
class TestData : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    std::unique_ptr<Database> m_db;
    const QDate m_today{2026, 10, 10};
    int m_counter = 0;

    Vehicle vehicle(const QString &plate, qint64 price = 150000)
    {
        Vehicle v;
        v.plate = plate;
        v.brand = "Renault";
        v.model = "Clio";
        v.year = 2024;
        v.dailyPrice = price;
        v.mileage = 10000;
        v.nextServiceKm = 25000;
        return v;
    }

    Customer customer(const QString &nationalId = "10000000146")
    {
        return {0, "Ayşe Yılmaz", "0532 123 45 67", "AYSE@ornek.com", nationalId, "b123456",
                QDate(1995, 5, 5), QDate(2015, 5, 5)};
    }

    qint64 addVehicle(const QString &plate = "34 ABC 123")
    {
        return VehicleRepository(*m_db).add(vehicle(plate), m_today).id;
    }

    qint64 addCustomer()
    {
        return CustomerRepository(*m_db).add(customer(), m_today).id;
    }

    Result reserve(qint64 vehicleId, qint64 customerId, int fromDay, int toDay)
    {
        Rental r;
        r.vehicleId = vehicleId;
        r.customerId = customerId;
        r.startDate = m_today.addDays(fromDay);
        r.endDate = m_today.addDays(toDay);
        return RentalRepository(*m_db).reserve(r, m_today);
    }

private slots:
    void init()
    {
        m_db = std::make_unique<Database>();
        QVERIFY(m_db->open(m_dir.filePath(QString("test%1.db").arg(++m_counter))));
    }

    void cleanup() { m_db.reset(); }

    void vehiclesAreValidatedAndUnique()
    {
        VehicleRepository repo(*m_db);
        const Result added = repo.add(vehicle("34abc123"), m_today);
        QVERIFY(added.ok());
        QCOMPARE(repo.find(added.id)->plate, QString("34 ABC 123")); // tek biçime getirildi
        QCOMPARE(repo.add(vehicle("34 ABC 123"), m_today).error, QString("duplicate_plate"));
        QCOMPARE(repo.add(vehicle("99 X 1"), m_today).error, QString("invalid_plate"));
        QCOMPARE(repo.add(vehicle("06 A 11", 0), m_today).error, QString("invalid_price"));
        QCOMPARE(repo.all().size(), 1);
    }

    void sqlInjectionIsHarmless()
    {
        VehicleRepository repo(*m_db);
        Vehicle v = vehicle("35 AB 12");
        v.brand = "x'); DROP TABLE vehicles; --";
        QVERIFY(repo.add(v, m_today).ok());
        QCOMPARE(repo.all().first().brand, v.brand); // metin olarak kaydedildi, tablo yerinde
    }

    void customersAreCleanedAndChecked()
    {
        CustomerRepository repo(*m_db);
        const Result added = repo.add(customer(), m_today);
        QVERIFY(added.ok());
        const Customer saved = *repo.find(added.id);
        QCOMPARE(saved.phone, QString("5321234567"));
        QCOMPARE(saved.email, QString("ayse@ornek.com"));
        QCOMPARE(saved.licenseNumber, QString("B123456"));
        QCOMPARE(repo.add(customer(), m_today).error, QString("duplicate_national_id"));
        Customer young = customer("10000000078");
        young.birthDate = QDate(2010, 1, 1);
        QCOMPARE(repo.add(young, m_today).error, QString("too_young"));
    }

    void overlappingReservationsAreRejected()
    {
        const qint64 car = addVehicle();
        const qint64 person = addCustomer();
        QVERIFY(reserve(car, person, 1, 5).ok());
        QCOMPARE(reserve(car, person, 3, 7).error, QString("vehicle_not_available"));
        QCOMPARE(reserve(car, person, 0, 2).error, QString("vehicle_not_available"));
        QVERIFY(reserve(car, person, 5, 8).ok()); // aynı gün devir
        QCOMPARE(reserve(car, person, -1, 2).error, QString("start_in_past"));

        VehicleRepository vehicles(*m_db);
        QCOMPARE(vehicles.availableBetween(m_today.addDays(2), m_today.addDays(4)).size(), 0);
        QCOMPARE(vehicles.availableBetween(m_today.addDays(8), m_today.addDays(9)).size(), 1);
    }

    void reservationPriceIsLocked()
    {
        const qint64 car = addVehicle();
        const Result r = reserve(car, addCustomer(), 0, 7);
        QCOMPARE(RentalRepository(*m_db).find(r.id)->total, 945000); // 7 gün × 1.500 TL − %10
        Vehicle changed = *VehicleRepository(*m_db).find(car);
        changed.dailyPrice = 999900;
        QVERIFY(VehicleRepository(*m_db).update(changed, m_today).ok());
        QCOMPARE(RentalRepository(*m_db).find(r.id)->total, 945000); // eski sözleşme değişmedi
    }

    void fullLifecycle()
    {
        const qint64 car = addVehicle();
        RentalRepository rentals(*m_db);
        const qint64 id = reserve(car, addCustomer(), 0, 3).id;

        QCOMPARE(rentals.pickUp(id, 9000, 8, m_today).error, QString("km_below_odometer"));
        QVERIFY(rentals.pickUp(id, 10050, 8, m_today).ok());
        QCOMPARE(VehicleRepository(*m_db).find(car)->status, VehicleStatus::Rented);
        QCOMPARE(rentals.pickUp(id, 10050, 8, m_today).error, QString("invalid_state")); // iki kez teslim yok
        QCOMPARE(rentals.cancel(id).error, QString("invalid_state")); // aktif kiralama iptal edilmez

        QCOMPARE(ReportRepository(*m_db).summary(m_today.addDays(4)).overdue, 1);
        QVERIFY(rentals.giveBack(id, m_today.addDays(4), 11500, 6).ok()); // 1 gün geç, 1.450 km, 2/8 eksik
        const Rental done = *rentals.find(id);
        QCOMPARE(done.status, RentalStatus::Returned);
        QCOMPARE(done.extraFees, 225000 + 25000 * 2 + 250 * 500); // gecikme + yakıt + 250 fazla km
        const Vehicle back = *VehicleRepository(*m_db).find(car);
        QCOMPARE(back.status, VehicleStatus::Available);
        QCOMPARE(back.mileage, 11500);

        const QList<qint64> months = ReportRepository(*m_db).monthlyRevenue(2026);
        QCOMPARE(months[9], done.total + done.extraFees);
        QCOMPARE(ReportRepository(*m_db).topVehicles(5).first().rentals, 1);
        QCOMPARE(VehicleRepository(*m_db).remove(car).error, QString("has_history"));
    }

    void futureReservationCannotBePickedUpEarly()
    {
        const qint64 id = reserve(addVehicle(), addCustomer(), 2, 4).id;
        QCOMPARE(RentalRepository(*m_db).pickUp(id, 10000, 8, m_today).error, QString("not_started_yet"));
        QVERIFY(RentalRepository(*m_db).cancel(id).ok());
        QCOMPARE(RentalRepository(*m_db).cancel(id).error, QString("invalid_state"));
    }

    void maintenanceBlocksRentals()
    {
        const qint64 car = addVehicle();
        MaintenanceRepository maintenance(*m_db);
        QCOMPARE(maintenance.start(car, "   ", m_today).error, QString("invalid_description"));
        QVERIFY(maintenance.start(car, "Periyodik bakım", m_today).ok());
        QCOMPARE(ReportRepository(*m_db).summary(m_today).inMaintenance, 1);
        QCOMPARE(reserve(car, addCustomer(), 0, 2).error, QString("vehicle_in_maintenance"));
        QCOMPARE(maintenance.finish(car, 350000, 9000, m_today).error, QString("invalid_service_km"));
        QVERIFY(maintenance.finish(car, 350000, 25000, m_today).ok());
        QCOMPARE(maintenance.forVehicle(car).first().cost, 350000);
        QCOMPARE(VehicleRepository(*m_db).find(car)->status, VehicleStatus::Available);
    }

    void corruptedValuesFallBack()
    {
        const qint64 car = addVehicle();
        QSqlQuery query(m_db->connection());
        query.exec(QString("UPDATE vehicles SET status = 99, fuel = -5 WHERE id = %1").arg(car));
        const Vehicle v = *VehicleRepository(*m_db).find(car);
        QCOMPARE(v.status, VehicleStatus::Available);
        QCOMPARE(v.fuel, Fuel::Petrol);

        query.exec(QString("UPDATE vehicles SET body_type = 42, color = -1, features = 4096 + 3 WHERE id = %1").arg(car));
        const Vehicle broken = *VehicleRepository(*m_db).find(car);
        QCOMPARE(broken.bodyType, BodyType::Hatchback);
        QCOMPARE(broken.color, CarColor::White);
        QCOMPARE(broken.features, 3); // bilinmeyen bit atıldı
    }

    void vehicleDetailsAreStored()
    {
        VehicleRepository repo(*m_db);
        Vehicle v = vehicle("34 SUV 77");
        v.bodyType = BodyType::Suv;
        v.color = CarColor::Blue;
        v.luggage = 4;
        v.features = Feature::Navigation | Feature::FourWheelDrive;
        v.photo = "0123456789abcdef0123456789abcdef.jpg";
        const Result added = repo.add(v, m_today);
        QVERIFY(added.ok());
        const Vehicle stored = *repo.find(added.id);
        QCOMPARE(stored.bodyType, BodyType::Suv);
        QCOMPARE(stored.color, CarColor::Blue);
        QCOMPARE(stored.luggage, 4);
        QCOMPARE(stored.features, Feature::Navigation | Feature::FourWheelDrive);
        QCOMPARE(stored.photo, v.photo);

        v.plate = "34 SUV 78";
        v.luggage = 12;
        QCOMPARE(repo.add(v, m_today).error, QString("invalid_luggage"));
        v.luggage = 2;
        v.features = 1 << 12;
        QCOMPARE(repo.add(v, m_today).error, QString("invalid_features"));
        v.features = 0;
        // Fotoğraf adı yol içeremez: klasör dışındaki bir dosya gösterilemez ya da silinemez
        for (const QString &name : {"../../secret.jpg", "C:/Windows/x.jpg", "abc.jpg", "0123456789ABCDEF0123456789ABCDEF.jpg"}) {
            v.photo = name;
            QCOMPARE(repo.add(v, m_today).error, QString("invalid_photo"));
        }
    }

    void demoDataIsConsistent()
    {
        QVERIFY(DemoData::isEmpty(*m_db));
        QVERIFY(DemoData::load(*m_db, m_today));
        QVERIFY(!DemoData::isEmpty(*m_db));
        QVERIFY(!DemoData::load(*m_db, m_today)); // dolu veritabanına ikinci kez yüklenmez

        const QList<Vehicle> vehicles = VehicleRepository(*m_db).all();
        QVERIFY(vehicles.size() >= 100);
        QCOMPARE(CustomerRepository(*m_db).all().size(), 25);
        qint64 cheapest = vehicles.first().dailyPrice, priciest = cheapest;
        for (const Vehicle &v : vehicles) {
            QVERIFY2(Rules::vehicleProblems(v, m_today).isEmpty(), qPrintable(v.plate));
            cheapest = std::min(cheapest, v.dailyPrice);
            priciest = std::max(priciest, v.dailyPrice);
        }
        QVERIFY(cheapest <= 1000'00 && priciest >= 10000'00); // her bütçeye araç var

        // Her aracın durumu kiralamalarıyla tutarlı; açık kiralamalar ve geçmiş birbiriyle çakışmaz
        const QList<Rental> rentals = RentalRepository(*m_db).all();
        QVERIFY(rentals.size() > 1000);
        for (const Vehicle &v : vehicles) {
            QList<Rental> own;
            int active = 0;
            for (const Rental &r : rentals)
                if (r.vehicleId == v.id && r.status != RentalStatus::Cancelled) {
                    own << r;
                    active += r.status == RentalStatus::Active;
                }
            QCOMPARE(active, v.status == VehicleStatus::Rented ? 1 : 0);
            for (int a = 0; a < own.size(); ++a)
                for (int b = a + 1; b < own.size(); ++b) {
                    const QDate aEnd = own[a].returnDate.isValid() ? own[a].returnDate : own[a].endDate;
                    const QDate bEnd = own[b].returnDate.isValid() ? own[b].returnDate : own[b].endDate;
                    QVERIFY2(!Rules::overlaps(own[a].startDate, aEnd, own[b].startDate, bEnd), qPrintable(v.plate));
                }
        }
        const Summary s = ReportRepository(*m_db).summary(m_today);
        QVERIFY(s.rented > 0 && s.available > 0 && s.inMaintenance > 0 && s.overdue > 0 && s.pickUpsToday > 0
                && s.serviceDue > 0);
        int monthsWithRevenue = 0;
        for (qint64 revenue : ReportRepository(*m_db).monthlyRevenue(m_today.year()))
            monthsWithRevenue += revenue > 0;
        QVERIFY(monthsWithRevenue >= 9); // ekim ayına kadar her ayın geliri var
    }
};

QTEST_GUILESS_MAIN(TestData)
#include "test_data.moc"
