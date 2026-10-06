#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;

// Araç ekleme ve düzenleme formu. Kaydet'e basınca VehicleRepository kuralları kontrol eder;
// hata varsa form kapanmaz ve mesaj formun altında gösterilir.
class VehicleDialog : public QDialog
{
    Q_OBJECT

public:
    VehicleDialog(Database &db, const Vehicle &vehicle, QWidget *parent);

private:
    void save();

    Database &m_db;
    Vehicle m_vehicle;
    QLineEdit *m_plate, *m_brand, *m_model;
    QSpinBox *m_year, *m_seats, *m_mileage, *m_nextService;
    QComboBox *m_class, *m_transmission, *m_fuel;
    QDoubleSpinBox *m_price;
    QLabel *m_error;
};