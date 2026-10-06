#include "ui/RevenueChart.h"

#include "app/I18n.h"
#include "app/Theme.h"

#include <QLocale>
#include <QPainter>

RevenueChart::RevenueChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(240);
}

void RevenueChart::setValues(const QList<qint64> &values)
{
    m_values = values;
    update(); // pencereyi yeniden çizdirir (paintEvent çağrılır)
}

void RevenueChart::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QLocale locale(I18n::language() == "en" ? QLocale::English : QLocale::Turkish);
    const QColor text = palette().color(QPalette::WindowText);
    const int labelHeight = fontMetrics().height() + 6;

    // Çizim alanı: altta ay adları, üstte tutar yazıları için boşluk
    const QRectF area = QRectF(rect()).adjusted(8, labelHeight, -8, -labelHeight);
    const qint64 maximum = m_values.isEmpty() ? 0 : *std::max_element(m_values.cbegin(), m_values.cend());
    const double slot = area.width() / 12.0;
    const double barWidth = slot * 0.6;

    painter.setPen(Theme::muted());
    painter.drawLine(area.bottomLeft(), area.bottomRight());
    for (int month = 0; month < 12; ++month) {
        const double x = area.left() + slot * month;
        const qint64 value = month < m_values.size() ? m_values[month] : 0;
        // En yüksek ay alanın tamamını kaplar, diğerleri ona oranla
        const double height = maximum > 0 ? area.height() * double(value) / double(maximum) : 0;
        const QRectF bar(x + (slot - barWidth) / 2, area.bottom() - height, barWidth, height);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::accent());
        painter.drawRoundedRect(bar, 3, 3);

        painter.setPen(text);
        painter.drawText(QRectF(x, area.bottom() + 3, slot, labelHeight), Qt::AlignHCenter | Qt::AlignTop,
                         locale.monthName(month + 1, QLocale::ShortFormat));
        if (value > 0) {
            // Tutar kısaltılır: 12.500 TL → 12,5K; 4.775.600 TL → 4,8M
            const double lira = value / 100.0;
            const QString label = lira >= 1'000'000 ? locale.toString(lira / 1'000'000, 'f', 1) + "M"
                                                    : locale.toString(lira / 1'000, 'f', 1) + "K";
            painter.drawText(QRectF(x, bar.top() - labelHeight, slot, labelHeight), Qt::AlignCenter, label);
        }
    }
}
