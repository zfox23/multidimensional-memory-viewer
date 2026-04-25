#include "ThumbnailModel.h"
#include "audio/OpusReader.h"
#include "audio/AudioEngine.h"
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

ThumbnailModel::ThumbnailModel(QObject* parent)
    : QAbstractListModel(parent)
{}

void ThumbnailModel::setMdms(const QList<MdmFile>& mdms)
{
    ++m_generation;
    beginResetModel();
    m_mdms = mdms;
    m_waveforms.assign(mdms.size(), {});
    endResetModel();

    for (int i = 0; i < mdms.size(); ++i) {
        if (!mdms[i].audioPath.isEmpty())
            computeWaveformAsync(i, mdms[i].audioPath, m_generation);
    }
}

void ThumbnailModel::computeWaveformAsync(int row, const QString& audioPath, int generation)
{
    auto future = QtConcurrent::run([audioPath]() -> QVector<float> {
        auto result = OpusReader::decode(audioPath);
        if (!result.ok) return {};
        return AudioEngine::computeWaveform(result.pcm, result.channels);
    });

    auto* watcher = new QFutureWatcher<QVector<float>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this,
            [this, watcher, row, generation]() {
        watcher->deleteLater();
        if (generation != m_generation) return;     // model was reset, discard
        if (row >= m_waveforms.size()) return;
        m_waveforms[row] = watcher->result();
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {WaveformRole});
    });
    watcher->setFuture(future);
}

int ThumbnailModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_mdms.size();
}

QVariant ThumbnailModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_mdms.size())
        return {};

    const MdmFile& mdm = m_mdms[idx.row()];
    switch (role) {
    case DisplayNameRole: return mdm.displayName;
    case ImagePathRole:   return mdm.imagePath;
    case HasAudioRole:    return !mdm.audioPath.isEmpty();
    case WaveformRole: {
        const QVector<float>& wf = m_waveforms[idx.row()];
        QVariantList list;
        list.reserve(wf.size());
        for (float v : wf) list.append(static_cast<double>(v));
        return list;
    }
    default: return {};
    }
}

QHash<int, QByteArray> ThumbnailModel::roleNames() const
{
    return {
        { DisplayNameRole, "displayName" },
        { ImagePathRole,   "imagePath"   },
        { HasAudioRole,    "hasAudio"    },
        { WaveformRole,    "waveform"    },
    };
}
