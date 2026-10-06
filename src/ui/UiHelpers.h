#pragma once

#include "app/I18n.h"
#include "data/Database.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>

// Sayfaların ortak küçük yardımcıları: tablo kurulumu, hücre ekleme, hata mesajı.
namespace Ui {

inline QTableWidget *makeTable(const QStringList &headers, QWidget *parent)
{
    auto *table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); // tablo sadece gösterir; düzenleme formdan
    table->verticalHeader()->hide();
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return table;
}

// Satıra hücre ekler; ilk hücreye kaydın kimliği gizli veri olarak konur (seçilen satırın hangi kayıt olduğu buradan bulunur)
inline void setRow(QTableWidget *table, int row, const QStringList &cells, qint64 id, const QColor &color = QColor())
{
    for (int column = 0; column < cells.size(); ++column) {
        auto *item = new QTableWidgetItem(cells[column]);
        if (column == 0)
            item->setData(Qt::UserRole, id);
        if (color.isValid())
            item->setForeground(color);
        table->setItem(row, column, item);
    }
}

inline qint64 selectedId(QTableWidget *table)
{
    const QList<QTableWidgetItem *> items = table->selectedItems();
    if (items.isEmpty())
        return 0;
    return table->item(items.first()->row(), 0)->data(Qt::UserRole).toLongLong();
}

// Qt'nin hazır düğmeleri (Yes, No, OK) çeviri dosyası olmadan İngilizce görünür; düğmeler kendi metinlerimizle kurulur
inline bool ask(QWidget *parent, const QString &text, bool defaultYes = false, const QString &title = QString())
{
    QMessageBox box(QMessageBox::Question, title.isEmpty() ? I18n::t("app_name") : title, text, QMessageBox::NoButton,
                    parent);
    QPushButton *yes = box.addButton(I18n::t("yes"), QMessageBox::YesRole);
    QPushButton *no = box.addButton(I18n::t("no"), QMessageBox::NoRole);
    box.setDefaultButton(defaultYes ? yes : no); // silme gibi işlemlerde Enter yanlışlıkla onaylamasın
    box.exec();
    return box.clickedButton() == yes;
}

inline void message(QWidget *parent, QMessageBox::Icon icon, const QString &text)
{
    QMessageBox box(icon, I18n::t("app_name"), text, QMessageBox::NoButton, parent);
    box.addButton(I18n::t("ok"), QMessageBox::AcceptRole);
    box.exec();
}

inline void inform(QWidget *parent, const QString &text)
{
    message(parent, QMessageBox::Information, text);
}

inline void warn(QWidget *parent, const QString &text)
{
    message(parent, QMessageBox::Warning, text);
}

inline bool showResult(QWidget *parent, const Result &result)
{
    if (!result.ok())
        warn(parent, I18n::error(result.error));
    return result.ok();
}

inline QPushButton *accentButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setProperty("accent", true); // stil sayfasındaki QPushButton[accent="true"] kuralı
    return button;
}

} // namespace Ui
