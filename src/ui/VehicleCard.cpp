#include "ui/VehicleCard.h"

#include "app/I18n.h"
#include "app/Theme.h"
#include "core/Pricing.h"
#include "ui/CarArt.h"

#include <QPainter>
#include <QPainterPath>

namespace {

const QSize CARD(272, 268);
constexpr int PADDING = 12;
constexpr int IMAGE_HEIGHT = 112;

} // namespace

VehicleCardDelegate::VehicleCardDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void VehicleCardDelegate::setVehicles(const QList<Vehicle> &vehicles)
{
    m_vehicles.clear();
    for (const Vehicle &v : vehicles)
        m_vehicles.insert(v.id, v);
}

void VehicleCardDelegate::setDates(const QDate &start, const QDate &end)
{
    m_start = start;
    m_end = end;
}

QSize VehicleCardDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return CARD;
}

void VehicleCardDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const auto it = m_vehicles.constFind(index.data(ID_ROLE).toLongLong());
    if (it == m_vehicles.constEnd())
        return;
    const Vehicle &v = *it;
    const bool selected = option.state & QStyle::State_Selected;
    const QRect card = option.rect.adjusted(6, 6, -6, -6);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Kart zemini; seçili kart vurgu renginde çerçevelenir
    QColor border = Theme::muted();
    border.setAlpha(90);
    painter->setPen(QPen(selected ? Theme::accent() : border, selected ? 2 : 1));
    painter->setBrush(option.palette.color(QPalette::Button));
    painter->drawRoundedRect(QRectF(card).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);

    // Görsel: fotoğraf kartın üst köşelerine göre kırpılır
    const QRect imageRect(card.left() + 1, card.top() + 1, card.width() - 2, IMAGE_HEIGHT);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(imageRect), 7, 7);
    painter->setClipPath(clip);
    painter->drawPixmap(imageRect, CarArt::image(v, imageRect.size()));
    painter->setClipping(false);

    const QRect text = card.adjusted(PADDING, IMAGE_HEIGHT + 8, -PADDING, -PADDING);
    const QColor normal = option.palette.color(QPalette::Text);
    QFont font = option.font;
    int y = text.top();
    auto line = [&](const QString &value, const QColor &color, int pointSize, bool bold) {
        QFont f = font;
        f.setPointSize(pointSize);
        f.setBold(bold);
        painter->setFont(f);
        painter->setPen(color);
        const QFontMetrics metrics(f);
        // Sığmayan metin "…" ile kısaltılır
        painter->drawText(QRect(text.left(), y, text.width(), metrics.height()), Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(value, Qt::ElideRight, text.width()));
        y += metrics.height() + 3;
    };

    line(v.brand + " " + v.model, normal, 11, true);
    line(QString("%1 · %2 · %3 · %4").arg(v.plate, QString::number(v.year), I18n::vehicleClass(v.vehicleClass),
                                         I18n::color(v.color)),
         Theme::muted(), 9, false);
    line(QString("%1 · %2 · %3 · %4")
             .arg(I18n::t("seat_count").replace("{0}", QString::number(v.seats)),
                  I18n::t("luggage_count").replace("{0}", QString::number(v.luggage)),
                  I18n::transmission(v.transmission), I18n::fuel(v.fuel)),
         normal, 9, false);
    const QStringList features = I18n::features(v.features);
    line(features.isEmpty() ? QString("—") : features.join(", "), Theme::muted(), 9, false);

    // Alt satır: solda günlük fiyat, sağda seçilen tarihler için indirimli toplam
    const Pricing::Quote quote = Pricing::quote(v.dailyPrice, m_start, m_end);
    QFont priceFont = font;
    priceFont.setPointSize(9);
    painter->setFont(priceFont);
    painter->setPen(Theme::muted());
    const QRect bottom(text.left(), text.bottom() - 24, text.width(), 24);
    painter->drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter, I18n::money(v.dailyPrice) + " " + I18n::t("per_day"));
    priceFont.setPointSize(12);
    priceFont.setBold(true);
    painter->setFont(priceFont);
    painter->setPen(Theme::accent());
    painter->drawText(bottom, Qt::AlignRight | Qt::AlignVCenter, I18n::money(quote.total));
    if (quote.discountPercent > 0) {
        priceFont.setPointSize(8);
        priceFont.setBold(false);
        painter->setFont(priceFont);
        painter->setPen(Theme::success());
        painter->drawText(QRect(bottom.left(), bottom.top() - 16, bottom.width(), 16), Qt::AlignRight | Qt::AlignVCenter,
                          I18n::t("discount").replace("{0}", QString::number(quote.discountPercent)));
    }
    painter->restore();
}