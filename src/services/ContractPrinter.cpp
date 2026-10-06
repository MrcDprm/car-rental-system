#include "services/ContractPrinter.h"

#include "app/I18n.h"
#include "core/Pricing.h"

#include <QFileInfo>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>

namespace {

// Kayıtlardaki metinler HTML'e kaçışlanarak girer: müşteri adında "<b>" olsa bile biçim olarak çalışmaz
QString escaped(const QString &text)
{
    return text.toHtmlEscaped();
}

QString row(const QString &label, const QString &value)
{
    return QString("<tr><td style='color:#555;padding:3px 12px 3px 0'>%1</td><td style='padding:3px 0'>%2</td></tr>")
        .arg(escaped(label), escaped(value));
}

} // namespace

namespace ContractPrinter {

QString html(const Rental &rental, const Vehicle &vehicle, const Customer &customer)
{
    using I18n::t;
    const bool returned = rental.status == RentalStatus::Returned;
    const Pricing::Quote quote = Pricing::quote(rental.dailyPrice, rental.startDate, rental.endDate);

    QString page = "<html><body style='font-family:Segoe UI;font-size:10pt;color:#000'>";
    page += QString("<h2 style='margin-bottom:0'>%1</h2>").arg(escaped(t(returned ? "receipt_title" : "contract_title")));
    page += QString("<p style='color:#555;margin-top:2px'>%1 · %2 %3 · %4</p>")
                .arg(escaped(t("app_name")), escaped(t("number")), QString("%1").arg(rental.id, 5, 10, QChar('0')),
                     escaped(I18n::date(QDate::currentDate())));

    page += QString("<h3>%1</h3><table>").arg(escaped(t("customer")));
    page += row(t("full_name"), customer.fullName) + row(t("national_id"), customer.nationalId)
            + row(t("phone"), I18n::phone(customer.phone)) + row(t("license_number"), customer.licenseNumber);
    page += "</table>";

    page += QString("<h3>%1</h3><table>").arg(escaped(t("vehicle")));
    page += row(t("plate"), vehicle.plate) + row(t("vehicle"), vehicle.brand + " " + vehicle.model)
            + row(t("color"), I18n::color(vehicle.color))
            + row(t("start_date"), I18n::date(rental.startDate)) + row(t("end_date"), I18n::date(rental.endDate))
            + row(t("km_out"), I18n::number(rental.startKm))
            + row(t("fuel_level"), QString("%1/8").arg(rental.fuelOut));
    if (returned)
        page += row(t("return_date"), I18n::date(rental.returnDate)) + row(t("km_in"), I18n::number(rental.endKm))
                + row(t("fuel_level"), QString("%1/8").arg(rental.fuelIn));
    page += "</table>";

    page += QString("<h3>%1</h3><table>").arg(escaped(t("total")));
    page += row(t("rental_fee"), QString("%1 × %2").arg(t("days").replace("{0}", QString::number(quote.days)),
                                                       I18n::money(rental.dailyPrice)));
    if (quote.discountPercent > 0)
        page += row(t("discount").replace("{0}", QString::number(quote.discountPercent)), "−" + I18n::money(quote.discount));
    page += row(t("total"), I18n::money(rental.total));
    if (returned)
        page += row(t("extra_total"), I18n::money(rental.extraFees))
                + row(t("grand_total"), I18n::money(rental.total + rental.extraFees));
    page += row(t("deposit"), I18n::money(rental.deposit));
    page += "</table>";

    if (!rental.notes.isEmpty())
        page += QString("<p><b>%1:</b> %2</p>").arg(escaped(t("notes")), escaped(rental.notes));
    page += QString("<p style='color:#555;font-size:9pt'>%1</p>").arg(escaped(t("contract_terms")));
    page += QString("<table width='100%' style='margin-top:40px'><tr><td>%1<br><br>__________________</td>"
                    "<td align='right'>%2<br><br>__________________</td></tr></table>")
                .arg(escaped(t("signature_renter")), escaped(t("signature_office")));
    return page + "</body></html>";
}

bool savePdf(const QString &html, const QString &path)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(18, 18, 18, 18), QPageLayout::Millimeter);
    writer.setResolution(300);
    QTextDocument document;
    document.setHtml(html);
    document.setPageSize(writer.pageLayout().paintRectPixels(writer.resolution()).size());
    document.print(&writer);
    return QFileInfo(path).size() > 0; // klasöre yazılamadıysa QPdfWriter sessizce başarısız olur
}

} // namespace ContractPrinter