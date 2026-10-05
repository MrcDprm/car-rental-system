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