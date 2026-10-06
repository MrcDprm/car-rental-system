#include "services/PhotoStore.h"

#include "core/Rules.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QUuid>

namespace {

constexpr qint64 MAX_FILE_SIZE = 10 * 1024 * 1024; // 10 MB
constexpr int MAX_SOURCE_SIDE = 10000;             // daha büyüğü bellek sorununa yol açar
constexpr int MAX_SAVED_WIDTH = 1280;

QString g_folder;

} // namespace

namespace PhotoStore {

void setFolder(const QString &folder)
{
    g_folder = folder;
    QDir().mkpath(folder);
}

QString importPhoto(const QString &sourcePath)
{
    const QFileInfo info(sourcePath);
    if (!info.isFile() || info.size() == 0 || info.size() > MAX_FILE_SIZE || g_folder.isEmpty())
        return QString();

    // Tür dosya adının uzantısından değil içeriğinden anlaşılır
    QImageReader reader(sourcePath);
    reader.setDecideFormatFromContent(true);
    const QByteArray format = reader.format();
    if (format != "jpeg" && format != "png")
        return QString();
    const QSize size = reader.size();
    if (!size.isValid() || size.width() > MAX_SOURCE_SIDE || size.height() > MAX_SOURCE_SIDE)
        return QString();
    reader.setAutoTransform(true); // telefon fotoğrafındaki döndürme bilgisi uygulanır
    QImage image = reader.read();
    if (image.isNull())
        return QString();

    if (image.width() > MAX_SAVED_WIDTH)
        image = image.scaledToWidth(MAX_SAVED_WIDTH, Qt::SmoothTransformation);
    // Ad tahmin edilemez ve dosya sisteminde güvenli: 32 onaltılık karakter
    const QString name = QUuid::createUuid().toString(QUuid::Id128) + ".jpg";
    if (!image.convertToFormat(QImage::Format_RGB32).save(g_folder + "/" + name, "JPG", 85))
        return QString();
    return name;
}

QString path(const QString &name)
{
    if (g_folder.isEmpty() || !Rules::isValidPhotoName(name))
        return QString();
    return g_folder + "/" + name;
}

void remove(const QString &name)
{
    const QString file = path(name);
    if (!file.isEmpty())
        QFile::remove(file);
}

} // namespace PhotoStore