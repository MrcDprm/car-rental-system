#include "ui/RentalDialog.h"

#include "app/Theme.h"
#include "core/Money.h"
#include "core/Pricing.h"
#include "data/CustomerRepository.h"
#include "data/RentalRepository.h"
#include "data/VehicleRepository.h"
#include "ui/CustomerDialog.h"
#include "ui/UiHelpers.h"
#include "ui/VehicleCard.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QDateEdit *dateEdit(const QDate &value, QWidget *parent)
{
    auto *edit = new QDateEdit(value, parent);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat("dd.MM.yyyy");
    return edit;
}

// İlk satırı "Tümü" olan filtre listesi; sonraki satırlar enum sırasıyla
template <typename Enum, typename NameFunction>
QComboBox *filterCombo(Enum last, NameFunction name, QWidget *parent)
{
    auto *combo = new QComboBox(parent);
    combo->addItem(I18n::t("all"));
    for (int i = 0; i <= static_cast<int>(last); ++i)
        combo->addItem(name(static_cast<Enum>(i)));
    return combo;
}

// Seçili filtre değeri; "Tümü" seçiliyse -1
int filterValue(const QComboBox *combo)
{
    return combo->currentIndex() - 1;
}

QWidget *labeled(const QString &label, QWidget *field, QWidget *parent)
{
    auto *box = new QWidget(parent);
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    auto *caption = new QLabel(label, box);
    caption->setObjectName("muted");
    layout->addWidget(caption);
    layout->addWidget(field);
    return box;
}

} // namespace

