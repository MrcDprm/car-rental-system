#include "services/ContractPrinter.h"

#include "app/I18n.h"
#include "core/Pricing.h"

#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>

namespace {

constexpr int TERM_COUNT = 9;
const QString ACCENT = "#005fb8";
const QString LIGHT = "#eef4fb";
const QString MUTED = "#5f6b7a";

// Kayıtlardaki metinler HTML'e kaçışlanarak girer: müşteri adında "<b>" olsa bile biçim olarak çalışmaz
QString escaped(const QString &text)
{
    return text.toHtmlEscaped();
}

QString t(const char *key)
{
    return escaped(I18n::t(key));
}

// Etiket / değer satırı (bilgi kutularında); değer kaçışlanır
QString row(const QString &label, const QString &value)
{
    return QString("<tr><td width='42%' style='color:%1'>%2</td><td><b>%3</b></td></tr>")
        .arg(MUTED, escaped(label), escaped(value));
}

// Değeri zaten HTML olan satır (yakıt çubuğu gibi; içinde kullanıcı verisi yoktur)
QString htmlRow(const QString &label, const QString &html)
{
    return QString("<tr><td width='42%' style='color:%1'>%2</td><td><b>%3</b></td></tr>")
        .arg(MUTED, escaped(label), html);
}

// Renkli başlıklı bilgi kutusu
QString box(const QString &title, const QString &rows)
{
    return QString("<table width='100%' cellspacing='0' cellpadding='3' style='border:1px solid #c9d6e6'>"
                   "<tr><td colspan='2' bgcolor='%1' style='color:%2'><b>%3</b></td></tr>%4</table>")
        .arg(LIGHT, ACCENT, escaped(title), rows);
}

// İki kutu yan yana
QString columns(const QString &left, const QString &right)
{
    return QString("<table width='100%' cellspacing='0' cellpadding='0'><tr><td width='49%'>%1</td>"
                   "<td width='2%'></td><td width='49%'>%2</td></tr></table><p></p>")
        .arg(left, right);
}

// Ücret tablosunun satırı
QString chargeRow(const QString &label, const QString &amount, bool strong = false)
{
    const QString style = strong ? "font-size:11pt;font-weight:bold;" : "";
    const QString background = strong ? QString(" bgcolor='%1'").arg(LIGHT) : "";
    return QString("<tr%1><td style='%2'>%3</td><td align='right' style='%2'>%4</td></tr>")
        .arg(background, style, escaped(label), escaped(amount));
}

QString fuel(int eighths)
{
    // Yakıt seviyesi hem sayıyla hem çubukla: 6/8 ■■■■■■■■ (boş kısım soluk renkte)
    const QString full(eighths, QChar(0x25A0));
    const QString empty(FUEL_FULL - eighths, QChar(0x25A0));
    return QString("%1/%2&nbsp;&nbsp;%3<span style='color:#c9d6e6'>%4</span>").arg(eighths).arg(FUEL_FULL).arg(full, empty);
}

} // namespace

