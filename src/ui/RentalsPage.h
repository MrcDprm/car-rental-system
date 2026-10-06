#pragma once

#include "core/Models.h"

#include <QHash>
#include <QWidget>

class Database;
class QComboBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Kiralama listesi ve yaşam döngüsü düğmeleri: yeni, teslim et, iade al, iptal, sözleşme PDF'i, CSV.
// Düğmeler seçili kiralamanın durumuna göre açılır (ör. rezervasyon iade alınamaz).
class RentalsPage : public QWidget
{
    Q_OBJECT

public:
    RentalsPage(Database &db, QWidget *parent);

public slots:
    void refresh();

private:
    const Rental *selected() const;
    QStringList cells(const Rental &rental) const;
    void updateButtons();
    void newRental();
    void pickUp();
    void giveBack();
    void cancelRental();
    void printContract();
    void exportCsv();

    Database &m_db;
    QList<Rental> m_rentals;
    QHash<qint64, Vehicle> m_vehicles;   // kimlikten araca: tabloda plaka göstermek için
    QHash<qint64, Customer> m_customers;
    QLineEdit *m_search;
    QComboBox *m_filter;
    QTableWidget *m_table;
    QPushButton *m_pickUp, *m_giveBack, *m_cancel, *m_print;
};