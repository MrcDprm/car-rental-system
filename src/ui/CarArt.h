#pragma once

#include "core/Models.h"

#include <QPixmap>

// Araç görseli: fotoğrafı varsa o, yoksa kasa tipine göre çizilmiş yandan görünüm (aracın renginde).
// Çizimler QPainter ile koddan üretilir; telifli bir görsel kullanılmaz.
namespace CarArt {

QColor paint(CarColor color);
QPixmap drawing(BodyType body, CarColor color, const QSize &size);
QPixmap image(const Vehicle &vehicle, const QSize &size); // önce fotoğraf, yoksa çizim

} // namespace CarArt