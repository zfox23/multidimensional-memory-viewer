#pragma once
#include <QAbstractListModel>
#include <QList>
#include "mdm/MdmFile.h"

class ThumbnailModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        DisplayNameRole = Qt::UserRole + 1,
        ImagePathRole,
        HasAudioRole,
    };

    explicit ThumbnailModel(QObject* parent = nullptr);

    void setMdms(const QList<MdmFile>& mdms);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<MdmFile> m_mdms;
};
