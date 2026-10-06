#pragma once

#include <QWidget>

class Database;
class QComboBox;
class QLabel;
class QTableWidget;
class RevenueChart;

// Raporlar: seçilen yılın aylık geliri (grafik) ve en çok kiralanan araçlar.
class ReportsPage : public QWidget
{
    Q_OBJECT

public:
    ReportsPage(Database &db, QWidget *parent);

public slots:
    void refresh();

private:
    void showYear();

    Database &m_db;
    QComboBox *m_year;
    QLabel *m_total, *m_empty;
    RevenueChart *m_chart;
    QTableWidget *m_top;
};