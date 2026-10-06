#include "ui/CarArt.h"

#include "services/PhotoStore.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>

namespace {

// Çizimler 200 × 90'lık bir alanda tasarlanır, istenen boyuta ölçeklenir. Araç sağa bakar.
constexpr double WIDTH = 200;
constexpr double HEIGHT = 90;
constexpr double GROUND = 82;

struct Shape {
    QList<QPointF> body;  // dış hat: arka alttan başlayıp üstten öne, sonra ön alta
    QList<QPointF> glass; // camlar
    QList<double> pillars; // camı bölen dikmelerin x konumu
    double rearWheel, frontWheel, wheelRadius;
};

Shape shapeFor(BodyType body)
{
    switch (body) {
    case BodyType::Sedan:
        return {{{12, 72}, {10, 50}, {16, 45}, {48, 43}, {68, 25}, {118, 23}, {144, 43}, {180, 47}, {190, 55},
                 {190, 68}, {186, 72}},
                {{56, 42}, {71, 28}, {116, 27}, {136, 42}},
                {94},
                46, 158, 14};
    case BodyType::Station:
        return {{{12, 72}, {11, 46}, {18, 26}, {28, 23}, {118, 23}, {144, 43}, {180, 47}, {190, 55}, {190, 68},
                 {186, 72}},
                {{19, 42}, {24, 28}, {116, 27}, {136, 42}},
                {50, 92},
                46, 158, 14};
    case BodyType::Suv:
        return {{{14, 74}, {13, 42}, {20, 18}, {30, 14}, {118, 14}, {146, 36}, {182, 41}, {190, 50}, {190, 70},
                 {184, 74}},
                {{21, 36}, {26, 20}, {116, 19}, {138, 36}},
                {54, 92},
                50, 154, 17};
    case BodyType::Minivan:
        return {{{14, 74}, {13, 40}, {17, 14}, {26, 10}, {104, 10}, {150, 36}, {184, 42}, {190, 52}, {190, 70},
                 {184, 74}},
                {{19, 34}, {22, 16}, {102, 15}, {140, 35}},
                {50, 82},
                46, 160, 15};
    case BodyType::Coupe:
        return {{{14, 72}, {12, 52}, {20, 47}, {40, 45}, {76, 27}, {112, 26}, {148, 45}, {182, 50}, {190, 56},
                 {190, 68}, {186, 72}},
                {{52, 44}, {79, 31}, {110, 30}, {136, 44}},
                {},
                48, 156, 14};
    case BodyType::Pickup:
        return {{{12, 74}, {11, 44}, {80, 44}, {84, 16}, {120, 15}, {146, 36}, {182, 41}, {190, 50}, {190, 70},
                 {184, 74}},
                {{89, 36}, {91, 20}, {118, 19}, {138, 36}},
                {},
                46, 158, 17};
    case BodyType::Hatchback:
        break;
    }
    return {{{20, 72}, {18, 50}, {26, 44}, {34, 26}, {48, 22}, {110, 22}, {138, 42}, {176, 47}, {186, 55},
             {186, 68}, {182, 72}},
            {{36, 42}, {44, 27}, {108, 26}, {128, 42}},
            {82},
            52, 152, 14};
}

void drawCar(QPainter &p, BodyType body, const QColor &paint)
{
    const Shape s = shapeFor(body);
    p.setRenderHint(QPainter::Antialiasing);

    // Zemin gölgesi
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 45));
    p.drawEllipse(QRectF(10, GROUND - 5, 180, 8));

    // Gövde: aynı renkte kalın kalemle çizilince köşeler yuvarlanır
    QPolygonF outline(s.body);
    QLinearGradient shade(0, 10, 0, 74);
    shade.setColorAt(0, paint.lighter(112));
    shade.setColorAt(1, paint.darker(118));
    p.setBrush(shade);
    p.setPen(QPen(paint.darker(118), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolygon(outline);
    // Koyu renkli araç koyu zeminde kaybolmasın: kenar çizgisi koyu boyada açık, açık boyada koyu
    p.setPen(QPen(paint.lightness() < 80 ? paint.lighter(260) : paint.darker(150), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawPolygon(outline);

    // Camlar
    QLinearGradient glass(0, 14, 0, 44);
    glass.setColorAt(0, QColor("#6c7f93"));
    glass.setColorAt(1, QColor("#28323d"));
    p.setPen(Qt::NoPen);
    p.setBrush(glass);
    p.drawPolygon(QPolygonF(s.glass));
    // Dikmeler sadece camın içinde görünsün: çizim cam şekliyle sınırlandırılır (clip)
    QPainterPath glassPath;
    glassPath.addPolygon(QPolygonF(s.glass));
    p.save();
    p.setClipPath(glassPath);
    p.setPen(QPen(paint.darker(110), 3.5));
    for (double x : s.pillars)
        p.drawLine(QPointF(x, 0), QPointF(x - 3, HEIGHT));
    p.restore();

    // Kapı çizgisi ve kapı kolu
    p.setPen(QPen(paint.darker(140), 1));
    const double doorTop = s.glass.first().y() + 3;
    for (double x : s.pillars)
        p.drawLine(QPointF(x, doorTop), QPointF(x - 2, 66));
    p.drawLine(QPointF(s.glass.last().x() + 4, doorTop), QPointF(s.glass.last().x() + 2, 66));

    // Farlar
    const QPointF front = s.body[s.body.size() - 3];
    const QPointF rear = s.body[1];
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#fff3c4"));
    p.drawRoundedRect(QRectF(front.x() - 10, front.y() + 1, 9, 5), 2, 2);
    p.setBrush(QColor("#d43a3a"));
    p.drawRoundedRect(QRectF(rear.x() - 1, rear.y() + 1, 5, 7), 1.5, 1.5);

    // Tekerlekler
    for (double x : {s.rearWheel, s.frontWheel}) {
        const QPointF center(x, GROUND - s.wheelRadius);
        p.setBrush(paint.darker(190)); // çamurluk boşluğu
        p.drawEllipse(center, s.wheelRadius + 3, s.wheelRadius + 3);
        p.setBrush(QColor("#1d1f22"));
        p.drawEllipse(center, s.wheelRadius, s.wheelRadius);
        p.setBrush(QColor("#a9aeb5"));
        p.drawEllipse(center, s.wheelRadius * 0.55, s.wheelRadius * 0.55);
        p.setBrush(QColor("#5b6067"));
        p.drawEllipse(center, s.wheelRadius * 0.18, s.wheelRadius * 0.18);
    }
}

} // namespace

namespace CarArt {

QColor paint(CarColor color)
{
    static const char *const colors[] = {"#eef0f2", "#2b2d31", "#767c85", "#bfc4ca", "#c8323c", "#2f6fc4",
                                         "#3f8f5a", "#d6c4a0", "#e8833a", "#e6c13a", "#7a5136"};
    return QColor(colors[static_cast<int>(color)]);
}

QPixmap drawing(BodyType body, CarColor color, const QSize &size)
{
    // Aynı çizim yüzlerce kez istenir (her tablo satırı, her kart); önbellekten verilir
    const QString key = QString("car-%1-%2-%3x%4").arg(int(body)).arg(int(color)).arg(size.width()).arg(size.height());
    QPixmap pixmap;
    if (QPixmapCache::find(key, &pixmap))
        return pixmap;

    const qreal ratio = 2; // yüksek DPI ekranlarda keskin görünsün
    pixmap = QPixmap(size * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    const double scale = std::min(size.width() / WIDTH, size.height() / HEIGHT);
    painter.translate((size.width() - WIDTH * scale) / 2, (size.height() - HEIGHT * scale) / 2);
    painter.scale(scale, scale);
    drawCar(painter, body, paint(color));
    painter.end();
    QPixmapCache::insert(key, pixmap);
    return pixmap;
}

QPixmap image(const Vehicle &vehicle, const QSize &size)
{
    const QString file = PhotoStore::path(vehicle.photo);
    if (!file.isEmpty()) {
        const QString key = "photo-" + vehicle.photo + QString("-%1x%2").arg(size.width()).arg(size.height());
        QPixmap cached;
        if (QPixmapCache::find(key, &cached))
            return cached;
        const QPixmap photo(file);
        if (!photo.isNull()) {
            // Alanı dolduracak şekilde ölçeklenir ve ortadan kırpılır
            const QPixmap scaled = photo.scaled(size * 2, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            QPixmap result = scaled.copy((scaled.width() - size.width() * 2) / 2,
                                         (scaled.height() - size.height() * 2) / 2, size.width() * 2,
                                         size.height() * 2);
            result.setDevicePixelRatio(2);
            QPixmapCache::insert(key, result);
            return result;
        }
    }
    return drawing(vehicle.bodyType, vehicle.color, size);
}

} // namespace CarArt