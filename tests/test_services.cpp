#include "app/I18n.h"
#include "core/Rules.h"
#include "services/ContractPrinter.h"
#include "services/CsvExport.h"
#include "services/PhotoStore.h"

#include <QImage>
#include <QImageReader>

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class TestServices : public QObject
{
    Q_OBJECT

private slots:
    void init() { I18n::setLanguage("tr"); }

    void csvEscape()
    {
        QCOMPARE(CsvExport::escape("Renault Clio", ';'), QString("Renault Clio"));
        QCOMPARE(CsvExport::escape("1.250,50 ₺", ';'), QString("1.250,50 ₺"));  // ; ayırıcıda virgül sorun değil
        QCOMPARE(CsvExport::escape("1,250.50", ','), QString("\"1,250.50\""));   // , ayırıcıda tırnağa alınır
        QCOMPARE(CsvExport::escape("a;b", ';'), QString("\"a;b\""));
        QCOMPARE(CsvExport::escape("12\" jant", ';'), QString("\"12\"\" jant\""));
        QCOMPARE(CsvExport::escape("satır\nalt", ';'), QString("\"satır\nalt\""));
        QCOMPARE(CsvExport::escape("", ';'), QString(""));
    }

    void csvFormulaInjection()
    {
        // Formül gibi başlayan hücre metne çevrilir: Excel çalıştırmaz
        QCOMPARE(CsvExport::escape("=HYPERLINK(\"http://x\")", ','), QString("\"'=HYPERLINK(\"\"http://x\"\")\""));
        QCOMPARE(CsvExport::escape("+905321234567", ';'), QString("'+905321234567"));
        QCOMPARE(CsvExport::escape("-2+3", ';'), QString("'-2+3"));
        QCOMPARE(CsvExport::escape("@SUM(A1)", ';'), QString("'@SUM(A1)"));
        QCOMPARE(CsvExport::escape("a=b", ';'), QString("a=b")); // ortadaki = zararsız
    }

    void csvWrite()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("out.csv");
        QVERIFY(CsvExport::write(path, {"Ad", "Tutar"}, {{"Ayşe", "1.250,00 ₺"}, {"Can", "=1+1"}}, ';'));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray data = file.readAll();
        QVERIFY(data.startsWith("\xEF\xBB\xBF")); // BOM
        QCOMPARE(QString::fromUtf8(data.mid(3)), QString("Ad;Tutar\r\nAyşe;1.250,00 ₺\r\nCan;'=1+1\r\n"));
        QCOMPARE(CsvExport::separatorFor("tr"), QChar(';'));
        QCOMPARE(CsvExport::separatorFor("en"), QChar(','));
        QVERIFY(!CsvExport::write(dir.filePath("yok/out.csv"), {"a"}, {}, ';')); // olmayan klasör
    }

    void i18nFormatting()
    {
        QCOMPARE(I18n::phone("5321234567"), QString("0532 123 45 67"));
        QCOMPARE(I18n::phone("123"), QString("123")); // beklenmeyen uzunluk olduğu gibi kalır
        QCOMPARE(I18n::number(12650), QString("12.650"));
        I18n::setLanguage("en");
        QCOMPARE(I18n::number(12650), QString("12,650"));
        QCOMPARE(I18n::t("fleet"), QString("Fleet"));
        QCOMPARE(I18n::error("too_young"), QString("The customer must be at least 21."));
        QCOMPARE(I18n::features(Feature::Navigation | Feature::Sunroof), QStringList({"Navigation", "Sunroof"}));
        QCOMPARE(I18n::bodyType(BodyType::Station), QString("Estate"));
        QCOMPARE(I18n::error("no_such_key"), I18n::t("unexpected_error")); // bilinmeyen hata genel mesaja düşer
        I18n::setLanguage("de");
        QCOMPARE(I18n::language(), QString("tr")); // desteklenmeyen dil Türkçeye döner
    }

    void photosAreCheckedAndReEncoded()
    {
        QTemporaryDir dir;
        PhotoStore::setFolder(dir.filePath("photos"));
        QImage picture(2400, 1200, QImage::Format_RGB32);
        picture.fill(Qt::red);
        QVERIFY(picture.save(dir.filePath("car.png")));

        const QString name = PhotoStore::importPhoto(dir.filePath("car.png"));
        QVERIFY(Rules::isValidPhotoName(name));
        {
            QImageReader saved(PhotoStore::path(name)); // blok bitince dosya kapanır (Windows açık dosyayı silmez)
            QCOMPARE(saved.format(), QByteArray("jpeg")); // PNG değil, yeniden kodlanmış JPEG
            QCOMPARE(saved.size().width(), 1280);       // küçültüldü
        }

        // Uzantısı .jpg olan ama resim olmayan dosya reddedilir
        QFile fake(dir.filePath("virus.jpg"));
        QVERIFY(fake.open(QIODevice::WriteOnly));
        fake.write("MZ this is not an image");
        fake.close();
        QVERIFY(PhotoStore::importPhoto(dir.filePath("virus.jpg")).isEmpty());
        QVERIFY(PhotoStore::importPhoto(dir.filePath("missing.png")).isEmpty());

        // Geçersiz ad klasör dışını gösteremez
        QVERIFY(PhotoStore::path("../car.png").isEmpty());
        PhotoStore::remove("../car.png");
        QVERIFY(QFile::exists(dir.filePath("car.png")));
        PhotoStore::remove(name);
        QVERIFY(!QFile::exists(dir.filePath("photos/" + name)));
    }

    void contractEscapesHtml()
    {
        Rental rental;
        rental.id = 12;
        rental.startDate = QDate(2026, 10, 10);
        rental.endDate = QDate(2026, 10, 13);
        rental.dailyPrice = 150000;
        rental.total = 450000;
        rental.notes = "<script>alert(1)</script>";
        Vehicle vehicle;
        vehicle.plate = "34 ABC 123";
        vehicle.brand = "Renault";
        vehicle.model = "<b>Clio</b>";
        Customer customer;
        customer.fullName = "Ayşe & Can";
        customer.phone = "5321234567";
        const QString html = ContractPrinter::html(rental, vehicle, customer);
        QVERIFY(!html.contains("<script>"));
        QVERIFY(html.contains("&lt;script&gt;"));
        QVERIFY(html.contains("&lt;b&gt;Clio&lt;/b&gt;"));
        QVERIFY(html.contains("Ayşe &amp; Can"));
        QVERIFY(html.contains("00012"));
        QVERIFY(html.contains(I18n::t("contract_title")));
        rental.status = RentalStatus::Returned;
        QVERIFY(ContractPrinter::html(rental, vehicle, customer).contains(I18n::t("receipt_title")));
    }
};

QTEST_GUILESS_MAIN(TestServices)
#include "test_services.moc"
