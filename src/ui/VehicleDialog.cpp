#include "ui/VehicleDialog.h"

#include "app/Theme.h"
#include "core/Money.h"
#include "data/VehicleRepository.h"
#include "services/PhotoStore.h"
#include "ui/CarArt.h"
#include "ui/UiHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

const QSize PREVIEW_SIZE(300, 135);

// Enum sırasıyla doldurulan liste: seçili satırın sırası enum değeridir
template <typename Enum, typename NameFunction>
QComboBox *enumCombo(Enum last, Enum selected, NameFunction name, QWidget *parent)
{
    auto *combo = new QComboBox(parent);
    for (int i = 0; i <= static_cast<int>(last); ++i)
        combo->addItem(name(static_cast<Enum>(i)));
    combo->setCurrentIndex(static_cast<int>(selected));
    return combo;
}

QSpinBox *spinBox(int low, int high, int value, const QString &suffix, QWidget *parent)
{
    auto *box = new QSpinBox(parent);
    box->setRange(low, high);
    box->setSuffix(suffix);
    box->setGroupSeparatorShown(true);
    box->setValue(value);
    return box;
}

} // namespace

VehicleDialog::VehicleDialog(Database &db, const Vehicle &vehicle, QWidget *parent)
    : QDialog(parent), m_db(db), m_vehicle(vehicle), m_photo(vehicle.photo)
{
    setWindowTitle(I18n::t(vehicle.id ? "edit_vehicle" : "new_vehicle"));
    const int thisYear = QDate::currentDate().year();

    m_plate = new QLineEdit(vehicle.plate, this);
    m_plate->setMaxLength(12);
    m_plate->setPlaceholderText("34 ABC 123");
    m_brand = new QLineEdit(vehicle.brand, this);
    m_brand->setMaxLength(100);
    m_model = new QLineEdit(vehicle.model, this);
    m_model->setMaxLength(100);
    m_year = spinBox(1990, thisYear + 1, vehicle.year ? vehicle.year : thisYear, QString(), this);
    m_year->setGroupSeparatorShown(false); // 2.024 değil 2024
    m_class = enumCombo(VehicleClass::Luxury, vehicle.vehicleClass, I18n::vehicleClass, this);
    m_body = enumCombo(BodyType::Pickup, vehicle.bodyType, I18n::bodyType, this);
    m_color = enumCombo(CarColor::Brown, vehicle.color, I18n::color, this);
    m_transmission = enumCombo(Transmission::Automatic, vehicle.transmission, I18n::transmission, this);
    m_fuel = enumCombo(Fuel::Lpg, vehicle.fuel, I18n::fuel, this);
    m_seats = spinBox(2, 9, vehicle.seats, QString(), this);
    m_luggage = spinBox(0, 9, vehicle.luggage, QString(), this);
    m_price = new QDoubleSpinBox(this);
    m_price->setRange(0, 100000);
    m_price->setDecimals(2);
    m_price->setGroupSeparatorShown(true);
    m_price->setSuffix(" ₺");
    m_price->setValue(Money::toLira(vehicle.dailyPrice));
    m_mileage = spinBox(0, 2'000'000, vehicle.mileage, " km", this);
    m_nextService = spinBox(0, 2'000'000, vehicle.nextServiceKm ? vehicle.nextServiceKm : 15000, " km", this);

    auto *form = new QFormLayout;
    form->addRow(I18n::t("plate"), m_plate);
    form->addRow(I18n::t("brand"), m_brand);
    form->addRow(I18n::t("model"), m_model);
    form->addRow(I18n::t("year"), m_year);
    form->addRow(I18n::t("class"), m_class);
    form->addRow(I18n::t("body_type"), m_body);
    form->addRow(I18n::t("color"), m_color);
    form->addRow(I18n::t("transmission"), m_transmission);
    form->addRow(I18n::t("fuel"), m_fuel);
    form->addRow(I18n::t("seats"), m_seats);
    form->addRow(I18n::t("luggage"), m_luggage);
    form->addRow(I18n::t("daily_price"), m_price);
    form->addRow(I18n::t("mileage"), m_mileage);
    form->addRow(I18n::t("next_service"), m_nextService);

    // Sağ sütun: görsel, fotoğraf düğmeleri ve donanım
    m_preview = new QLabel(this);
    m_preview->setFixedSize(PREVIEW_SIZE);
    m_preview->setAlignment(Qt::AlignCenter);
    auto *choose = new QPushButton(I18n::t("choose_photo"), this);
    m_removePhoto = new QPushButton(I18n::t("remove_photo"), this);
    auto *hint = new QLabel(I18n::t("photo_hint"), this);
    hint->setObjectName("muted");
    hint->setWordWrap(true);
    auto *photoButtons = new QHBoxLayout;
    photoButtons->addWidget(choose);
    photoButtons->addWidget(m_removePhoto);

    auto *equipment = new QGroupBox(I18n::t("features"), this);
    auto *grid = new QGridLayout(equipment);
    for (int bit = 0; bit < Feature::COUNT; ++bit) {
        auto *box = new QCheckBox(I18n::feature(bit), equipment);
        box->setChecked(vehicle.features & (1 << bit));
        grid->addWidget(box, bit / 2, bit % 2);
        m_features << box;
    }

    auto *side = new QVBoxLayout;
    side->addWidget(m_preview);
    side->addLayout(photoButtons);
    side->addWidget(hint);
    side->addSpacing(8);
    side->addWidget(equipment);
    side->addStretch();

    auto *columns = new QHBoxLayout;
    columns->addLayout(form, 1);
    columns->addSpacing(16);
    columns->addLayout(side);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(columns);
    layout->addWidget(m_error);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &VehicleDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(choose, &QPushButton::clicked, this, &VehicleDialog::choosePhoto);
    connect(m_removePhoto, &QPushButton::clicked, this, &VehicleDialog::removePhoto);
    // Kasa tipi ya da renk değişince çizim hemen güncellenir
    connect(m_body, &QComboBox::currentIndexChanged, this, &VehicleDialog::updatePreview);
    connect(m_color, &QComboBox::currentIndexChanged, this, &VehicleDialog::updatePreview);
    updatePreview();
}