RentalDialog::RentalDialog(Database &db, QWidget *parent, qint64 vehicleId)
    : QDialog(parent), m_db(db), m_preselect(vehicleId)
{
    setWindowTitle(I18n::t("new_rental"));
    const QDate today = QDate::currentDate();

    // Üst satır: müşteri ve tarihler
    m_customer = new QComboBox(this);
    auto *newCustomer = new QPushButton("+ " + I18n::t("new_customer"), this);
    auto *customerRow = new QWidget(this);
    auto *customerLayout = new QHBoxLayout(customerRow);
    customerLayout->setContentsMargins(0, 0, 0, 0);
    customerLayout->addWidget(m_customer, 1);
    customerLayout->addWidget(newCustomer);
    m_start = dateEdit(today, this);
    m_start->setMinimumDate(today); // geçmişe rezervasyon yapılamaz
    m_end = dateEdit(today.addDays(3), this);
    m_end->setMinimumDate(today.addDays(1));
    auto *top = new QHBoxLayout;
    top->addWidget(labeled(I18n::t("customer"), customerRow, this), 2);
    top->addWidget(labeled(I18n::t("start_date"), m_start, this), 1);
    top->addWidget(labeled(I18n::t("end_date"), m_end, this), 1);

    // Filtreler
    m_budget = new QSpinBox(this);
    m_budget->setRange(0, 20000);
    m_budget->setSingleStep(250);
    m_budget->setGroupSeparatorShown(true);
    m_budget->setSuffix(" ₺");
    m_budget->setSpecialValueText(I18n::t("no_limit")); // 0 = sınırsız
    m_class = filterCombo(VehicleClass::Luxury, I18n::vehicleClass, this);
    m_transmission = filterCombo(Transmission::Automatic, I18n::transmission, this);
    m_fuel = filterCombo(Fuel::Lpg, I18n::fuel, this);
    m_minSeats = new QSpinBox(this);
    m_minSeats->setRange(2, 9);
    m_minLuggage = new QSpinBox(this);
    m_minLuggage->setRange(0, 6);
    m_sort = new QComboBox(this);
    m_sort->addItems({I18n::t("sort_cheap"), I18n::t("sort_expensive")});
    auto *filters = new QHBoxLayout;
    filters->addWidget(labeled(I18n::t("max_budget"), m_budget, this));
    filters->addWidget(labeled(I18n::t("class"), m_class, this));
    filters->addWidget(labeled(I18n::t("transmission"), m_transmission, this));
    filters->addWidget(labeled(I18n::t("fuel"), m_fuel, this));
    filters->addWidget(labeled(I18n::t("min_seats"), m_minSeats, this));
    filters->addWidget(labeled(I18n::t("min_luggage"), m_minLuggage, this));
    filters->addWidget(labeled(I18n::t("sort"), m_sort, this));

    // Kartlar: sığdığı kadar yan yana, satır satır akar
    m_count = new QLabel(this);
    m_count->setObjectName("muted");
    m_cards = new QListWidget(this);
    m_cards->setViewMode(QListView::IconMode);
    m_cards->setResizeMode(QListView::Adjust);
    m_cards->setMovement(QListView::Static);
    m_cards->setUniformItemSizes(true);
    m_cards->setSpacing(0);
    m_cards->setSelectionMode(QAbstractItemView::SingleSelection);
    m_cards->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_delegate = new VehicleCardDelegate(m_cards);
    m_cards->setItemDelegate(m_delegate);

    // Alt kısım: fiyat, depozito, not
    m_quote = new QLabel(this);
    m_quote->setStyleSheet("font-weight: 600;");
    m_deposit = new QDoubleSpinBox(this);
    m_deposit->setRange(0, 1'000'000);
    m_deposit->setDecimals(2);
    m_deposit->setGroupSeparatorShown(true);
    m_deposit->setSuffix(" ₺");
    m_deposit->setValue(2000);
    m_notes = new QLineEdit(this);
    m_notes->setMaxLength(500);
    m_pickUpNow = new QCheckBox(I18n::t("pick_up_now"), this);
    m_pickUpNow->setChecked(true);
    auto *form = new QFormLayout;
    form->addRow(I18n::t("total"), m_quote);
    form->addRow(I18n::t("deposit"), m_deposit);
    form->addRow(I18n::t("notes"), m_notes);
    form->addRow(m_pickUpNow);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    auto *buttons = new QDialogButtonBox(this);
    m_save = Ui::accentButton(I18n::t("save"), this);
    buttons->addButton(m_save, QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addLayout(filters);
    layout->addWidget(m_count);
    layout->addWidget(m_cards, 1);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &RentalDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // Teslim tarihi değişince dönüş en az bir gün sonrası olur; tarih değişince boş araçlar yeniden aranır
    connect(m_start, &QDateEdit::dateChanged, this, [this](const QDate &start) {
        m_end->setMinimumDate(start.addDays(1));
        m_pickUpNow->setEnabled(start == QDate::currentDate()); // ileri tarihli kiralama bugün teslim edilemez
        loadVehicles();
    });
    connect(m_end, &QDateEdit::dateChanged, this, &RentalDialog::loadVehicles);
    for (QComboBox *combo : {m_class, m_transmission, m_fuel, m_sort})
        connect(combo, &QComboBox::currentIndexChanged, this, &RentalDialog::applyFilters);
    for (QSpinBox *box : {m_budget, m_minSeats, m_minLuggage})
        connect(box, &QSpinBox::valueChanged, this, &RentalDialog::applyFilters);
    connect(m_cards, &QListWidget::itemSelectionChanged, this, &RentalDialog::updateQuote);
    connect(newCustomer, &QPushButton::clicked, this, &RentalDialog::addCustomer);

    loadCustomers(0);
    loadVehicles();
    resize(1180, 740); // dört kart yan yana sığar
}

void RentalDialog::loadCustomers(qint64 select)
{
    m_customer->clear();
    for (const Customer &c : CustomerRepository(m_db).all())
        m_customer->addItem(c.fullName + "  ·  " + I18n::phone(c.phone), c.id); // görünen metin + gizli kimlik
    if (select != 0)
        m_customer->setCurrentIndex(m_customer->findData(select));
    const bool any = m_customer->count() > 0;
    m_save->setEnabled(any);
    m_error->setText(any ? QString() : I18n::t("no_customers"));
}

void RentalDialog::addCustomer()
{
    CustomerDialog dialog(m_db, Customer{}, this);
    if (dialog.exec() == QDialog::Accepted)
        loadCustomers(dialog.savedId()); // yeni müşteri listeye eklenir ve seçilir
}

void RentalDialog::loadVehicles()
{
    m_free = VehicleRepository(m_db).availableBetween(m_start->date(), m_end->date());
    m_delegate->setVehicles(m_free);
    m_delegate->setDates(m_start->date(), m_end->date());
    applyFilters();
}

void RentalDialog::applyFilters()
{
    // Seçim korunur; pencere bir araçla açıldıysa ilk listede o araç seçili gelir
    const qint64 previous = m_preselect ? std::exchange(m_preselect, 0) : selectedVehicle();
    const qint64 budget = qint64(m_budget->value()) * 100; // kuruş; 0 = sınırsız
    QList<Vehicle> shown;
    for (const Vehicle &v : m_free) {
        if ((budget > 0 && v.dailyPrice > budget) || v.seats < m_minSeats->value() || v.luggage < m_minLuggage->value()
            || (filterValue(m_class) >= 0 && int(v.vehicleClass) != filterValue(m_class))
            || (filterValue(m_transmission) >= 0 && int(v.transmission) != filterValue(m_transmission))
            || (filterValue(m_fuel) >= 0 && int(v.fuel) != filterValue(m_fuel)))
            continue;
        shown << v;
    }
    const bool cheapFirst = m_sort->currentIndex() == 0;
    std::stable_sort(shown.begin(), shown.end(), [cheapFirst](const Vehicle &a, const Vehicle &b) {
        return cheapFirst ? a.dailyPrice < b.dailyPrice : a.dailyPrice > b.dailyPrice;
    });

    m_cards->clear();
    for (const Vehicle &v : shown) {
        auto *item = new QListWidgetItem(m_cards);
        item->setData(VehicleCardDelegate::ID_ROLE, v.id);
        item->setToolTip(v.brand + " " + v.model + " · " + v.plate);
        if (v.id == previous) {
            m_cards->setCurrentItem(item);
            m_cards->scrollToItem(item);
        }
    }
    m_count->setText(I18n::t("matching").replace("{0}", QString::number(shown.size())));
    updateQuote();
}

qint64 RentalDialog::selectedVehicle() const
{
    const QList<QListWidgetItem *> items = m_cards->selectedItems();
    return items.isEmpty() ? 0 : items.first()->data(VehicleCardDelegate::ID_ROLE).toLongLong();
}

void RentalDialog::updateQuote()
{
    const qint64 id = selectedVehicle();
    const auto it = std::find_if(m_free.cbegin(), m_free.cend(), [id](const Vehicle &v) { return v.id == id; });
    if (it == m_free.cend()) {
        m_quote->setText(m_free.isEmpty() ? I18n::t("no_vehicle_free") : I18n::t("choose_card"));
        return;
    }
    // Kayıtta kullanılan hesaplamanın aynısı: ekranda görülen fiyat ile kaydedilen fiyat hep aynı
    const Pricing::Quote quote = Pricing::quote(it->dailyPrice, m_start->date(), m_end->date());
    QString text = it->brand + " " + it->model + "  ·  "
                   + I18n::t("price_summary")
                         .replace("{0}", I18n::t("days").replace("{0}", QString::number(quote.days)))
                         .replace("{1}", I18n::money(it->dailyPrice))
                         .replace("{2}", I18n::money(quote.base));
    if (quote.discount > 0)
        text += "  −  " + I18n::t("discount").replace("{0}", QString::number(quote.discountPercent)) + "  =  "
                + I18n::money(quote.total);
    m_quote->setText(text);
}

void RentalDialog::save()
{
    Rental rental;
    rental.customerId = m_customer->currentData().toLongLong();
    rental.vehicleId = selectedVehicle();
    rental.startDate = m_start->date();
    rental.endDate = m_end->date();
    rental.deposit = Money::fromLira(m_deposit->value());
    rental.notes = m_notes->text().trimmed();
    if (rental.vehicleId == 0) {
        m_error->setText(I18n::t(m_free.isEmpty() ? "no_vehicle_free" : "choose_card"));
        return;
    }

    RentalRepository repo(m_db);
    const Result reserved = repo.reserve(rental);
    if (!reserved.ok()) {
        m_error->setText(I18n::error(reserved.error));
        return;
    }
    if (m_pickUpNow->isEnabled() && m_pickUpNow->isChecked()) {
        const auto vehicle = VehicleRepository(m_db).find(rental.vehicleId);
        const Result picked = repo.pickUp(reserved.id, vehicle ? vehicle->mileage : 0, FUEL_FULL);
        if (!picked.ok()) {
            // Rezervasyon kaydedildi ama teslim yapılamadı: pencere kapanır, teslim listeden tekrar denenir
            Ui::showResult(this, picked);
        }
    }
    accept();
}
