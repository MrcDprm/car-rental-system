#include "services/DemoData.h"

#include "core/Pricing.h"
#include "core/Rules.h"
#include "data/CustomerRepository.h"
#include "data/Database.h"
#include "data/VehicleRepository.h"

#include <QRandomGenerator>
#include <QSet>
#include <QSqlQuery>
#include <QVariant>

namespace {

constexpr int MANUAL = 1;
constexpr int AUTOMATIC = 2;

// Katalog: Türkiye'deki kiralama filolarında sık görülen modeller. Fiyat günlük TL, count filodaki adet.
struct Model {
    const char *brand, *model;
    VehicleClass vehicleClass;
    BodyType body;
    int seats, luggage, transmissions;
    QList<Fuel> fuels;
    int price, count;
    int extraFeatures; // modelde her zaman bulunan donanım (ör. 4x4)
};

const QList<Model> &catalog()
{
    using VC = VehicleClass;
    using BT = BodyType;
    static const QList<Model> models = {
        // Ekonomi
        {"Dacia", "Sandero", VC::Economy, BT::Hatchback, 5, 2, MANUAL, {Fuel::Petrol, Fuel::Lpg}, 950, 5, 0},
        {"Renault", "Clio", VC::Economy, BT::Hatchback, 5, 2, MANUAL | AUTOMATIC, {Fuel::Petrol, Fuel::Diesel}, 1100, 5, 0},
        {"Fiat", "Egea Hatchback", VC::Economy, BT::Hatchback, 5, 2, MANUAL, {Fuel::Petrol, Fuel::Diesel}, 1050, 4, 0},
        {"Citroën", "C3", VC::Economy, BT::Hatchback, 5, 2, MANUAL, {Fuel::Petrol}, 1050, 3, 0},
        {"Hyundai", "i20", VC::Economy, BT::Hatchback, 5, 2, MANUAL | AUTOMATIC, {Fuel::Petrol}, 1150, 3, 0},
        {"Opel", "Corsa", VC::Economy, BT::Hatchback, 5, 2, AUTOMATIC, {Fuel::Petrol}, 1200, 3, 0},
        {"Peugeot", "208", VC::Economy, BT::Hatchback, 5, 2, AUTOMATIC, {Fuel::Petrol}, 1250, 3, 0},
        {"Toyota", "Yaris", VC::Economy, BT::Hatchback, 5, 2, AUTOMATIC, {Fuel::Hybrid}, 1350, 3, 0},
        {"Volkswagen", "Polo", VC::Economy, BT::Hatchback, 5, 2, AUTOMATIC, {Fuel::Petrol}, 1300, 3, 0},
        // Kompakt
        {"Renault", "Taliant", VC::Compact, BT::Sedan, 5, 3, MANUAL | AUTOMATIC, {Fuel::Petrol, Fuel::Lpg}, 1150, 4, 0},
        {"Fiat", "Egea Sedan", VC::Compact, BT::Sedan, 5, 3, MANUAL | AUTOMATIC, {Fuel::Diesel, Fuel::Petrol}, 1250, 5, 0},
        {"Renault", "Megane Sedan", VC::Compact, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Diesel}, 1550, 3, 0},
        {"Hyundai", "Elantra", VC::Compact, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Petrol}, 1700, 2, 0},
        {"Toyota", "Corolla", VC::Compact, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Hybrid}, 1800, 4, 0},
        {"Honda", "Civic", VC::Compact, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Petrol}, 1900, 3, 0},
        {"Volkswagen", "Golf", VC::Compact, BT::Hatchback, 5, 2, AUTOMATIC, {Fuel::Petrol}, 1850, 3, 0},
        {"Opel", "Astra", VC::Compact, BT::Hatchback, 5, 2, AUTOMATIC, {Fuel::Diesel}, 1650, 2, 0},
        {"Skoda", "Octavia Combi", VC::Compact, BT::Station, 5, 4, AUTOMATIC, {Fuel::Diesel}, 1900, 2, 0},
        {"Peugeot", "308 SW", VC::Compact, BT::Station, 5, 4, AUTOMATIC, {Fuel::Diesel}, 1950, 2, 0},
        // Orta sınıf
        {"Volkswagen", "Passat", VC::Midsize, BT::Sedan, 5, 4, AUTOMATIC, {Fuel::Diesel}, 2600, 3, 0},
        {"Skoda", "Superb", VC::Midsize, BT::Sedan, 5, 4, AUTOMATIC, {Fuel::Diesel}, 2700, 2, 0},
        {"Peugeot", "508", VC::Midsize, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Diesel}, 2650, 2, 0},
        {"Tesla", "Model 3", VC::Midsize, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Electric}, 3400, 2, 0},
        // SUV ve pikap
        {"Dacia", "Duster", VC::Suv, BT::Suv, 5, 3, MANUAL, {Fuel::Diesel, Fuel::Lpg}, 1500, 3, 0},
        {"Renault", "Captur", VC::Suv, BT::Suv, 5, 3, AUTOMATIC, {Fuel::Petrol}, 1650, 3, 0},
        {"Nissan", "Qashqai", VC::Suv, BT::Suv, 5, 3, AUTOMATIC, {Fuel::Petrol}, 2200, 3, 0},
        {"Kia", "Sportage", VC::Suv, BT::Suv, 5, 3, AUTOMATIC, {Fuel::Diesel}, 2400, 2, 0},
        {"Hyundai", "Tucson", VC::Suv, BT::Suv, 5, 3, AUTOMATIC, {Fuel::Hybrid}, 2500, 3, 0},
        {"Peugeot", "3008", VC::Suv, BT::Suv, 5, 3, AUTOMATIC, {Fuel::Diesel}, 2600, 2, 0},
        {"Chery", "Tiggo 8 Pro", VC::Suv, BT::Suv, 7, 2, AUTOMATIC, {Fuel::Petrol}, 2500, 2, 0},
        {"BYD", "Atto 3", VC::Suv, BT::Suv, 5, 3, AUTOMATIC, {Fuel::Electric}, 2700, 2, 0},
        {"Togg", "T10X", VC::Suv, BT::Suv, 5, 3, AUTOMATIC, {Fuel::Electric}, 3300, 3, 0},
        {"Volkswagen", "Tiguan", VC::Suv, BT::Suv, 5, 4, AUTOMATIC, {Fuel::Diesel}, 2900, 2, 0},
        {"Toyota", "RAV4", VC::Suv, BT::Suv, 5, 4, AUTOMATIC, {Fuel::Hybrid}, 3000, 2, Feature::FourWheelDrive},
        {"Toyota", "Hilux", VC::Suv, BT::Pickup, 5, 4, MANUAL | AUTOMATIC, {Fuel::Diesel}, 2800, 2, Feature::FourWheelDrive},
        {"Ford", "Ranger", VC::Suv, BT::Pickup, 5, 4, AUTOMATIC, {Fuel::Diesel}, 3000, 1, Feature::FourWheelDrive},
        // Minivan
        {"Fiat", "Doblo", VC::Van, BT::Minivan, 5, 4, MANUAL, {Fuel::Diesel}, 1350, 2, 0},
        {"Citroën", "Berlingo", VC::Van, BT::Minivan, 5, 4, MANUAL, {Fuel::Diesel}, 1400, 2, 0},
        {"Ford", "Tourneo Custom", VC::Van, BT::Minivan, 9, 6, MANUAL | AUTOMATIC, {Fuel::Diesel}, 3200, 2, 0},
        {"Volkswagen", "Caravelle", VC::Van, BT::Minivan, 9, 6, AUTOMATIC, {Fuel::Diesel}, 3800, 2, 0},
        {"Mercedes-Benz", "Vito Tourer", VC::Van, BT::Minivan, 9, 5, AUTOMATIC, {Fuel::Diesel}, 4200, 1, 0},
        // Lüks
        {"BMW", "320i", VC::Luxury, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Petrol}, 3800, 2, 0},
        {"Mercedes-Benz", "C 200", VC::Luxury, BT::Sedan, 5, 3, AUTOMATIC, {Fuel::Petrol}, 4000, 2, 0},
        {"Audi", "A6", VC::Luxury, BT::Sedan, 5, 4, AUTOMATIC, {Fuel::Diesel}, 5000, 1, 0},
        {"BMW", "520i", VC::Luxury, BT::Sedan, 5, 4, AUTOMATIC, {Fuel::Petrol}, 5200, 2, 0},
        {"Mercedes-Benz", "E 200", VC::Luxury, BT::Sedan, 5, 4, AUTOMATIC, {Fuel::Petrol}, 5600, 1, 0},
        {"BMW", "420i Coupe", VC::Luxury, BT::Coupe, 4, 2, AUTOMATIC, {Fuel::Petrol}, 6000, 1, 0},
        {"Ford", "Mustang", VC::Luxury, BT::Coupe, 4, 2, AUTOMATIC, {Fuel::Petrol}, 7000, 1, 0},
        {"Volvo", "XC90", VC::Luxury, BT::Suv, 7, 4, AUTOMATIC, {Fuel::Hybrid}, 7500, 1, Feature::FourWheelDrive},
        {"Mercedes-Benz", "V 250", VC::Luxury, BT::Minivan, 7, 5, AUTOMATIC, {Fuel::Diesel}, 7800, 1, 0},
        {"BMW", "X5", VC::Luxury, BT::Suv, 5, 4, AUTOMATIC, {Fuel::Diesel}, 8500, 1, Feature::FourWheelDrive},
        {"Porsche", "911 Carrera", VC::Luxury, BT::Coupe, 4, 1, AUTOMATIC, {Fuel::Petrol}, 12000, 1, 0},
    };
    return models;
}

