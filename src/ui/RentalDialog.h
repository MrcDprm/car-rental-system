#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class VehicleCardDelegate;

// Yeni kiralama: müşteri ve tarihler seçilir; o tarihlerde boş araçlar bütçe, sınıf, vites, yakıt,
// koltuk ve bagaj filtreleriyle kartlar hâlinde listelenir. Fiyat seçilen tarihlere göre hesaplanır.
// "Hemen teslim et" işaretliyse rezervasyon kaydedilip aynı anda teslim edilir (kapıya gelen müşteri).
class RentalDialog : public QDialog
{
    Q_OBJECT

public:
    RentalDialog(Database &db, QWidget *parent);

private:
    void loadVehicles(); // tarih değişince veritabanından boş araçlar
    void applyFilters(); // filtre değişince sadece liste süzülür
    qint64 selectedVehicle() const;
    void updateQuote();
    void save();

    Database &m_db;
    QList<Vehicle> m_free;
    QComboBox *m_customer;
    QDateEdit *m_start, *m_end;
    QSpinBox *m_budget, *m_minSeats, *m_minLuggage;
    QComboBox *m_class, *m_transmission, *m_fuel, *m_sort;
    QLabel *m_count;
    QListWidget *m_cards;
    VehicleCardDelegate *m_delegate;
    QLabel *m_quote, *m_error;
    QDoubleSpinBox *m_deposit;
    QLineEdit *m_notes;
    QCheckBox *m_pickUpNow;
};