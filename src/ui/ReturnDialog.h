#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QComboBox;
class QDateEdit;
class QLabel;
class QSpinBox;

// Araç iadesi: tarih, km ve yakıt girilir; gecikme, fazla km ve eksik yakıt ücretleri girerken hesaplanır.
class ReturnDialog : public QDialog
{
    Q_OBJECT

public:
    ReturnDialog(Database &db, const Rental &rental, QWidget *parent);

private:
    void updateCharges();
    void save();

    Database &m_db;
    Rental m_rental;
    QDateEdit *m_date;
    QSpinBox *m_km;
    QComboBox *m_fuel;
    QLabel *m_charges, *m_error;
};