const char *const NAMES[] = {
    "Ayşe Yılmaz",   "Mehmet Kaya",   "Zeynep Demir",   "Can Öztürk",    "Elif Şahin",   "Burak Aydın",
    "Selin Arslan",  "Emre Koç",      "Deniz Çelik",    "Ece Kurt",      "Mert Polat",   "Gizem Yıldız",
    "Oğuzhan Erdem", "Buse Aksoy",    "Kerem Doğan",    "Merve Kılıç",   "Hakan Çetin",  "İrem Şimşek",
    "Tolga Özdemir", "Nazlı Aydoğan", "Serkan Yavuz",   "Ceren Güneş",   "Barış Tekin",  "Duygu Karaca",
    "John Miller",
};

class Generator
{
public:
    explicit Generator(quint32 seed) : m_rng(seed) {}
    int between(int low, int high) { return low + int(m_rng.bounded(high - low + 1)); } // iki uç dahil
    bool chance(int percent) { return int(m_rng.bounded(100)) < percent; }
    template <typename T> const T &pick(const QList<T> &items) { return items[int(m_rng.bounded(items.size()))]; }

private:
    QRandomGenerator m_rng;
};

QString nationalId(Generator &g)
{
    // Geçerli sağlama basamaklarıyla rastgele T.C. kimlik numarası (gerçek bir kişiye ait olması beklenmez)
    int d[11];
    d[0] = g.between(1, 9);
    for (int i = 1; i < 9; ++i)
        d[i] = g.between(0, 9);
    const int odd = d[0] + d[2] + d[4] + d[6] + d[8];
    const int even = d[1] + d[3] + d[5] + d[7];
    d[9] = ((odd * 7 - even) % 10 + 10) % 10;
    int sum = 0;
    for (int i = 0; i < 10; ++i)
        sum += d[i];
    d[10] = sum % 10;
    QString id;
    for (int digit : d)
        id += QChar('0' + digit);
    return id;
}

