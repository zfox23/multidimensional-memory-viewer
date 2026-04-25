#pragma once
#include <QQuickAsyncImageProvider>
#include <QRunnable>
#include <QImage>

// Provides thumbnails for the strip via "image://mdmthumbnail/<absolute-path>"
class MdmThumbnailProvider : public QQuickAsyncImageProvider {
public:
    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize& requestedSize) override;
};
