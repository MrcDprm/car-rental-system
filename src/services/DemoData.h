#pragma once

#include <QDate>

class Database;

// Uygulamayı denemek için örnek veri: ~120 araçlık hayali filo (gerçek modeller, her bütçeden),
// 25 müşteri ve son 12 ayın kiralama ve bakım geçmişi. Sadece boş veritabanına yüklenir.
namespace DemoData {

bool isEmpty(Database &db);
bool load(Database &db, const QDate &today = QDate::currentDate());

} // namespace DemoData