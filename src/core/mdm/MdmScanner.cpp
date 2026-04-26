#include "MdmScanner.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

QList<MdmFile> MdmScanner::scan(const QString& folderPath)
{
    QDir dir(folderPath);
    if (!dir.exists())
        return {};

    // Collect image and audio files separately
    QStringList imageFiles;
    QStringList audioFiles;
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Files, QDir::Name)) {
        const QString name = fi.fileName();
        if (name.endsWith(QStringLiteral(".opus"), Qt::CaseInsensitive))
            audioFiles << fi.absoluteFilePath();
        else if (name.endsWith(QStringLiteral(".JPG"), Qt::CaseInsensitive)
                 || name.endsWith(QStringLiteral(".jpg"), Qt::CaseInsensitive)
                 || name.endsWith(QStringLiteral(".jpeg"), Qt::CaseInsensitive)
            imageFiles << fi.absoluteFilePath();
    }

    // Index images by their ZFP_XXXX.JPG token (e.g. "ZFP_3447.JPG")
    // A token is just the filename without the leading timestamp prefix.
    // The filename is like: "2026-04-25 13-48-43 ZFP_3447.JPG"
    // The token embedded in audio filenames is just the bare "ZFP_3447" portion.
    QHash<QString, QString> tokenToImagePath;
    for (const QString& imgPath : imageFiles) {
        const QString name = QFileInfo(imgPath).fileName();
        // Extract the token: everything from "ZFP_" onwards (case-insensitive)
        static const QRegularExpression tokenRx(QStringLiteral("(ZFP_\\d+)"),
                                                QRegularExpression::CaseInsensitiveOption);
        const auto m = tokenRx.match(name);
        if (m.hasMatch())
            tokenToImagePath[m.captured(1).toUpper()] = imgPath;
    }

    // Match each audio file to one or more images
    static const QRegularExpression audioTokenRx(QStringLiteral("(ZFP_\\d+)"),
                                                  QRegularExpression::CaseInsensitiveOption);

    QList<MdmFile> results;
    for (const QString& audioPath : audioFiles) {
        const QString audioName = QFileInfo(audioPath).fileName();
        QRegularExpressionMatchIterator it = audioTokenRx.globalMatch(audioName);
        while (it.hasNext()) {
            const QString token = it.next().captured(1).toUpper();
            if (tokenToImagePath.contains(token)) {
                const QString imgPath = tokenToImagePath[token];
                MdmFile mdm;
                mdm.imagePath = imgPath;
                mdm.audioPath = audioPath;
                mdm.displayName = QFileInfo(imgPath).completeBaseName();
                results << mdm;
            }
        }
    }

    // Images without any matching audio file are also included (audio path left empty)
    QSet<QString> matchedImages;
    for (const MdmFile& m : results)
        matchedImages.insert(m.imagePath);

    for (const QString& imgPath : imageFiles) {
        if (!matchedImages.contains(imgPath)) {
            MdmFile mdm;
            mdm.imagePath = imgPath;
            mdm.displayName = QFileInfo(imgPath).completeBaseName();
            results << mdm;
        }
    }

    // Sort by image filename (which begins with a sortable timestamp)
    std::sort(results.begin(), results.end(), [](const MdmFile& a, const MdmFile& b) {
        return QFileInfo(a.imagePath).fileName() < QFileInfo(b.imagePath).fileName();
    });

    return results;
}
