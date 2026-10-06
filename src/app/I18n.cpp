#include "app/I18n.h"

#include "core/Money.h"

#include <QHash>
#include <QLocale>

namespace {

QString g_language = "tr";

struct Text {
    const char *tr;
    const char *en;
};

const QHash<QString, Text> &texts()
{
    static const QHash<QString, Text> table = {
        // Genel
        {"app_name", {"Araç Kiralama", "Car Rental"}},
        {"dashboard", {"Özet", "Dashboard"}},
        {"fleet", {"Filo", "Fleet"}},
        {"customers", {"Müşteriler", "Customers"}},
        {"rentals", {"Kiralamalar", "Rentals"}},
        {"reports", {"Raporlar", "Reports"}},
        {"theme", {"Tema", "Theme"}},
        {"language", {"English", "Türkçe"}},
        {"about", {"Hakkında", "About"}},
        {"add", {"Ekle", "Add"}},
        {"edit", {"Düzenle", "Edit"}},
        {"delete", {"Sil", "Delete"}},
        {"save", {"Kaydet", "Save"}},
        {"cancel", {"Vazgeç", "Cancel"}},
        {"close", {"Kapat", "Close"}},
        {"yes", {"Evet", "Yes"}},
        {"no", {"Hayır", "No"}},
        {"ok", {"Tamam", "OK"}},
        {"search", {"Ara…", "Search…"}},
        {"export_csv", {"CSV'ye aktar", "Export CSV"}},
        {"all", {"Tümü", "All"}},
        {"confirm_delete", {"Seçili kayıt silinsin mi?", "Delete the selected record?"}},
        {"saved_csv", {"{0} satır kaydedildi.", "{0} rows saved."}},
        {"csv_failed", {"Dosya kaydedilemedi.", "The file could not be saved."}},
        {"select_row", {"Önce listeden bir satır seç.", "Select a row in the list first."}},
        {"unexpected_error", {"Beklenmeyen bir hata oluştu.", "An unexpected error occurred."}},
        {"db_open_failed", {"Veri dosyası açılamadı. Klasöre yazma izni olduğundan emin ol:\n{0}",
                            "The data file could not be opened. Make sure this folder is writable:\n{0}"}},
        {"version", {"Sürüm {0}", "Version {0}"}},
        {"about_text",
         {"Küçük bir araç kiralama ofisi için C++20 ve Qt 6 ile yazılmış yönetim uygulaması. Veriler bu bilgisayarda "
          "SQLite veritabanında tutulur.",
          "A management app for a small car rental office, written in C++20 and Qt 6. Data is stored on this "
          "computer in an SQLite database."}},
        {"data_folder", {"Veri klasörü", "Data folder"}},
        {"view_on_github", {"GitHub'da görüntüle", "View on GitHub"}},

        // Özet
        {"vehicles_total", {"Toplam araç", "Vehicles"}},
        {"available_now", {"Müsait", "Available"}},
        {"rented_now", {"Kirada", "Rented"}},
        {"in_maintenance", {"Bakımda", "In maintenance"}},
        {"pickups_today", {"Teslim bekleyen", "Pick-ups due"}},
        {"returns_today", {"Bugün dönecek", "Returns today"}},
        {"overdue", {"Geciken", "Overdue"}},
        {"service_due", {"Bakımı yaklaşan", "Service due"}},
        {"todo", {"Yapılacaklar", "To do"}},
        {"nothing_todo", {"Bugün için bekleyen iş yok.", "Nothing waiting for today."}},
        {"todo_pickup", {"Teslim edilecek", "Pick up"}},
        {"todo_return", {"Dönecek", "Return"}},
        {"todo_overdue", {"Gecikti", "Overdue"}},
        {"nothing_here", {"Bu listede kayıt yok.", "Nothing in this list."}},
        {"rent_this", {"Kirala", "Rent"}},
        {"utilization", {"Doluluk", "Utilisation"}},

        // Filo
        {"plate", {"Plaka", "Plate"}},
        {"brand", {"Marka", "Brand"}},
        {"model", {"Model", "Model"}},
        {"vehicle", {"Araç", "Vehicle"}},
        {"year", {"Yıl", "Year"}},
        {"class", {"Sınıf", "Class"}},
        {"transmission", {"Vites", "Transmission"}},
        {"fuel", {"Yakıt", "Fuel"}},
        {"seats", {"Koltuk", "Seats"}},
        {"daily_price", {"Günlük fiyat", "Daily price"}},
        {"mileage", {"Km", "Mileage"}},
        {"next_service", {"Sonraki bakım (km)", "Next service (km)"}},
        {"status", {"Durum", "Status"}},
        {"new_vehicle", {"Yeni araç", "New vehicle"}},
        {"edit_vehicle", {"Aracı düzenle", "Edit vehicle"}},
        {"start_maintenance", {"Bakıma al", "Start maintenance"}},
        {"finish_maintenance", {"Bakımdan çıkar", "Finish maintenance"}},
        {"maintenance_description", {"Yapılacak iş", "Work to do"}},
        {"maintenance_cost", {"Maliyet", "Cost"}},
        {"maintenance_history", {"Bakım geçmişi", "Maintenance history"}},
        {"service_warning", {"Bakım yaklaşıyor", "Service due soon"}},
        {"body_type", {"Kasa tipi", "Body type"}},
        {"color", {"Renk", "Colour"}},
        {"luggage", {"Bagaj", "Luggage"}},
        {"luggage_count", {"{0} valiz", "{0} bags"}},
        {"seat_count", {"{0} koltuk", "{0} seats"}},
        {"features", {"Donanım", "Equipment"}},
        {"photo", {"Fotoğraf", "Photo"}},
        {"choose_photo", {"Fotoğraf seç…", "Choose photo…"}},
        {"remove_photo", {"Fotoğrafı kaldır", "Remove photo"}},
        {"photo_hint", {"Fotoğraf yoksa kasa tipine ve renge göre çizim gösterilir.",
                        "Without a photo, a drawing based on body type and colour is shown."}},
        {"images", {"Görseller", "Images"}},
        {"demo_title", {"Örnek filo", "Sample fleet"}},
        {"demo_question", {"Veritabanı boş. Uygulamayı denemek için örnek filo yüklensin mi?\n\n"
                           "~120 araç, 25 müşteri ve son 12 ayın kiralama geçmişi eklenir. Boş başlamak için Hayır de.",
                           "The database is empty. Load a sample fleet to try the app?\n\n"
                           "About 120 cars, 25 customers and 12 months of rental history are added. "
                           "Choose No to start empty."}},
        {"demo_failed", {"Örnek veri yüklenemedi.", "The sample data could not be loaded."}},

        // Müşteriler
        {"full_name", {"Ad soyad", "Full name"}},
        {"phone", {"Telefon", "Phone"}},
        {"email", {"E-posta (isteğe bağlı)", "E-mail (optional)"}},
        {"national_id", {"T.C. kimlik no", "National ID"}},
        {"license_number", {"Ehliyet no", "Licence number"}},
        {"license_hint", {"Ehliyetin 5. alanındaki numara", "Number in field 5 of the licence"}},
        {"birth_date", {"Doğum tarihi", "Date of birth"}},
        {"license_date", {"Ehliyet tarihi", "Licence date"}},
        {"new_customer", {"Yeni müşteri", "New customer"}},
        {"edit_customer", {"Müşteriyi düzenle", "Edit customer"}},
        {"customer", {"Müşteri", "Customer"}},

        // Kiralamalar
        {"new_rental", {"Yeni kiralama", "New rental"}},
        {"pick_up", {"Teslim et", "Pick up"}},
        {"give_back", {"İade al", "Return"}},
        {"cancel_rental", {"Rezervasyonu iptal et", "Cancel reservation"}},
        {"print_contract", {"Sözleşme / fiş", "Contract / receipt"}},
        {"confirm_cancel", {"Rezervasyon iptal edilsin mi?", "Cancel this reservation?"}},
        {"number", {"No", "No."}},
        {"start_date", {"Teslim", "Pick-up"}},
        {"end_date", {"Dönüş", "Return"}},
        {"total", {"Tutar", "Total"}},
        {"days", {"{0} gün", "{0} days"}},
        {"deposit", {"Depozito", "Deposit"}},
        {"notes", {"Not", "Notes"}},
        {"choose_vehicle", {"Bu tarihlerde boş araçlar", "Cars free on these dates"}},
        {"no_vehicle_free", {"Bu tarihlerde boş araç yok.", "No car is free on these dates."}},
        {"no_customers", {"Kayıtlı müşteri yok. \"+ Yeni müşteri\" ile ekle.", "No customers yet. Add one with \"+ New customer\"."}},
        {"price_summary", {"{0} × {1} = {2}", "{0} × {1} = {2}"}},
        {"discount", {"İndirim %{0}", "Discount {0}%"}},
        {"pick_up_now", {"Hemen teslim et", "Pick up now"}},
        {"per_day", {"/ gün", "/ day"}},
        {"max_budget", {"Bütçe (günlük en fazla)", "Budget (max per day)"}},
        {"no_limit", {"Sınırsız", "No limit"}},
        {"min_seats", {"En az koltuk", "Min. seats"}},
        {"min_luggage", {"En az bagaj", "Min. luggage"}},
        {"sort", {"Sıralama", "Sort"}},
        {"sort_cheap", {"Önce en ucuz", "Cheapest first"}},
        {"sort_expensive", {"Önce en pahalı", "Most expensive first"}},
        {"matching", {"{0} araç uygun", "{0} cars match"}},
        {"choose_card", {"Listeden bir araç seç.", "Choose a car from the list."}},
        {"km_out", {"Çıkış km", "Mileage out"}},
        {"km_in", {"Dönüş km", "Mileage in"}},
        {"fuel_level", {"Yakıt seviyesi", "Fuel level"}},
        {"return_date", {"İade tarihi", "Return date"}},
        {"late_fee", {"Gecikme ({0} gün)", "Late return ({0} days)"}},
        {"extra_km_fee", {"Fazla km ({0} km)", "Extra mileage ({0} km)"}},
        {"fuel_fee", {"Eksik yakıt ({0}/8)", "Missing fuel ({0}/8)"}},
        {"extra_total", {"Ek ücret toplamı", "Extra charges"}},
        {"grand_total", {"Genel toplam", "Grand total"}},
        {"rental_fee", {"Kira bedeli", "Rental fee"}},
        {"save_pdf", {"PDF olarak kaydet", "Save as PDF"}},
        {"contract_title", {"ARAÇ KİRALAMA SÖZLEŞMESİ", "CAR RENTAL CONTRACT"}},
        {"receipt_title", {"İADE FİŞİ", "RETURN RECEIPT"}},
        {"renter", {"Kiracı", "Renter"}},
        {"signature_renter", {"Kiracı imzası", "Renter's signature"}},
        {"signature_office", {"Yetkili imzası", "Office signature"}},
        {"email_short", {"E-posta", "E-mail"}},
        {"allowances", {"Haklar ve teminat", "Allowances and deposit"}},
        {"return_info", {"İade bilgileri", "Return details"}},
        {"rental_details", {"Kiralama bilgileri", "Rental details"}},
        {"charges", {"Ücretler", "Charges"}},
        {"description", {"Açıklama", "Description"}},
        {"amount", {"Tutar", "Amount"}},
        {"date", {"Tarih", "Date"}},
        {"rental_period", {"Kiralama süresi", "Rental period"}},
        {"km_driven", {"Kullanılan km", "Distance driven"}},
        {"free_km", {"Ücretsiz km hakkı", "Free mileage"}},
        {"terms_title", {"Kiralama koşulları", "Terms and conditions"}},
        {"damage_notes", {"Teslimde araç durumu ve hasar notları", "Vehicle condition and damage notes at pick-up"}},
        {"return_notes", {"İadede araç durumu ve notlar", "Vehicle condition and notes at return"}},
        {"generated_by", {"Bu belge {0} uygulamasıyla oluşturulmuştur.", "Generated by {0}."}},
        {"name_signature", {"Ad soyad / imza", "Name / signature"}},
        {"term_1", {"Araç yalnızca bu sözleşmede adı yazan kiracı tarafından kullanılabilir; başkasına devredilemez veya "
                    "kiralanamaz.",
                    "Only the renter named in this contract may drive the car; it may not be lent or sublet."}},
        {"term_2", {"Kiralama süresince oluşan trafik cezaları, köprü ve otoyol (HGS/OGS) geçiş ücretleri kiracıya aittir.",
                    "Traffic fines and bridge or motorway tolls incurred during the rental are paid by the renter."}},
        {"term_3", {"Araç yurt dışına çıkarılamaz; arazi yolunda, yarışta ve ticari yük taşımacılığında kullanılamaz.",
                    "The car may not leave the country or be used off-road, for racing or for commercial haulage."}},
        {"term_4", {"Kaza ya da hasar durumunda kiracı ofise hemen haber verir ve kaza tespit tutanağı veya polis/jandarma "
                    "raporu alır.",
                    "In case of an accident or damage the renter informs the office at once and obtains an accident "
                    "report or a police report."}},
        {"term_5", {"Araç teslim alındığı yakıt seviyesiyle iade edilir; eksik her 1/8 depo için {0} alınır.",
                    "The car is returned with the same fuel level; each missing 1/8 tank costs {0}."}},
        {"term_6", {"Günlük {0} km ücretsizdir; aşan her km için {1} alınır.",
                    "{0} km per day are free; each extra kilometre costs {1}."}},
        {"term_7", {"Geç iade edilen her gün için günlük fiyatın %{0}'i alınır.",
                    "Each late day is charged at {0}% of the daily price."}},
        {"term_8", {"Araç içinde sigara içilmez; evcil hayvanlar yalnızca taşıma çantasında taşınabilir.",
                    "Smoking is not allowed in the car; pets may travel only in a carrier."}},
        {"term_9", {"Depozito, araç hasarsız ve eksiksiz iade edildiğinde geri ödenir.",
                    "The deposit is refunded when the car is returned undamaged and complete."}},
        {"pdf_saved", {"PDF kaydedildi.", "PDF saved."}},

        // Raporlar
        {"monthly_revenue", {"Aylık gelir", "Monthly revenue"}},
        {"top_vehicles", {"En çok kiralanan araçlar", "Most rented cars"}},
        {"rental_count", {"Kiralama", "Rentals"}},
        {"revenue", {"Gelir", "Revenue"}},
        {"year_total", {"Yıl toplamı: {0}", "Year total: {0}"}},
        {"no_data", {"Henüz tamamlanmış kiralama yok.", "No completed rentals yet."}},

        // Hatalar (Result ve Rules anahtarları)
        {"err_invalid_plate", {"Plaka geçersiz (ör. 34 ABC 123).", "Invalid plate (e.g. 34 ABC 123)."}},
        {"err_missing_brand_model", {"Marka ve model gerekli.", "Brand and model are required."}},
        {"err_invalid_year", {"Model yılı geçersiz.", "Invalid model year."}},
        {"err_invalid_seats", {"Koltuk sayısı 2 ile 9 arasında olmalı.", "Seats must be between 2 and 9."}},
        {"err_invalid_price", {"Günlük fiyat geçersiz.", "Invalid daily price."}},
        {"err_invalid_mileage", {"Km geçersiz.", "Invalid mileage."}},
        {"err_invalid_service_km", {"Sonraki bakım km'si mevcut km'den büyük olmalı.",
                                    "Next service mileage must be above the current mileage."}},
        {"err_invalid_luggage", {"Bagaj 0 ile 9 valiz arasında olmalı.", "Luggage must be between 0 and 9 bags."}},
        {"err_invalid_features", {"Donanım bilgisi geçersiz.", "Invalid equipment."}},
        {"err_invalid_photo", {"Fotoğraf açılamadı. JPG ya da PNG biçiminde, en fazla 10 MB bir resim seç.",
                               "The photo could not be opened. Choose a JPG or PNG image up to 10 MB."}},
        {"err_duplicate_plate", {"Bu plaka zaten kayıtlı.", "This plate is already registered."}},
        {"err_invalid_name", {"Ad soyad gerekli.", "Full name is required."}},
        {"err_invalid_phone", {"Telefon numarası geçersiz.", "Invalid phone number."}},
        {"err_invalid_email", {"E-posta adresi geçersiz.", "Invalid e-mail address."}},
        {"err_invalid_national_id", {"T.C. kimlik numarası geçersiz.", "Invalid national ID number."}},
        {"err_invalid_license_number", {"Ehliyet numarası gerekli.", "Licence number is required."}},
        {"err_invalid_birth_date", {"Doğum tarihi geçersiz.", "Invalid date of birth."}},
        {"err_too_young", {"Müşteri en az 21 yaşında olmalı.", "The customer must be at least 21."}},
        {"err_invalid_license_date", {"Ehliyet tarihi geçersiz.", "Invalid licence date."}},
        {"err_license_too_new", {"Ehliyet en az 2 yıllık olmalı.", "The licence must be at least 2 years old."}},
        {"err_duplicate_national_id", {"Bu T.C. kimlik numarası zaten kayıtlı.", "This national ID is already registered."}},
        {"err_invalid_dates", {"Dönüş tarihi teslimden sonra olmalı.", "The return date must be after pick-up."}},
        {"err_start_in_past", {"Teslim tarihi geçmişte olamaz.", "The pick-up date cannot be in the past."}},
        {"err_too_long", {"Kiralama en fazla 365 gün olabilir.", "A rental can last at most 365 days."}},
        {"err_invalid_deposit", {"Depozito geçersiz.", "Invalid deposit."}},
        {"err_notes_too_long", {"Not çok uzun.", "The note is too long."}},
        {"err_vehicle_in_maintenance", {"Araç bakımda.", "The car is in maintenance."}},
        {"err_vehicle_not_available", {"Araç bu tarihlerde müsait değil.", "The car is not available on these dates."}},
        {"err_invalid_state", {"Bu işlem kaydın şu anki durumunda yapılamaz.", "This cannot be done in the record's current state."}},
        {"err_not_started_yet", {"Teslim günü henüz gelmedi.", "The pick-up date has not come yet."}},
        {"err_km_below_odometer", {"Km, aracın göstergesindeki değerden düşük olamaz.", "Mileage cannot be below the odometer."}},
        {"err_invalid_km", {"Dönüş km'si geçersiz.", "Invalid return mileage."}},
        {"err_invalid_fuel", {"Yakıt seviyesi geçersiz.", "Invalid fuel level."}},
        {"err_invalid_description", {"Yapılacak işi yaz (en fazla 300 karakter).", "Describe the work (300 characters max)."}},
        {"err_invalid_cost", {"Maliyet geçersiz.", "Invalid cost."}},
        {"err_has_history", {"Geçmişi olan kayıt silinemez.", "A record with history cannot be deleted."}},
        {"err_not_found", {"Kayıt bulunamadı.", "Record not found."}},
        {"err_save_failed", {"Kaydedilemedi.", "Could not be saved."}},
    };
    return table;
}

QString lookup(const QString &key)
{
    const auto it = texts().constFind(key);
    if (it == texts().constEnd())
        return key; // eksik metin ekranda anahtarıyla görünür; çökme olmaz
    return QString::fromUtf8(g_language == "en" ? it->en : it->tr);
}

} // namespace

