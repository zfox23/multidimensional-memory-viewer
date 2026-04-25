#include "ThumbnailModel.h"

ThumbnailModel::ThumbnailModel(QObject* parent)
    : QAbstractListModel(parent)
{}

void ThumbnailModel::setMdms(const QList<MdmFile>& mdms)
{
    beginResetModel();
    m_mdms = mdms;
    endResetModel();
}

int ThumbnailModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_mdms.size();
}

QVariant ThumbnailModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_mdms.size())
        return {};

    const MdmFile& mdm = m_mdms[index.row()];
    switch (role) {
    case DisplayNameRole: return mdm.displayName;
    case ImagePathRole:   return mdm.imagePath;
    case HasAudioRole:    return !mdm.audioPath.isEmpty();
    default:              return {};
    }
}

QHash<int, QByteArray> ThumbnailModel::roleNames() const
{
    return {
        { DisplayNameRole, "displayName" },
        { ImagePathRole,   "imagePath"   },
        { HasAudioRole,    "hasAudio"    },
    };
}
