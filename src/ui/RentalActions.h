#pragma once

#include "core/Models.h"

class Database;
class QWidget;

// Kiralamalar sayfası ve özet ekranının ortak işlemleri: teslim, iade, sözleşme/fiş PDF'i.
// Her biri işlem yapıldıysa true döner (çağıran liste yenilenir).
namespace RentalActions {

bool pickUp(Database &db, const Rental &rental, QWidget *parent);
bool giveBack(Database &db, const Rental &rental, QWidget *parent);
void printContract(Database &db, const Rental &rental, QWidget *parent);
QString number(qint64 id); // 12 → 00012

} // namespace RentalActions
