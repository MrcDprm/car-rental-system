#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;

// Aracı bakıma alma (yapılacak iş) ya da bakımdan çıkarma (maliyet, sonraki bakım km'si) formu.
// Altta aracın bakım geçmişi listelenir.
class MaintenanceDialog : public QDialog
{
    Q_OBJECT

public:
    MaintenanceDialog(Database &db, const Vehicle &vehicle, QWidget *parent);

private:
    void save();

    Database &m_db;
    Vehicle m_vehicle;
    bool m_finishing;
    QLineEdit *m_description = nullptr;
    QDoubleSpinBox *m_cost = nullptr;
    QSpinBox *m_nextService = nullptr;
    QLabel *m_error;
};