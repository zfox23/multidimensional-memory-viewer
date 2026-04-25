#pragma once
#include <QObject>
#include <QImage>
#include <QFuture>

// Asynchronously loads large images (e.g., 8192×4096 JPG) on a thread pool.
// Emits imageReady() on the calling thread when loading is complete.
class TextureLoader : public QObject {
    Q_OBJECT
public:
    explicit TextureLoader(QObject* parent = nullptr);

    void load(const QString& path);
    void cancel();

    // Returns a 200×200 thumbnail cropped from the left-eye half of an SBS image.
    static QImage makeThumbnail(const QString& path, int size = 200);

signals:
    void imageReady(const QImage& image, const QString& path);
    void loadFailed(const QString& path, const QString& error);

private:
    QFuture<void> m_future;
};
