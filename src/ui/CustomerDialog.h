#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QDateEdit;
class QLabel;
class QLineEdit;

// Müşteri ekleme ve düzenleme formu. Yaş (21+) ve ehliyet süresi (2 yıl+) kayıtta kontrol edilir.
class CustomerDialog : public QDialog
{
    Q_OBJECT

public:
    CustomerDialog(Database &db, const Customer &customer, QWidget *parent);

private:
    void save();

    Database &m_db;
    Customer m_customer;
    QLineEdit *m_name, *m_phone, *m_email, *m_nationalId, *m_license;
    QDateEdit *m_birthDate, *m_licenseDate;
    QLabel *m_error;
};