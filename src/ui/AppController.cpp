#include "AppController.h"
#include "mdm/MdmScanner.h"
#include <QUrl>

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    connect(&m_audio, &AudioEngine::mutedChanged, this, &AppController::mutedChanged);
}

QString AppController::currentImagePath() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_mdms.size())
        return {};
    return m_mdms[m_currentIndex].imagePath;
}

QString AppController::currentDisplayName() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_mdms.size())
        return {};
    return m_mdms[m_currentIndex].displayName;
}

int AppController::currentIndex() const { return m_currentIndex; }
bool AppController::muted() const { return m_audio.isMuted(); }
bool AppController::hasContent() const { return !m_mdms.isEmpty(); }
ThumbnailModel* AppController::thumbnailModel() { return &m_thumbnailModel; }
float AppController::yaw() const { return m_yaw; }
float AppController::pitch() const { return m_pitch; }

void AppController::loadFolder(const QUrl& folderUrl)
{
    const QString folder = folderUrl.toLocalFile();
    if (folder.isEmpty())
        return;

    m_audio.stop();
    m_mdms = MdmScanner::scan(folder);
    m_thumbnailModel.setMdms(m_mdms);
    emit hasContentChanged();

    if (!m_mdms.isEmpty())
        loadMdm(0);
}

void AppController::selectMdm(int index)
{
    if (index < 0 || index >= m_mdms.size() || index == m_currentIndex)
        return;
    loadMdm(index);
}

void AppController::loadMdm(int index)
{
    m_audio.stop();

    m_currentIndex = index;

    // Reset camera to forward-looking position
    m_yaw = 0.0f;
    m_pitch = 0.0f;
    emit yawChanged();
    emit pitchChanged();
    emit currentIndexChanged();
    emit currentImagePathChanged();

    const MdmFile& mdm = m_mdms[index];
    if (!mdm.audioPath.isEmpty()) {
        if (m_audio.loadFile(mdm.audioPath))
            m_audio.play();
    }
}

void AppController::toggleMute()
{
    m_audio.setMuted(!m_audio.isMuted());
}

void AppController::setYaw(float yaw)
{
    if (qFuzzyCompare(m_yaw, yaw))
        return;
    m_yaw = yaw;
    m_audio.setOrientation(m_yaw, m_pitch);
    emit yawChanged();
}

void AppController::setPitch(float pitch)
{
    if (qFuzzyCompare(m_pitch, pitch))
        return;
    m_pitch = pitch;
    m_audio.setOrientation(m_yaw, m_pitch);
    emit pitchChanged();
}