namespace ContractPrinter {

QString html(const Rental &rental, const Vehicle &vehicle, const Customer &customer)
{
    const bool returned = rental.status == RentalStatus::Returned;
    const Pricing::Quote quote = Pricing::quote(rental.dailyPrice, rental.startDate, rental.endDate);
    const QString number = QString("%1").arg(rental.id, 5, 10, QChar('0'));

    QString page = "<html><body style='font-family:\"Segoe UI\";font-size:9pt;color:#1a1a1a'>";

    // Başlık bandı: solda belge adı, sağda numara ve tarih
    page += QString("<table width='100%' cellspacing='0' cellpadding='14' bgcolor='%1'><tr>"
                    "<td style='color:white'><span style='font-size:18pt;font-weight:bold'>%2</span><br>%3</td>"
                    "<td align='right' style='color:white'>%4 <b>%5</b><br>%6 <b>%7</b></td></tr></table><p></p>")
                .arg(ACCENT, t(returned ? "receipt_title" : "contract_title"), t("app_name"), t("number"), number,
                     t("date"), escaped(I18n::date(QDate::currentDate())));

    // Kiracı ve araç yan yana
    const QString renter = row(I18n::t("full_name"), customer.fullName) + row(I18n::t("national_id"), customer.nationalId)
                           + row(I18n::t("phone"), I18n::phone(customer.phone))
                           + row(I18n::t("email_short"), customer.email.isEmpty() ? "—" : customer.email)
                           + row(I18n::t("birth_date"), I18n::date(customer.birthDate))
                           + row(I18n::t("license_number"), customer.licenseNumber)
                           + row(I18n::t("license_date"), I18n::date(customer.licenseDate));
    const QString car =
        row(I18n::t("plate"), vehicle.plate)
        + row(I18n::t("vehicle"), vehicle.brand + " " + vehicle.model + " (" + QString::number(vehicle.year) + ")")
        + row(I18n::t("class"), I18n::vehicleClass(vehicle.vehicleClass))
        + row(I18n::t("body_type"), I18n::bodyType(vehicle.bodyType))
        + row(I18n::t("color"), I18n::color(vehicle.color))
        + row(I18n::t("transmission"), I18n::transmission(vehicle.transmission) + " · " + I18n::fuel(vehicle.fuel))
        + row(I18n::t("seats"), I18n::t("seat_count").replace("{0}", QString::number(vehicle.seats)) + " · "
                                     + I18n::t("luggage_count").replace("{0}", QString::number(vehicle.luggage)));
    page += columns(box(I18n::t("renter"), renter), box(I18n::t("vehicle"), car));

    // Kiralama bilgileri: solda teslim, sağda iade (fişte) ya da depozito ve km hakkı (sözleşmede)
    const QString freeKm = I18n::number(quote.days * Pricing::KM_PER_DAY) + " km";
    const QString pickUp = row(I18n::t("start_date"), I18n::date(rental.startDate))
                           + row(I18n::t("end_date"), I18n::date(rental.endDate))
                           + row(I18n::t("rental_period"), I18n::t("days").replace("{0}", QString::number(quote.days)))
                           + row(I18n::t("km_out"), I18n::number(rental.startKm) + " km")
                           + htmlRow(I18n::t("fuel_level"), fuel(rental.fuelOut));
    const QString second = returned ? row(I18n::t("return_date"), I18n::date(rental.returnDate))
                                          + row(I18n::t("km_in"), I18n::number(rental.endKm) + " km")
                                          + row(I18n::t("km_driven"), I18n::number(rental.endKm - rental.startKm) + " km")
                                          + row(I18n::t("free_km"), freeKm)
                                          + htmlRow(I18n::t("fuel_level"), fuel(rental.fuelIn))
                                    : row(I18n::t("daily_price"), I18n::money(rental.dailyPrice))
                                          + row(I18n::t("free_km"), freeKm)
                                          + row(I18n::t("deposit"), I18n::money(rental.deposit));
    page += columns(box(I18n::t("rental_details"), pickUp), box(I18n::t(returned ? "return_info" : "allowances"), second));

    // Ücretler: iade fişinde ek ücretler kalem kalem (kayıttaki km ve yakıttan yeniden hesaplanır)
    QString charges = chargeRow(I18n::t("rental_fee") + " · " + I18n::t("days").replace("{0}", QString::number(quote.days))
                                    + " × " + I18n::money(rental.dailyPrice),
                                I18n::money(quote.base));
    if (quote.discount > 0)
        charges += chargeRow(I18n::t("discount").replace("{0}", QString::number(quote.discountPercent)),
                             "−" + I18n::money(quote.discount));
    if (returned) {
        const Pricing::ReturnCharges c = Pricing::returnCharges(rental, rental.returnDate, rental.endKm, rental.fuelIn);
        if (c.lateFee > 0)
            charges += chargeRow(I18n::t("late_fee").replace("{0}", QString::number(c.lateDays)), I18n::money(c.lateFee));
        if (c.extraKmFee > 0)
            charges += chargeRow(I18n::t("extra_km_fee").replace("{0}", I18n::number(c.extraKm)), I18n::money(c.extraKmFee));
        if (c.fuelFee > 0)
            charges += chargeRow(I18n::t("fuel_fee").replace("{0}", QString::number(c.missingFuel)), I18n::money(c.fuelFee));
        charges += chargeRow(I18n::t("grand_total"), I18n::money(rental.total + rental.extraFees), true);
    } else {
        charges += chargeRow(I18n::t("total"), I18n::money(rental.total), true);
    }
    charges += chargeRow(I18n::t("deposit"), I18n::money(rental.deposit));
    page += QString("<table width='100%' cellspacing='0' cellpadding='4' border='1' style='border-collapse:collapse;"
                    "border-color:#c9d6e6'><tr bgcolor='%1' style='color:white'><td><b>%2</b></td>"
                    "<td align='right'><b>%3</b></td></tr>%4</table>")
                .arg(ACCENT, t("description"), t("amount"), charges);

    if (!rental.notes.isEmpty())
        page += QString("<p><b>%1:</b> %2</p>").arg(t("notes"), escaped(rental.notes));

    // Koşullar: ücret kuralları Pricing sabitlerinden yazılır, hesapla belge hep aynı kalır
    page += QString("<p style='color:%1;font-size:11pt'><b>%2</b></p><ol style='font-size:8.5pt;color:#333'>")
                .arg(ACCENT, t("terms_title"));
    for (int i = 1; i <= TERM_COUNT; ++i) {
        QString term = I18n::t(QString("term_%1").arg(i).toLatin1().constData());
        if (i == 5)
            term.replace("{0}", I18n::money(Pricing::FUEL_FEE_PER_EIGHTH));
        if (i == 6)
            term.replace("{0}", QString::number(Pricing::KM_PER_DAY)).replace("{1}", I18n::money(Pricing::EXTRA_KM_FEE));
        if (i == 7)
            term.replace("{0}", QString::number(Pricing::LATE_DAY_PERCENT));
        page += "<li>" + escaped(term) + "</li>";
    }
    page += "</ol>";

    // Hasar notları için elle doldurulacak boş alan; iade fişinde ücret kalemleri fazla olduğu için daha kısa
    page += QString("<table width='100%' cellspacing='0' cellpadding='8' style='border:1px solid #c9d6e6'>"
                    "<tr><td bgcolor='%1' style='color:%2'><b>%3</b></td></tr>"
                    "<tr><td>%4</td></tr></table><p></p>")
                .arg(LIGHT, ACCENT, t(returned ? "return_notes" : "damage_notes"),
                     QString("&nbsp;<br>").repeated(returned ? 1 : 4)); // yükseklik boş satırlarla verilir

    // İmza kutuları
    const QString signature = "<td width='49%' style='border:1px solid #c9d6e6' height='95' valign='top'>"
                              "<b>%1</b><br><span style='color:" + MUTED + "'>%2</span></td>";
    page += QString("<table width='100%' cellspacing='0' cellpadding='10'><tr>%1<td width='2%'></td>%2</tr></table>")
                .arg(signature.arg(t("signature_renter"), escaped(customer.fullName)),
                     signature.arg(t("signature_office"), t("name_signature")));

    page += QString("<p align='center' style='color:%1;font-size:8pt'>%2</p>")
                .arg(MUTED, escaped(I18n::t("generated_by").replace("{0}", I18n::t("app_name"))));
    return page + "</body></html>";
}

bool savePdf(const QString &html, const QString &path)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(14, 12, 14, 12), QPageLayout::Millimeter);
    writer.setResolution(300);
    writer.setTitle(QFileInfo(path).completeBaseName());

    QTextDocument document;
    // Yazı boyutları PDF'in çözünürlüğüne göre hesaplansın; yoksa 300 dpi sayfada her şey minicik kalır
    document.documentLayout()->setPaintDevice(&writer);
    document.setHtml(html);
    document.setPageSize(writer.pageLayout().paintRectPixels(writer.resolution()).size());
    document.print(&writer);
    return QFileInfo(path).size() > 0; // klasöre yazılamadıysa QPdfWriter sessizce başarısız olur
}

} // namespace ContractPrinter
