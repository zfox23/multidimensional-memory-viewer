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

    // Read at reduced scale to avoid loading the full 8192×4096 image
    const QSize nativeSize = reader.size();
    if (nativeSize.isValid()) {
        // Left-eye half is the left nativeSize.width()/2 × full height
        const int halfW = nativeSize.width() / 2;
        const QSize scaleTarget(size * 2, size * 2); // slightly larger than needed
        reader.setScaledSize(QSize(
            qMax(size, halfW * scaleTarget.height() / nativeSize.height()),
            scaleTarget.height()
        ));
    }

    QImage img = reader.read();
    if (img.isNull())
        return {};

    // Crop to left eye half
    const int leftW = img.width() / 2;
    img = img.copy(0, 0, leftW, img.height());

    // Scale to square thumbnail
    return img.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
              .copy(0, 0, size, size);
}
