﻿#pragma once

#include <QAbstractItemModel>
#include <memory>

#include "raildbitems.h"
class RailDB;

class RailDBModel : public QAbstractItemModel
{
    Q_OBJECT;
    std::shared_ptr<RailDB> _raildb;
    std::unique_ptr<navi::RailCategoryItem> _root;

    using ACI=navi::AbstractComponentItem;
    using pACI=ACI*;

public:
    explicit RailDBModel(std::shared_ptr<RailDB> raildb, QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    pACI getItem(const QModelIndex& idx)const;
    QModelIndex indexByPath(const std::deque<int>& path);

    /**
     * @brief railIndexBrute
     * 暴力查找 （DFS）指定线路。一般用不到；只在Path失效的情况紧急替代。
     */
    QModelIndex railIndexBrute(std::shared_ptr<Railway> railway);
private:
    /**
     * @brief getParentItem
     * @param parent
     * 获取parent对应的节点指针。如果是invalid，则理解为根指针。
     */
    pACI getParentItem(const QModelIndex& parent)const;
    QModelIndex railIndexBruteFrom(std::shared_ptr<Railway> railway,
                                   const QModelIndex& idx);
    std::shared_ptr<Railway> railwayByIndex(const QModelIndex& idx);
public slots:
    void resetModel();

    /**
     * 指定线路变更后，更新数据。
     */
    void onRailInfoChanged(std::shared_ptr<Railway> railway, const std::deque<int>& path);
};

