#pragma once

#include "core/Models.h"

#include <QWidget>

class Database;
class QComboBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Filo listesi: arama ve durum filtresi, ekleme/düzenleme/silme, bakım ve CSV'ye aktarma.
class FleetPage : public QWidget
{
    Q_OBJECT

public:
    FleetPage(Database &db, QWidget *parent);

public slots:
    void refresh();

private:
    void addVehicle();
    void editVehicle();
    void deleteVehicle();
    void maintenance();
    void exportCsv();
    void updateButtons();

    Database &m_db;
    QList<Vehicle> m_vehicles;
    QLineEdit *m_search;
    QComboBox *m_filter;
    QTableWidget *m_table;
    QPushButton *m_edit, *m_delete, *m_maintenance;
};