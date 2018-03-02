#ifndef GROUPLISTMODEL_H
#define GROUPLISTMODEL_H

#include <QObject>
#include <QAbstractItemModel>

class ForumDatabase;
class ForumSubscription;

class GroupListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    GroupListModel(ForumDatabase *db);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;
private slots:
    void subscriptionFound(ForumSubscription *sub);
    void subscriptionRemoved(ForumSubscription *sub);
private:
    ForumDatabase *m_forumDatabase;
    QList<QVariant> m_headerData;
};

#endif // GROUPLISTMODEL_H
