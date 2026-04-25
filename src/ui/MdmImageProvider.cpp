#include "MdmImageProvider.h"
#include "rendering/TextureLoader.h"
#include <QThreadPool>
#include <QUrl>

class ThumbnailResponse : public QQuickImageResponse, public QRunnable {
public:
    ThumbnailResponse(const QString& path, const QSize& requestedSize)
        : m_path(path), m_size(requestedSize.isValid() ? requestedSize.width() : 200)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        m_image = TextureLoader::makeThumbnail(m_path, m_size);
        emit finished();
    }

    QQuickTextureFactory* textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    QString m_path;
    int m_size;
    QImage m_image;
};

QQuickImageResponse* MdmThumbnailProvider::requestImageResponse(const QString& id,
                                                                  const QSize& requestedSize)
{
    // The id arrives as a path (URL-encoded by Qt Quick, decode it)
    const QString path = QUrl::fromPercentEncoding(id.toUtf8());
    auto* response = new ThumbnailResponse(path, requestedSize);
    QThreadPool::globalInstance()->start(response);
    return response;
}
