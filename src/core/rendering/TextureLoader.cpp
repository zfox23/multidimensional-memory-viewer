#include "TextureLoader.h"
#include <QtConcurrent/QtConcurrent>
#include <QImageReader>

TextureLoader::TextureLoader(QObject* parent)
    : QObject(parent)
{}

void TextureLoader::load(const QString& path)
{
    m_future = QtConcurrent::run([this, path]() {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QImage img = reader.read();
        if (img.isNull())
            emit loadFailed(path, reader.errorString());
        else
            emit imageReady(img, path);
    });
}

void TextureLoader::cancel()
{
    m_future.cancel();
}

QImage TextureLoader::makeThumbnail(const QString& path, int size)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);

    // The MDM image is an 8192×4096 SBS image: 2:1 width-to-height ratio,
    // with the left-eye square occupying the left half (4096×4096).
    // Ask the decoder to produce a proportionally scaled image where the
    // left eye occupies exactly size×size pixels after cropping.
    // Scaling the full image to (size*2 × size) preserves the 2:1 ratio,
    // so the left half is exactly size×size — no distortion, no over-crop.
    const QSize nativeSize = reader.size();
    if (nativeSize.isValid())
        reader.setScaledSize(QSize(size * 2, size));

    QImage img = reader.read();
    if (img.isNull())
        return {};

    // Crop to the left-eye square (left size × size pixels)
    return img.copy(0, 0, size, img.height());
}
