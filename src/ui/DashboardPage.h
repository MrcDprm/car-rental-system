#pragma once

#include "core/Models.h"

#include <QWidget>

class Database;
class QButtonGroup;
class QLabel;
class QPushButton;
class QTableWidget;

// Özet: filo durumu kartları ve altta bir liste. Hiçbir kart seçili değilken liste bugünün
// yapılacaklarıdır; bir karta tıklanınca o kartın araçları ya da kiralamaları listelenir ve
// seçili kayıt için işlem düğmeleri (teslim, iade, kirala, bakım…) görünür.
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    DashboardPage(Database &db, QWidget *parent);

public slots:
    void refresh();

private:
    enum class View { Todo, AllVehicles, Available, Rented, Maintenance, PickUps, ReturnsToday, Overdue, ServiceDue };

    void selectCard(int index);
    bool showsVehicles() const;
    void fillVehicles();
    void fillRentals();
    void updateButtons();
    const Vehicle *selectedVehicle() const;
    const Rental *selectedRental() const;
    void rent();
    void editVehicle();
    void maintenance();
    void pickUp();
    void giveBack();
    void printContract();

    Database &m_db;
    View m_view = View::Todo;
    QList<Vehicle> m_vehicles;
    QList<Rental> m_rentals;
    QList<QLabel *> m_values;
    QButtonGroup *m_cards;
    QLabel *m_listTitle, *m_empty;
    QTableWidget *m_table;
    QPushButton *m_rent, *m_edit, *m_maintenance, *m_pickUp, *m_giveBack, *m_print;
};