namespace I18n {

void setLanguage(const QString &language)
{
    g_language = language == "en" ? "en" : "tr";
    // Takvim, sayı kutuları ve tarih seçicileri de seçili dilin biçimini kullansın (1.250,00 / 1,250.00)
    QLocale::setDefault(QLocale(g_language == "en" ? QLocale::English : QLocale::Turkish,
                                g_language == "en" ? QLocale::UnitedStates : QLocale::Turkey));
}

QString language()
{
    return g_language;
}

QString t(const char *key)
{
    return lookup(QString::fromLatin1(key));
}

QString error(const QString &key)
{
    const QString text = lookup("err_" + key);
    return text.startsWith("err_") ? lookup("unexpected_error") : text;
}

QString vehicleClass(VehicleClass value)
{
    static const Text names[] = {{"Ekonomi", "Economy"}, {"Kompakt", "Compact"}, {"Orta", "Midsize"},
                                 {"SUV", "SUV"},         {"Minivan", "Van"},     {"Lüks", "Luxury"}};
    const Text &name = names[static_cast<int>(value)];
    return QString::fromUtf8(g_language == "en" ? name.en : name.tr);
}

QString transmission(Transmission value)
{
    if (value == Transmission::Automatic)
        return g_language == "en" ? "Automatic" : "Otomatik";
    return g_language == "en" ? "Manual" : "Manuel";
}

QString fuel(Fuel value)
{
    static const Text names[] = {{"Benzin", "Petrol"}, {"Dizel", "Diesel"}, {"Hibrit", "Hybrid"},
                                 {"Elektrik", "Electric"}, {"LPG", "LPG"}};
    const Text &name = names[static_cast<int>(value)];
    return QString::fromUtf8(g_language == "en" ? name.en : name.tr);
}

QString vehicleStatus(VehicleStatus value)
{
    static const Text names[] = {{"Müsait", "Available"}, {"Kirada", "Rented"}, {"Bakımda", "Maintenance"}};
    const Text &name = names[static_cast<int>(value)];
    return QString::fromUtf8(g_language == "en" ? name.en : name.tr);
}

QString bodyType(BodyType value)
{
    static const Text names[] = {{"Hatchback", "Hatchback"}, {"Sedan", "Saloon"}, {"Station wagon", "Estate"},
                                 {"SUV", "SUV"},             {"Minivan", "MPV"},  {"Coupe", "Coupé"},
                                 {"Pikap", "Pickup"}};
    const Text &name = names[static_cast<int>(value)];
    return QString::fromUtf8(g_language == "en" ? name.en : name.tr);
}

QString color(CarColor value)
{
    static const Text names[] = {{"Beyaz", "White"}, {"Siyah", "Black"},   {"Gri", "Grey"},     {"Gümüş", "Silver"},
                                 {"Kırmızı", "Red"}, {"Mavi", "Blue"},     {"Yeşil", "Green"},  {"Bej", "Beige"},
                                 {"Turuncu", "Orange"}, {"Sarı", "Yellow"}, {"Kahverengi", "Brown"}};
    const Text &name = names[static_cast<int>(value)];
    return QString::fromUtf8(g_language == "en" ? name.en : name.tr);
}

QString feature(int bit)
{
    static const Text names[Feature::COUNT] = {
        {"Navigasyon", "Navigation"},     {"Geri görüş kamerası", "Rear camera"}, {"CarPlay / Android Auto", "CarPlay / Android Auto"},
        {"Isofix (bebek koltuğu)", "Isofix (child seat)"}, {"4x4", "4x4"},         {"Sunroof", "Sunroof"},
        {"Hız sabitleyici", "Cruise control"}, {"Isıtmalı koltuk", "Heated seats"}};
    if (bit < 0 || bit >= Feature::COUNT)
        return QString();
    const Text &name = names[bit];
    return QString::fromUtf8(g_language == "en" ? name.en : name.tr);
}

QStringList features(int flags)
{
    QStringList names;
    for (int bit = 0; bit < Feature::COUNT; ++bit)
        if (flags & (1 << bit))
            names << feature(bit);
    return names;
}

QString rentalStatus(RentalStatus value)
{
    static const Text names[] = {{"Rezervasyon", "Reserved"}, {"Aktif", "Active"}, {"İade edildi", "Returned"},
                                 {"İptal", "Cancelled"}};
    const Text &name = names[static_cast<int>(value)];
    return QString::fromUtf8(g_language == "en" ? name.en : name.tr);
}

QString date(const QDate &value)
{
    if (!value.isValid())
        return "—";
    return value.toString(g_language == "en" ? "MMM d, yyyy" : "dd.MM.yyyy");
}

QString money(qint64 kurus)
{
    return Money::format(kurus, g_language);
}

QString number(int value)
{
    return QLocale(g_language == "en" ? QLocale::English : QLocale::Turkish).toString(value);
}

QString phone(const QString &digits)
{
    // Kayıtta numara sadece rakam olarak tutulur; ekranda okunması kolay biçimde gösterilir
    if (digits.size() != 10)
        return digits;
    return QString("0%1 %2 %3 %4").arg(digits.left(3), digits.mid(3, 3), digits.mid(6, 2), digits.mid(8, 2));
}

} // namespace I18n
