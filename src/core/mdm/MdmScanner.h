#pragma once
#include "MdmFile.h"
#include <QList>
#include <QString>

class MdmScanner {
public:
    // Scans folderPath for .JPG/.jpg and .ambisonic.opus files,
    // matches audio to images by ZFP_XXXX.JPG tokens in the audio filename,
    // and returns a list sorted by image filename (which encodes timestamp).
    static QList<MdmFile> scan(const QString& folderPath);
};
