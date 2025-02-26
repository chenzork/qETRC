﻿#include "raildbmodel.h"
#include "raildb.h"
#include "data/rail/railway.h"

RailDBModel::RailDBModel(std::shared_ptr<RailDB> raildb, QObject *parent) :
    QAbstractItemModel(parent), _raildb(raildb),
    _root(std::make_unique<navi::RailCategoryItem>(raildb,0,nullptr))
{

}

QModelIndex RailDBModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!hasIndex(row,column,parent)) return {};
    auto* parentItem=getParentItem(parent);
    auto* childItem=parentItem->child(row);
    if(childItem){
        return createIndex(row,column,childItem);
    }else return {};
}

QModelIndex RailDBModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    pACI childItem = static_cast<pACI>(child.internalPointer());
    pACI parentItem = childItem->parent();

    if (!parentItem || parentItem == _root.get())
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int RailDBModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    auto* parentItem = getParentItem(parent);
    if (parentItem)
        return parentItem->childCount();
    return 0;
}

int RailDBModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return navi::AbstractComponentItem::DBColMAX;
}

QVariant RailDBModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    const ACI* item = static_cast<ACI*>(index.internalPointer());

    if(item)
        return item->data(index.column());
    return {};
}

QVariant RailDBModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section)
        {
        case navi::AbstractComponentItem::DBColName: return tr("线名");
        case navi::AbstractComponentItem::DBColMile:return tr("里程");
        case navi::AbstractComponentItem::DBColStart:return tr("起点");
        case navi::AbstractComponentItem::DBColEnd:return tr("终点");
        case navi::AbstractComponentItem::DBColAuthor:return tr("作者");
        case navi::AbstractComponentItem::DBColVersion:return tr("版本");
        default:return {};
        }
    }
    return {};
}

RailDBModel::pACI RailDBModel::getItem(const QModelIndex &idx) const
{
    if(idx.isValid())
        return static_cast<pACI>(idx.internalPointer());
    else return nullptr;
}

QModelIndex RailDBModel::indexByPath(const std::deque<int> &path)
{
    QModelIndex idx{};
    for(int i:path){
        idx=index(i,0,idx);
    }
    return idx;
}

QModelIndex RailDBModel::parentIndexByPath(const std::deque<int>& path)
{
    if (path.empty())return {};
    QModelIndex idx{};
    for (auto p = path.begin(); p != std::prev(path.end()); ++p) {
        idx = index(*p, 0, idx);
    }
    return idx;
}

QModelIndex RailDBModel::railIndexBrute(std::shared_ptr<Railway> railway)
{
    return railIndexBruteFrom(railway,{});
}

QModelIndex RailDBModel::categoryIndexBrute(std::shared_ptr<RailCategory> category)
{
    return categoryIndexBruteFrom(category, {});
}

std::deque<navi::path_t> RailDBModel::searchFullName(const QString &name)
{
    auto func=[&name](const Railway& rail)->bool{
        return bool(rail.stationByName(name));
    };
    return _root->searchBy(func);
}

std::deque<navi::path_t> RailDBModel::searchPartName(const QString &name)
{
    auto func=[&name](const Railway& rail)->bool{
        return bool(rail.stationByGeneralName(name));
    };
    return _root->searchBy(func);
}

std::deque<navi::path_t> RailDBModel::searchRailName(const QString &name)
{
    auto func=[&name](const Railway& rail)->bool{
        return rail.name().contains(name);
    };
    return _root->searchBy(func);
}

std::shared_ptr<Railway> RailDBModel::railwayByPath(const navi::path_t &path)
{
    auto it=_root->itemByPath(path);
    if(it&&it->type()==navi::RailwayItemDB::Type){
        return static_cast<navi::RailwayItemDB*>(it)->railway();
    }else return {};
}

RailDBModel::pACI RailDBModel::getParentItem(const QModelIndex &parent)const
{
    if(parent.isValid()){
        return static_cast<pACI>(parent.internalPointer());
    }else{
        return _root.get();
    }
}

QModelIndex RailDBModel::railIndexBruteFrom(std::shared_ptr<Railway> railway,
                                            const QModelIndex &idx)
{
    auto* it=getParentItem(idx);
    for(int i=0;i<it->childCount();i++){
        auto* sub=it->child(i);
        if(!sub) continue;
        if(sub->type()==navi::RailCategoryItem::Type){
            // 递归
            auto subres=railIndexBruteFrom(railway,index(i,0,idx));
            if(subres.isValid()){
                return subres;
            }
        }else if(sub->type()==navi::RailwayItemDB::Type){
            if (static_cast<navi::RailwayItemDB*>(sub)->railway()==railway){
                return index(sub->row(),0,idx);
            }
        }
    }
    return {};
}

QModelIndex RailDBModel::categoryIndexBruteFrom(std::shared_ptr<RailCategory> cat, const QModelIndex& idx)
{
    // 能进来递归的肯定是Category类型
    auto* it = static_cast<navi::RailCategoryItem*>(getParentItem(idx));
    if (it->category() == cat)
        return idx;
    for (int i = 0; i < it->childCount(); i++) {
        auto* sub = it->child(i);
        if (!sub) continue;
        if (sub->type() == navi::RailCategoryItem::Type) {
            // 递归
            auto subres = categoryIndexBruteFrom(cat, index(i, 0, idx));
            if (subres.isValid()) {
                return subres;
            }
        }
    }
    return {};
}

std::shared_ptr<Railway> RailDBModel::railwayByIndex(const QModelIndex& idx)
{
    if (!idx.isValid())return nullptr;
    if (auto* it = static_cast<pACI>(idx.internalPointer());
        it->type() == navi::RailwayItemDB::Type) {
        return static_cast<navi::RailwayItemDB*>(it)->railway();
    }
    return nullptr;
}