QString plate(Generator &g, QSet<QString> &used)
{
    static const QList<int> cities = {34, 34, 34, 6, 6, 35, 16, 7, 41, 1, 42, 27, 55, 48};
    static const QString letters = "ABCDEFGHJKLMNPRSTUVYZ";
    while (true) {
        const int letterCount = g.between(1, 3);
        QString middle;
        for (int i = 0; i < letterCount; ++i)
            middle += letters[g.between(0, letters.size() - 1)];
        const int digits = letterCount == 1 ? 4 : letterCount == 2 ? g.between(3, 4) : g.between(2, 3);
        int low = 1;
        for (int i = 1; i < digits; ++i)
            low *= 10;
        const QString text = QString("%1 %2 %3").arg(g.pick(cities), 2, 10, QChar('0')).arg(middle)
                                 .arg(g.between(low, low * 10 - 1));
        if (!used.contains(text)) {
            used.insert(text);
            return text;
        }
    }
}

CarColor color(Generator &g)
{
    // Filolarda en çok beyaz, gri ve siyah araç bulunur
    static const QList<CarColor> weighted = {
        CarColor::White, CarColor::White, CarColor::White, CarColor::White, CarColor::White, CarColor::White,
        CarColor::Gray,  CarColor::Gray,  CarColor::Gray,  CarColor::Black, CarColor::Black, CarColor::Black,
        CarColor::Silver, CarColor::Silver, CarColor::Blue, CarColor::Blue, CarColor::Red,  CarColor::Red,
        CarColor::Green, CarColor::Beige, CarColor::Orange, CarColor::Yellow, CarColor::Brown};
    return g.pick(weighted);
}

