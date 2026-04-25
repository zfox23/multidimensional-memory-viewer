#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "mdm/MdmFile.h"
#include "audio/AudioEngine.h"
#include "ThumbnailModel.h"

class AppController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentImagePath READ currentImagePath NOTIFY currentImagePathChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
    Q_PROPERTY(bool hasContent READ hasContent NOTIFY hasContentChanged)
    Q_PROPERTY(QString currentDisplayName READ currentDisplayName NOTIFY currentIndexChanged)
    Q_PROPERTY(ThumbnailModel* thumbnailModel READ thumbnailModel CONSTANT)
    Q_PROPERTY(float yaw READ yaw WRITE setYaw NOTIFY yawChanged)
    Q_PROPERTY(float pitch READ pitch WRITE setPitch NOTIFY pitchChanged)

public:
    explicit AppController(QObject* parent = nullptr);

    QString currentImagePath() const;
    QString currentDisplayName() const;
    int currentIndex() const;
    bool muted() const;
    bool hasContent() const;
    ThumbnailModel* thumbnailModel();
    float yaw() const;
    float pitch() const;

    Q_INVOKABLE void loadFolder(const QUrl& folderUrl);
    Q_INVOKABLE void selectMdm(int index);
    Q_INVOKABLE void toggleMute();
    void setYaw(float yaw);
    void setPitch(float pitch);

signals:
    void currentImagePathChanged();
    void currentIndexChanged();
    void mutedChanged();
    void hasContentChanged();
    void yawChanged();
    void pitchChanged();

private:
    void loadMdm(int index);

    QList<MdmFile> m_mdms;
    int m_currentIndex = -1;
    AudioEngine m_audio;
    ThumbnailModel m_thumbnailModel;
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
};