std::shared_ptr<RailCategory> RailDBModel::categoryByIndex(const QModelIndex& idx)
{
    if (!idx.isValid())return nullptr;
    if (auto* it = static_cast<pACI>(idx.internalPointer());
        it->type() == navi::RailCategoryItem::Type) {
        return static_cast<navi::RailCategoryItem*>(it)->category();
    }
    return nullptr;
}

void RailDBModel::onRailInfoChanged(std::shared_ptr<Railway> railway, const std::deque<int>& path)
{
    auto idx = indexByPath(path);
    if (!idx.isValid() || railwayByIndex(idx)!=railway) {
        qDebug() << "RailDBModel::onRailInfoChanged: WARNING: invalid path to railway: " <<
            railway->name() << ", will use brute-force alg. " << Qt::endl;
        idx = railIndexBrute(railway);
    }
    emit dataChanged(idx, index(idx.row(), ACI::DBColMAX - 1, idx.parent()), 
        { Qt::EditRole });
}

void RailDBModel::onCategoryInfoChanged(std::shared_ptr<RailCategory> cat, const std::deque<int>& path)
{
    auto idx = indexByPath(path);
    if (!idx.isValid() || categoryByIndex(idx) != cat) {
        qDebug() << "RailDBModel::commitRemoveRailwaysAt: WARNING: " <<
            "Invalid path, use brute-force" << Qt::endl;
        idx = categoryIndexBrute(cat);
    }
    emit dataChanged(idx, index(idx.row(), ACI::DBColMAX - 1, idx.parent()),
        { Qt::EditRole });
}

void RailDBModel::commitRemoveRailwayAt(std::shared_ptr<Railway> railway, 
    const std::deque<int>& path)
{
    auto idx = indexByPath(path);
    if (!idx.isValid() || railwayByIndex(idx) != railway) {
        qDebug() << "RailDBModel::commitRemoveRailwayAt: WARNING: " <<
            "Invalid path, use brute-force" << Qt::endl;
        idx = railIndexBrute(railway);
    }
    // 现在：idx指向要被删除的东西。
    const auto& par = idx.parent();
    beginRemoveRows(par, idx.row(), idx.row());
    auto* par_it = static_cast<navi::RailCategoryItem*>(par.internalPointer());
    par_it->removeRailwayAt(path.back());
    endRemoveRows();
}

void RailDBModel::commitInsertRailwayAt(std::shared_ptr<Railway> railway, const std::deque<int>& path)
{
    // 不用检查 （没办法检查），直接按照path插入。
    // 如果path出现越界啥的，直接让抛异常好了
    auto par = parentIndexByPath(path);
    beginInsertRows(par, path.back(), path.back());
    auto* par_it = static_cast<navi::RailCategoryItem*>(getParentItem(par));
    par_it->insertRailwayAt(railway, path.back());
    endInsertRows();
}

void RailDBModel::commitInsertRailwaysAt(const QList<std::shared_ptr<Railway>>& rails,
    const std::deque<int>& path)
{
    auto par = parentIndexByPath(path);
    beginInsertRows(par, path.back(), path.back() + rails.size() - 1);
    auto* par_it = static_cast<navi::RailCategoryItem*>(par.internalPointer());
    par_it->insertRailwaysAt(rails, path.back());
    endInsertRows();
}

void RailDBModel::commitRemoveRailwaysAt(const QList<std::shared_ptr<Railway>>& rails, const std::deque<int>& path)
{
    auto idx = indexByPath(path);
    if (!idx.isValid() || railwayByIndex(idx) != rails.front()) {
        qDebug() << "RailDBModel::commitRemoveRailwaysAt: WARNING: " <<
            "Invalid path, use brute-force" << Qt::endl;
        idx = railIndexBrute(rails.front());
    }

    // 现在：idx指向要被删除的东西。
    const auto& par = idx.parent();
    beginRemoveRows(par, idx.row(), idx.row() + rails.size() - 1);
    auto* par_it = static_cast<navi::RailCategoryItem*>(par.internalPointer());
    par_it->removeRailwaysAt(path.back(), rails.size());
    endRemoveRows();
}

void RailDBModel::commitInsertCategoryAt(std::shared_ptr<RailCategory> cat,
    const std::deque<int>& path)
{
    auto par = parentIndexByPath(path);
    beginInsertRows(par, path.back(), path.back());
    // 这里要注意：如果par无效，应当对根节点进行操作
    auto* it = static_cast<navi::RailCategoryItem*>(getParentItem(par));
    it->insertCategoryAt(cat, path.back());
    endInsertRows();
}

void RailDBModel::commitRemoveCategoryAt(std::shared_ptr<RailCategory> cat, 
    const std::deque<int>& path)
{
    auto idx = indexByPath(path);
    if (!idx.isValid() || categoryByIndex(idx) != cat) {
        qDebug() << "RailDBModel::commitRemoveRailwaysAt: WARNING: " <<
            "Invalid path, use brute-force" << Qt::endl;
        idx = categoryIndexBrute(cat);
    }

    // 现在: idx指向要被删除的东西
    const auto& par = idx.parent();
    auto* par_it = static_cast<navi::RailCategoryItem*>(getParentItem(par));
    beginRemoveRows(par, path.back(), path.back());
    par_it->removeCategoryAt(path.back());
    endRemoveRows();
}

void RailDBModel::resetModel()
{
    beginResetModel();
    _root=std::make_unique<navi::RailCategoryItem>(_raildb,0,nullptr);
    endResetModel();
}
