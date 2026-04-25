#pragma once
#include <QAbstractListModel>
#include <QList>
#include <QVector>
#include "mdm/MdmFile.h"

class ThumbnailModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        DisplayNameRole = Qt::UserRole + 1,
        ImagePathRole,
        HasAudioRole,
        WaveformRole,     // QVariantList of floats 0.0–1.0, empty while computing
    };

    explicit ThumbnailModel(QObject* parent = nullptr);

    void setMdms(const QList<MdmFile>& mdms);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    void computeWaveformAsync(int row, const QString& audioPath, int generation);

    QList<MdmFile> m_mdms;
    QVector<QVector<float>> m_waveforms;
    int m_generation = 0;   // incremented on each setMdms() to discard stale results
};
