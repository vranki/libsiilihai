#include "grouplistmodel.h"
#include "../forumdatabase/forumdatabase.h"
#include "forumsubscription.h"
#include <QDebug>

GroupListModel::GroupListModel(ForumDatabase *db) : QAbstractItemModel()
  , m_forumDatabase(db)
{
    connect(m_forumDatabase, &ForumDatabase::subscriptionFound, this, &GroupListModel::subscriptionFound);
    qDebug() << Q_FUNC_INFO;
    m_headerData << "Name" << "Unreads";
    beginInsertColumns(QModelIndex(), 0, 1);
    endInsertColumns();
}

QModelIndex GroupListModel::index(int row, int column, const QModelIndex &parent) const
{
    // Invalid row
    if(row < 0 || row > m_forumDatabase->count()-1)
        return QModelIndex();
    if(column < 0 || column > 1)
        return QModelIndex();

    //if (!hasIndex(row, column, parent))
    //    return QModelIndex();

    if(parent.isValid()) {
        qDebug() << Q_FUNC_INFO << row << column << parent.row() << parent.isValid() << "Returning invalid";
        return QModelIndex();
    } else {
        qDebug() << Q_FUNC_INFO << row << column << parent.row() << parent.isValid() << "Returning index for " << m_forumDatabase->at(row)->alias();
        return createIndex(row, column, m_forumDatabase->at(row));
    }
}

QModelIndex GroupListModel::parent(const QModelIndex &child) const
{
    qDebug() << Q_FUNC_INFO << child.row() << child.isValid();
    if (!child.isValid())
        return QModelIndex();
    return QModelIndex();
}

int GroupListModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid()) {
        qDebug() << Q_FUNC_INFO << parent.row() << "valid parent, returning 0";
        return 0;
    } else {
        qDebug() << Q_FUNC_INFO << "invalid parent, returning" << m_forumDatabase->count();
        return m_forumDatabase->count();
    }
}

int GroupListModel::columnCount(const QModelIndex &parent) const
{
    qDebug() << Q_FUNC_INFO;
    return 2;
}

QVariant GroupListModel::data(const QModelIndex &index, int role) const
{
    qDebug() << Q_FUNC_INFO << role << index.row() << index.isValid();
    if (!index.isValid())
        return QVariant();

    ForumSubscription *item = static_cast<ForumSubscription*>(index.internalPointer());
    Q_ASSERT(item);
    if(role == Qt::DisplayRole) {
        qDebug() << Q_FUNC_INFO << "returning" << item->alias();
        return item->alias();
    } else if(role == Qt::UserRole) {
        qDebug() << Q_FUNC_INFO << "returning" << QString::number(item->unreadCount());
        return QString::number(item->unreadCount());
    } else {
        qDebug() << Q_FUNC_INFO << "Unknown role" << role;
    }
    return QVariant();
}

void GroupListModel::subscriptionFound(ForumSubscription *sub)
{
    qDebug() << Q_FUNC_INFO;
    QModelIndex parent = QModelIndex();
    int index = m_forumDatabase->count()-1;
    beginInsertRows(parent, index, index);
    endInsertRows();
}

void GroupListModel::subscriptionRemoved(ForumSubscription *sub)
{
    qDebug() << Q_FUNC_INFO;
    QModelIndex parent = QModelIndex();
    /// Ffuu, can't be done easily
}

Qt::ItemFlags GroupListModel::flags(const QModelIndex &index) const
{
    qDebug() << Q_FUNC_INFO << index.isValid();

    return QAbstractItemModel::flags(index);
}


QVariant GroupListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_headerData;

    return QVariant();
}


QHash<int, QByteArray> GroupListModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { Qt::DisplayRole, "name" },
        { Qt::UserRole, "unreads" },
    };
    return roles;
}
