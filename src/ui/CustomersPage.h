#pragma once

#include "core/Models.h"

#include <QWidget>

class Database;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Müşteri listesi: arama, ekleme/düzenleme/silme ve CSV'ye aktarma.
class CustomersPage : public QWidget
{
    Q_OBJECT

public:
    CustomersPage(Database &db, QWidget *parent);

public slots:
    void refresh();

private:
    void addCustomer();
    void editCustomer();
    void deleteCustomer();
    void exportCsv();

    Database &m_db;
    QList<Customer> m_customers;
    QLineEdit *m_search;
    QTableWidget *m_table;
    QPushButton *m_edit, *m_delete;
};