#pragma once

#include <QString>

// Araç fotoğrafları kullanıcı klasöründeki "photos" klasöründe tutulur.
// Seçilen dosya olduğu gibi kopyalanmaz: resim olarak açılır, küçültülür ve yeniden JPEG kaydedilir.
// Böylece resim olmayan ya da içine başka veri gizlenmiş bir dosya klasöre giremez.
namespace PhotoStore {

void setFolder(const QString &folder);
QString importPhoto(const QString &sourcePath); // yeni dosya adı; açılamazsa boş
QString path(const QString &name);              // adı geçersizse boş
void remove(const QString &name);

} // namespace PhotoStore