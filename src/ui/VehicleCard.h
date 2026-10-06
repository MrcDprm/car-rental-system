#pragma once

#include "core/Models.h"

#include <QHash>
#include <QStyledItemDelegate>

// Kiralama penceresindeki araç kartı: görsel, ad, özellikler, günlük ve toplam fiyat.
// Liste her kart için ayrı bir widget oluşturmaz; bu sınıf kartı satır satır çizer (120 araçta da hızlı).
class VehicleCardDelegate : public QStyledItemDelegate
{
public:
    static constexpr int ID_ROLE = Qt::UserRole; // listedeki her öğe aracın kimliğini taşır

    explicit VehicleCardDelegate(QObject *parent);
    void setVehicles(const QList<Vehicle> &vehicles);
    void setDates(const QDate &start, const QDate &end);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QHash<qint64, Vehicle> m_vehicles;
    QDate m_start, m_end;
};