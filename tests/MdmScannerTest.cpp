#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include "mdm/MdmScanner.h"

class MdmScannerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {}

    void singleImageWithAudio()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile::copy(QString(), dir.filePath("2026-04-25 13-48-43 ZFP_3447.JPG"));
        touch(dir.filePath("2026-04-25 13-48-43 ZFP_3447.JPG"));
        touch(dir.filePath("2026-04-25 13-48-43 ZFP_3447.JPG SPTL004.ambisonic.opus"));

        const auto mdms = MdmScanner::scan(dir.path());
        QCOMPARE(mdms.size(), 1);
        QVERIFY(mdms[0].imagePath.endsWith("ZFP_3447.JPG"));
        QVERIFY(mdms[0].audioPath.endsWith("SPTL004.ambisonic.opus"));
    }

    void oneAudioMultipleImages()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        touch(dir.filePath("2026-04-25 13-48-43 ZFP_3447.JPG"));
        touch(dir.filePath("2026-04-25 13-49-23 ZFP_3448.JPG"));
        touch(dir.filePath("2026-04-25 13-48-43 ZFP_3447.JPG ZFP_3448.JPG SPTL004.ambisonic.opus"));

        const auto mdms = MdmScanner::scan(dir.path());
        QCOMPARE(mdms.size(), 2);

        // Both entries should point to the same audio file
        const QString audioPath = mdms[0].audioPath;
        QVERIFY(!audioPath.isEmpty());
        QCOMPARE(mdms[0].audioPath, mdms[1].audioPath);

        // Images should be sorted by filename (which begins with timestamp)
        QVERIFY(QFileInfo(mdms[0].imagePath).fileName() <
                QFileInfo(mdms[1].imagePath).fileName());
    }

    void imageWithoutAudio()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        touch(dir.filePath("2026-04-25 14-00-00 ZFP_3500.JPG"));

        const auto mdms = MdmScanner::scan(dir.path());
        QCOMPARE(mdms.size(), 1);
        QVERIFY(mdms[0].audioPath.isEmpty());
    }

    void emptyFolder()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const auto mdms = MdmScanner::scan(dir.path());
        QCOMPARE(mdms.size(), 0);
    }

    void nonexistentFolder()
    {
        const auto mdms = MdmScanner::scan(QStringLiteral("/no/such/folder/xyz"));
        QCOMPARE(mdms.size(), 0);
    }

private:
    static void touch(const QString& path)
    {
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.close();
    }
};

QTEST_MAIN(MdmScannerTest)
#include "MdmScannerTest.moc"
