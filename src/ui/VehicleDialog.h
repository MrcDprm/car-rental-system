#pragma once

#include "core/Models.h"

#include <QDialog>

class Database;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

// Araç ekleme ve düzenleme formu. Kaydet'e basınca VehicleRepository kuralları kontrol eder;
// hata varsa form kapanmaz ve mesaj formun altında gösterilir. Sağda aracın görseli ve fotoğraf seçimi bulunur.
class VehicleDialog : public QDialog
{
    Q_OBJECT

public:
    VehicleDialog(Database &db, const Vehicle &vehicle, QWidget *parent);
    void done(int result) override;

private:
    void choosePhoto();
    void removePhoto();
    void updatePreview();
    void save();

    Database &m_db;
    Vehicle m_vehicle;
    QString m_photo; // formda seçili fotoğraf; kaydedilene kadar m_vehicle.photo eski hâlini tutar
    QLineEdit *m_plate, *m_brand, *m_model;
    QSpinBox *m_year, *m_seats, *m_luggage, *m_mileage, *m_nextService;
    QComboBox *m_class, *m_body, *m_color, *m_transmission, *m_fuel;
    QDoubleSpinBox *m_price;
    QList<QCheckBox *> m_features;
    QLabel *m_preview, *m_error;
    QPushButton *m_removePhoto;
};