int features(Generator &g, VehicleClass vehicleClass)
{
    using namespace Feature;
    int flags = g.chance(90) ? Isofix : 0;
    switch (vehicleClass) {
    case VehicleClass::Economy:
        flags |= (g.chance(50) ? CarPlay : 0) | (g.chance(30) ? RearCamera : 0);
        break;
    case VehicleClass::Compact:
    case VehicleClass::Van:
        flags |= (g.chance(80) ? CarPlay : 0) | (g.chance(60) ? RearCamera : 0) | (g.chance(60) ? CruiseControl : 0);
        break;
    case VehicleClass::Midsize:
    case VehicleClass::Suv:
        flags |= CarPlay | RearCamera | CruiseControl | (g.chance(50) ? Navigation : 0) | (g.chance(20) ? Sunroof : 0)
                 | (g.chance(30) ? HeatedSeats : 0);
        break;
    case VehicleClass::Luxury:
        flags |= Navigation | RearCamera | CarPlay | CruiseControl | HeatedSeats | (g.chance(60) ? Sunroof : 0);
        break;
    }
    return flags;
}

bool exec(QSqlQuery &query, const QVariantList &values)
{
    for (const QVariant &value : values)
        query.addBindValue(value);
    return query.exec();
}

QString iso(const QDate &date)
{
    return date.toString(Qt::ISODate);
}

// Kiralamayı doğrudan yazar: geçmiş tarihli kayıtlar (iade edilmiş, iptal) repository kurallarına
// ("geçmişe rezervasyon yapılamaz") takılmasın diye. Tutarlar yine Pricing ile hesaplanır.
bool insertRental(QSqlDatabase db, const Rental &r)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO rentals (vehicle_id, customer_id, start_date, end_date, return_date, start_km, end_km, "
                  "fuel_out, fuel_in, daily_price, total, extra_fees, deposit, status, notes) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    return exec(query, {r.vehicleId, r.customerId, iso(r.startDate), iso(r.endDate),
                        r.returnDate.isValid() ? QVariant(iso(r.returnDate)) : QVariant(), r.startKm, r.endKm,
                        r.fuelOut, r.fuelIn, r.dailyPrice, r.total, r.extraFees, r.deposit, int(r.status),
                        text(r.notes)});
}

bool insertMaintenance(QSqlDatabase db, qint64 vehicleId, const QDate &start, const QDate &end,
                       const QString &description, qint64 cost)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO maintenance (vehicle_id, start_date, end_date, description, cost) VALUES (?, ?, ?, ?, ?)");
    return exec(query, {vehicleId, iso(start), end.isValid() ? QVariant(iso(end)) : QVariant(), description, cost});
}

qint64 depositFor(VehicleClass vehicleClass)
{
    static const qint64 deposits[] = {2000, 2500, 3500, 4000, 4000, 10000};
    return deposits[int(vehicleClass)] * 100;
}

} // namespace