void VehicleDialog::choosePhoto()
{
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString path = QFileDialog::getOpenFileName(this, I18n::t("choose_photo"), folder,
                                                      I18n::t("images") + " (*.jpg *.jpeg *.png)");
    if (path.isEmpty())
        return;
    const QString name = PhotoStore::importPhoto(path);
    if (name.isEmpty()) {
        m_error->setText(I18n::error("invalid_photo"));
        return;
    }
    removePhoto(); // kaydedilmemiş önceki seçim varsa diskten silinir
    m_photo = name;
    m_error->clear();
    updatePreview();
}

void VehicleDialog::removePhoto()
{
    // Kayıtlı fotoğraf hemen silinmez: kullanıcı Vazgeç'e basarsa geri dönebilmeli. Sadece bu pencerede
    // seçilip henüz kaydedilmemiş fotoğraf silinir.
    if (!m_photo.isEmpty() && m_photo != m_vehicle.photo)
        PhotoStore::remove(m_photo);
    m_photo.clear();
    updatePreview();
}

void VehicleDialog::updatePreview()
{
    Vehicle v;
    v.bodyType = static_cast<BodyType>(m_body->currentIndex());
    v.color = static_cast<CarColor>(m_color->currentIndex());
    v.photo = m_photo;
    m_preview->setPixmap(CarArt::image(v, PREVIEW_SIZE));
    m_removePhoto->setEnabled(!m_photo.isEmpty());
}

void VehicleDialog::done(int result)
{
    // Vazgeç ya da pencere kapatma: bu pencerede seçilen ama kaydedilmeyen fotoğraf diskte kalmasın
    if (result != QDialog::Accepted && m_photo != m_vehicle.photo)
        PhotoStore::remove(m_photo);
    QDialog::done(result);
}

void VehicleDialog::save()
{
    Vehicle v = m_vehicle;
    v.plate = m_plate->text();
    v.brand = m_brand->text();
    v.model = m_model->text();
    v.year = m_year->value();
    v.vehicleClass = static_cast<VehicleClass>(m_class->currentIndex());
    v.bodyType = static_cast<BodyType>(m_body->currentIndex());
    v.color = static_cast<CarColor>(m_color->currentIndex());
    v.transmission = static_cast<Transmission>(m_transmission->currentIndex());
    v.fuel = static_cast<Fuel>(m_fuel->currentIndex());
    v.seats = m_seats->value();
    v.luggage = m_luggage->value();
    v.features = 0;
    for (int bit = 0; bit < m_features.size(); ++bit)
        if (m_features[bit]->isChecked())
            v.features |= 1 << bit;
    v.photo = m_photo;
    v.dailyPrice = Money::fromLira(m_price->value());
    v.mileage = m_mileage->value();
    v.nextServiceKm = m_nextService->value();

    VehicleRepository repo(m_db);
    const Result result = v.id ? repo.update(v) : repo.add(v);
    if (!result.ok()) {
        m_error->setText(I18n::error(result.error));
        return;
    }
    if (m_vehicle.photo != m_photo)
        PhotoStore::remove(m_vehicle.photo); // değiştirilen ya da kaldırılan eski fotoğraf
    m_vehicle.photo = m_photo;               // done() yeni fotoğrafı silmesin
    accept();
}
