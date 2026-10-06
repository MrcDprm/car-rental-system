#pragma once

#include <QList>
#include <QWidget>

// 12 aylık gelir çubuk grafiği. Ek grafik kütüphanesi yerine QPainter ile çizilir.
class RevenueChart : public QWidget
{
    Q_OBJECT

public:
    explicit RevenueChart(QWidget *parent);
    void setValues(const QList<qint64> &values); // kuruş, 12 ay

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<qint64> m_values;
};