namespace DemoData {

bool isEmpty(Database &db)
{
    QSqlQuery query(db.connection());
    return query.exec("SELECT (SELECT COUNT(*) FROM vehicles) + (SELECT COUNT(*) FROM customers)") && query.next()
           && query.value(0).toInt() == 0;
}

bool load(Database &db, const QDate &today)
{
    if (!isEmpty(db))
        return false;
    Generator g(2026); // sabit tohum: her yüklemede aynı filo
    QSqlDatabase connection = db.connection();
    // Binlerce kayıt tek işlemde yazılır: hem hızlı hem yarıda kalırsa hiçbiri yazılmaz
    Transaction transaction(connection);

    // Müşteriler
    CustomerRepository customers(db);
    QList<qint64> customerIds;
    for (const char *name : NAMES) {
        Customer c;
        c.fullName = QString::fromUtf8(name);
        c.phone = QString("05%1%2%3").arg(g.between(30, 55)).arg(g.between(100, 999)).arg(g.between(1000, 9999));
        if (g.chance(70)) {
            const QString first = c.fullName.section(' ', 0, 0).toLower().replace(u'ı', u'i');
            QString ascii;
            for (const QChar ch : first.normalized(QString::NormalizationForm_D))
                if (ch.unicode() < 128)
                    ascii += ch;
            c.email = ascii + "@example.com";
        }
        c.nationalId = nationalId(g);
        c.licenseNumber = QString::number(g.between(100000, 999999));
        c.birthDate = QDate(g.between(1962, 2001), g.between(1, 12), g.between(1, 28));
        // Ehliyet 18 yaştan sonra ve en az 2 yıl önce alınmış (kiralama kuralı)
        const int age = Rules::fullYears(c.birthDate, today);
        c.licenseDate = c.birthDate.addYears(g.between(18, std::min(26, age - 2)));
        const Result result = customers.add(c, today);
        if (!result.ok())
            return false;
        customerIds << result.id;
    }

    // Araçlar
    VehicleRepository vehicles(db);
    QSet<QString> plates;
    struct Car {
        Vehicle vehicle;
        int km;
        QDate free; // geçmiş kayıtlardan sonra aracın boşaldığı gün
    };
    QList<Car> cars;
    for (const Model &m : catalog()) {
        for (int i = 0; i < m.count; ++i) {
            Vehicle v;
            v.plate = plate(g, plates);
            v.brand = QString::fromUtf8(m.brand);
            v.model = QString::fromUtf8(m.model);
            v.year = g.between(today.year() - 4, today.year());
            v.vehicleClass = m.vehicleClass;
            v.bodyType = m.body;
            v.seats = m.seats;
            v.luggage = m.luggage;
            v.transmission = m.transmissions == (MANUAL | AUTOMATIC) ? (g.chance(50) ? Transmission::Manual : Transmission::Automatic)
                             : m.transmissions == MANUAL                ? Transmission::Manual
                                                                        : Transmission::Automatic;
            v.fuel = g.pick(m.fuels);
            v.color = color(g);
            v.features = features(g, m.vehicleClass) | m.extraFeatures;
            // Yeni model yılı biraz daha pahalı; fiyat 50 TL'ye yuvarlanır
            const int price = m.price * (100 + (v.year - today.year() + 2) * 4) / 100;
            v.dailyPrice = qint64((price + 25) / 50 * 50) * 100;
            v.mileage = 0;
            v.nextServiceKm = 15000;
            const Result result = vehicles.add(v, today);
            if (!result.ok())
                return false;
            v.id = result.id;
            // Geçmiş kiralamalardan önceki km: aracın yaşına göre
            cars << Car{v, (today.year() - v.year) * g.between(4000, 9000) + g.between(500, 3000), QDate()};
        }
    }

    // Son 12 ayın geçmişi: her araç için sırayla kiralamalar, aralarda bazen bakım
    const QDate historyStart = today.addDays(-365);
    const QStringList services = {"Periyodik bakım", "Lastik değişimi", "Fren balatası", "Yağ ve filtre değişimi",
                                  "Klima bakımı", "Ön cam değişimi"};
    for (Car &car : cars) {
        const Vehicle &v = car.vehicle;
        QDate day = historyStart.addDays(g.between(0, 20));
        while (true) {
            const int length = g.chance(40) ? g.between(1, 3) : g.chance(70) ? g.between(4, 7) : g.between(8, 21);
            Rental r;
            r.vehicleId = v.id;
            r.customerId = g.pick(customerIds);
            r.startDate = day;
            r.endDate = day.addDays(length);
            if (r.endDate >= today.addDays(-1)) {
                car.free = day;
                break;
            }
            r.dailyPrice = v.dailyPrice;
            r.total = Pricing::quote(v.dailyPrice, r.startDate, r.endDate).total;
            r.deposit = depositFor(v.vehicleClass);
            if (g.chance(4)) {
                r.status = RentalStatus::Cancelled; // gelmeyen müşteri
            } else {
                r.status = RentalStatus::Returned;
                r.startKm = car.km;
                const int driven = length * g.between(50, 260) + (g.chance(10) ? g.between(200, 900) : 0);
                r.returnDate = g.chance(8) ? r.endDate.addDays(g.between(1, 2)) : r.endDate;
                r.fuelOut = FUEL_FULL;
                const int fuelIn = g.chance(85) ? FUEL_FULL : g.between(4, 7);
                const Pricing::ReturnCharges charges = Pricing::returnCharges(r, r.returnDate, car.km + driven, fuelIn);
                r.endKm = car.km + driven;
                r.fuelIn = fuelIn;
                r.extraFees = charges.total;
                car.km += driven;
            }
            if (!insertRental(connection, r))
                return false;
            day = (r.returnDate.isValid() ? r.returnDate : r.endDate).addDays(g.chance(60) ? g.between(0, 3) : g.between(4, 14));
            // Uzun boşluklarda ara sıra bakım
            if (g.chance(12) && day.addDays(3) < today.addDays(-1)) {
                const int length = g.between(1, 3);
                if (!insertMaintenance(connection, v.id, day, day.addDays(length), g.pick(services),
                                       qint64(g.between(15, 120)) * 100'00))
                    return false;
                day = day.addDays(length + g.between(0, 2));
            }
        }
    }

    // Bugünkü durum: kirada, bugün dönecek, geciken, teslim bekleyen, ileri rezervasyon, bakımda
    for (int i = 0; i < cars.size(); ++i) {
        Car &car = cars[i];
        Vehicle &v = car.vehicle;
        VehicleStatus status = VehicleStatus::Available;
        Rental r;
        r.vehicleId = v.id;
        r.customerId = g.pick(customerIds);
        r.dailyPrice = v.dailyPrice;
        r.deposit = depositFor(v.vehicleClass);
        r.startKm = r.endKm = car.km;
        const int roll = g.between(0, 99);
        if (i % 40 == 7) {
            // Bakımda
            if (!insertMaintenance(connection, v.id, today.addDays(-g.between(0, 2)), QDate(), g.pick(services), 0))
                return false;
            status = VehicleStatus::Maintenance;
        } else if (roll < 42 || i % 30 == 3) {
            // Kirada: birkaç tanesi bugün dönecek, birkaçı gecikmiş. Başlangıç, geçmiş kayıtlarla çakışmaz.
            r.status = RentalStatus::Active;
            const bool overdue = i % 30 == 3 && car.free <= today.addDays(-4);
            r.startDate = std::min(today, std::max(car.free, today.addDays(-g.between(overdue ? 4 : 1, 7))));
            r.endDate = overdue ? today.addDays(-g.between(1, 2)) : i % 25 == 5 ? today : today.addDays(g.between(1, 9));
            if (r.endDate <= r.startDate)
                r.endDate = r.startDate.addDays(1);
            status = VehicleStatus::Rented;
        } else if (roll < 60) {
            // Rezervasyon: bazıları bugün teslim edilecek
            r.status = RentalStatus::Reserved;
            r.startDate = std::max(car.free, roll < 46 ? today : today.addDays(g.between(1, 25)));
            r.endDate = r.startDate.addDays(g.between(2, 10));
        }
        if (r.startDate.isValid()) {
            r.total = Pricing::quote(v.dailyPrice, r.startDate, r.endDate).total;
            if (!insertRental(connection, r))
                return false;
        }
        // Son km, durum ve bakım km'si; araçların bir kısmının bakımı yaklaşmış olsun
        const int toService = i % 17 == 4 ? g.between(100, 900) : g.between(2000, 15000);
        QSqlQuery update(connection);
        update.prepare("UPDATE vehicles SET mileage = ?, next_service_km = ?, status = ? WHERE id = ?");
        if (!exec(update, {car.km, car.km + toService, int(status), v.id}))
            return false;
    }
    return transaction.commit();
}

} // namespace DemoData