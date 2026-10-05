#pragma once

#include <QWidget>

class Database;
class QLabel;
class QTableWidget;

// Özet: filo durumu kartları ve bugünün yapılacakları (teslim edilecek, dönecek, geciken).
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    DashboardPage(Database &db, QWidget *parent);

public slots:
    void refresh();

private:
    Database &m_db;
    QList<QLabel *> m_values;
    QTableWidget *m_todo = nullptr;
    QLabel *m_empty = nullptr;
};