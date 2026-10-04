# Araç Kiralama Sistemi (Car Rental System)

[English](README.md) | **Türkçe**

Küçük bir araç kiralama ofisi için C++20 ve Qt 6 ile yazılmış masaüstü yönetim uygulaması: filo, müşteriler, çakışmasız rezervasyon, teslim ve iade, bakım, yazdırılabilir sözleşme ve gelir raporları. Veriler yerelde SQLite'ta tutulur.

> 🚧 Geliştiriliyor. Bu README şimdilik proje planı; v1.0.0'da tamamlanacak.

## Plan

### MVP
- **Özet ekranı:** kiradaki araçlar, bugün dönecekler, gecikenler, bakım bekleyen araçlar.
- **Filo:** plaka, marka, model, yıl, sınıf (ekonomi, kompakt, SUV, minivan…), vites, yakıt, koltuk sayısı, günlük fiyat, km, durum (müsait, kirada, bakımda). Arama ve filtreler.
- **Müşteriler:** ad soyad, telefon, e-posta, kimlik no, ehliyet no ve tarihi. Kontroller: en az 21 yaş ve en az 2 yıllık ehliyet.
- **Rezervasyon:** Tarih aralığı seçilince sadece o aralıkta boş olan araçlar görünür; çakışan rezervasyon yapılamaz. Fiyat = gün × günlük fiyat; haftalık ve aylık indirim, isteğe bağlı depozito.
- **Teslim ve iade:** çıkış ve dönüş km'si, yakıt seviyesi, gecikme ücreti, fazla km; aracın durumu ve km'si kendiliğinden güncellenir. Rezervasyon iptali.
- **Bakım:** Aracı bakıma alma ve çıkarma, maliyet kaydı; sonraki bakım km'si yaklaşınca uyarı.
- **Sözleşme ve fiş:** Teslimde yazdırılabilir / PDF kiralama sözleşmesi, iadede fiş.
- **Raporlar:** aylık gelir grafiği, en çok kiralanan araçlar, filo doluluk oranı.
- **CSV dışa aktarma:** filo, müşteriler ve kiralamalar; Excel'de açılmaya hazır.
- **Güvenlik:** Her girdi doğrulanır, SQL sorguları sadece parametreyle yapılır, yuvarlama hatası olmasın diye para kuruş (tam sayı) olarak saklanır.
- **Masaüstü uygulaması:** koyu ve açık tema, Türkçe ve İngilizce, ikon, sürüm, Hakkında penceresi, kullanıcı klasöründe veri, Windows kurulum dosyası (windeployqt + Inno Setup).
- **Testler:** Qt Test ile fiyatlandırma, tarih çakışması, yaş ve ehliyet kuralları, ücretler.

### Gelecek Planları
- Rollü personel hesapları.
- Çevrimiçi rezervasyon formu.
- Teslim ve iadede hasar fotoğrafları.
- Birden fazla şube.

## Kullanılan Teknolojiler
- C++20, Qt 6 (Widgets, SQL, PrintSupport, Test)
- SQLite
- CMake, MinGW-w64 (MSYS2 UCRT64)
- windeployqt, Inno Setup
