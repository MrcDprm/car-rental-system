#pragma once

#include <QDate>
#include <QString>

// Uygulamanın temel kayıtları. Para her yerde kuruş olarak (tam sayı) tutulur:
// 1.250,50 TL = 125050. Ondalık sayılarla para tutmak yuvarlama hatası getirir.

enum class VehicleClass { Economy, Compact, Midsize, Suv, Van, Luxury };
enum class Transmission { Manual, Automatic };
enum class Fuel { Petrol, Diesel, Hybrid, Electric, Lpg };
enum class VehicleStatus { Available, Rented, Maintenance };
enum class RentalStatus { Reserved, Active, Returned, Cancelled };
enum class BodyType { Hatchback, Sedan, Station, Suv, Minivan, Coupe, Pickup };
enum class CarColor { White, Black, Gray, Silver, Red, Blue, Green, Beige, Orange, Yellow, Brown };

// Donanım etiketleri tek bir sayının bitlerinde tutulur: Navigation | RearCamera = 0b11 = 3
namespace Feature {
constexpr int Navigation = 1 << 0;
constexpr int RearCamera = 1 << 1;
constexpr int CarPlay = 1 << 2;
constexpr int Isofix = 1 << 3;
constexpr int FourWheelDrive = 1 << 4;
constexpr int Sunroof = 1 << 5;
constexpr int CruiseControl = 1 << 6;
constexpr int HeatedSeats = 1 << 7;
constexpr int COUNT = 8;
constexpr int ALL = (1 << COUNT) - 1;
} // namespace Feature

constexpr int FUEL_FULL = 8; // yakıt seviyesi sekizde bir olarak tutulur (0 = boş, 8 = dolu)

struct Vehicle {
    qint64 id = 0;
    QString plate;
    QString brand;
    QString model;
    int year = 0;
    VehicleClass vehicleClass = VehicleClass::Economy;
    Transmission transmission = Transmission::Manual;
    Fuel fuel = Fuel::Petrol;
    int seats = 5;
    BodyType bodyType = BodyType::Hatchback;
    CarColor color = CarColor::White;
    int luggage = 2;  // büyük valiz sayısı
    int features = 0; // Feature bitleri
    QString photo;    // fotoğraf klasöründeki dosya adı; boşsa çizim gösterilir
    qint64 dailyPrice = 0; // kuruş
    int mileage = 0;
    int nextServiceKm = 0;
    VehicleStatus status = VehicleStatus::Available;
};

struct Customer {
    qint64 id = 0;
    QString fullName;
    QString phone;
    QString email;
    QString nationalId;
    QString licenseNumber;
    QDate birthDate;
    QDate licenseDate;
};

struct Rental {
    qint64 id = 0;
    qint64 vehicleId = 0;
    qint64 customerId = 0;
    QDate startDate;
    QDate endDate;     // planlanan dönüş günü
    QDate returnDate;  // gerçek dönüş; araç dönene kadar geçersiz (null)
    int startKm = 0;
    int endKm = 0;
    int fuelOut = FUEL_FULL;
    int fuelIn = FUEL_FULL;
    qint64 dailyPrice = 0; // kiralama anındaki fiyat; araç fiyatı sonradan değişse de bu kalır
    qint64 total = 0;      // gün ücreti (indirimli)
    qint64 extraFees = 0;  // gecikme, fazla km, eksik yakıt
    qint64 deposit = 0;
    RentalStatus status = RentalStatus::Reserved;
    QString notes;
};

struct Maintenance {
    qint64 id = 0;
    qint64 vehicleId = 0;
    QDate startDate;
    QDate endDate; // bakımdan çıkana kadar geçersiz
    QString description;
    qint64 cost = 0;
};
