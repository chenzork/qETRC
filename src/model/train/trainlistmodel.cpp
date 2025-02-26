﻿#include "trainlistmodel.h"


#include "editors/trainlistwidget.h"
#include "data/train/traincollection.h"
#include "data/train/train.h"
#include "data/train/traintype.h"

TrainListModel::TrainListModel(TrainCollection& collection, QUndoStack* undo, QObject* parent):
	QAbstractTableModel(parent), coll(collection), _undo(undo)
{
}

int TrainListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return coll.size();
}

int TrainListModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return MAX_COLUMNS;
}

QVariant TrainListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return {};
	std::shared_ptr<Train> t = coll.trainAt(index.row());
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case ColTrainName:return t->trainName().full();
		case ColStarting:return t->starting().toSingleLiteral();
		case ColTerminal:return t->terminal().toSingleLiteral();
		case ColType:return t->type()->name();
		case ColMile:return QString::number(t->localMile(), 'f', 3);
		case ColSpeed:return QString::number(t->localTraverseSpeed(), 'f', 3);
		}
	}
	else if (role == Qt::CheckStateRole) {
		if (index.column() == ColShow)
			return t->isShow() ? Qt::Checked : Qt::Unchecked;
	}
	return {};
}

bool TrainListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid())
		return false;
	auto t = coll.trainAt(index.row());
	if (role == Qt::CheckStateRole) {
		if (index.column() == ColShow) {
			Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
			bool show = (state == Qt::Checked);
			if (show != t->isShow()) {
				emit trainShowChanged(t, show);
				return true;
			}
		}
	}
	return false;
}

Qt::ItemFlags TrainListModel::flags(const QModelIndex& index) const
{
	switch (index.column()) {
	case ColShow:return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
	}
	return QAbstractTableModel::flags(index);
}

void TrainListModel::sort(int column, Qt::SortOrder order)
{
	//把旧版的列表复制一份
	QList<std::shared_ptr<Train>> oldList(coll.trains());   //copy construct!!
	beginResetModel();
	auto& lst = coll.trains();
	if (order == Qt::AscendingOrder) {
		switch (column) {
        case ColTrainName:std::stable_sort(lst.begin(), lst.end(), &Train::ltName); break;
        case ColStarting:std::stable_sort(lst.begin(), lst.end(), &Train::ltStarting); break;
        case ColTerminal:std::stable_sort(lst.begin(), lst.end(), &Train::ltTerminal); break;
        case ColType:std::stable_sort(lst.begin(), lst.end(), &Train::ltType); break;
        case ColShow:std::stable_sort(lst.begin(), lst.end(), &Train::ltShow); break;
        case ColMile:std::stable_sort(lst.begin(), lst.end(), &Train::ltMile); break;
        case ColSpeed:std::stable_sort(lst.begin(), lst.end(), &Train::ltTravSpeed); break;
		default:break;
		}
	}
	else {
		switch (column) {
        case ColTrainName:std::stable_sort(lst.begin(), lst.end(), &Train::gtName); break;
        case ColStarting:std::stable_sort(lst.begin(), lst.end(), &Train::gtStarting); break;
        case ColTerminal:std::stable_sort(lst.begin(), lst.end(), &Train::gtTerminal); break;
        case ColType:std::stable_sort(lst.begin(), lst.end(), &Train::gtType); break;
        case ColShow:std::stable_sort(lst.begin(), lst.end(), &Train::gtShow); break;
        case ColMile:std::stable_sort(lst.begin(), lst.end(), &Train::gtMile); break;
        case ColSpeed:std::stable_sort(lst.begin(), lst.end(), &Train::gtTravSpeed); break;
		default:break;
		}
	}
	
	endResetModel();

	if (oldList != lst) {
		if (_undo) {
			//注意压栈后的第一个Redo是不执行的
			_undo->push(new qecmd::SortTrains(oldList, this));
		}
	}
}

QVariant TrainListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole&&orientation==Qt::Horizontal) {
		switch (section) {
		case ColTrainName:return QObject::tr("车次");
		case ColStarting:return QObject::tr("始发站");
		case ColTerminal:return QObject::tr("终到站");
		case ColType:return QObject::tr("类型");
		case ColShow:return QObject::tr("显示");
		case ColMile:return QObject::tr("铺画里程");
		case ColSpeed:return QObject::tr("铺画旅速");
		}
	}
	return QAbstractTableModel::headerData(section, orientation, role);
}

void TrainListModel::undoRedoSort(QList<std::shared_ptr<Train>>& lst)
{
	beginResetModel();
	std::swap(coll.trains(), lst);
	endResetModel();
}

void TrainListModel::redoRemoveTrains(const QList<std::shared_ptr<Train>>& trains,
	const QList<int>& indexes)
{
	beginResetModel();
	//注意：倒序遍历
	for (auto p = indexes.rbegin(); p != indexes.rend(); ++p) {
		std::shared_ptr<Train> train(coll.takeTrainAt(*p));   //move 
	}
	endResetModel();
	emit trainsRemovedRedone(trains);
}

void TrainListModel::undoRemoveTrains(const QList<std::shared_ptr<Train>>& trains, 
	const QList<int>& indexes)
{
	beginResetModel();
	for (int i = 0; i < trains.size(); i++) {
		coll.insertTrainForUndo(indexes.at(i), trains.at(i));
	}
	endResetModel();
	emit trainsRemovedUndone(trains);
}

void TrainListModel::onTrainChanged(std::shared_ptr<Train> train)
{
	int idx = coll.getTrainIndex(train);
	if (idx != -1) {
		emit dataChanged(index(idx, ColTrainName), index(idx, MAX_COLUMNS - 1));
	}
}

void TrainListModel::updateAllMileSpeed()
{
	//coll.invalidateAllTempData();
	emit dataChanged(index(0, ColMile), index(coll.trainCount() - 1, ColSpeed));
}

void TrainListModel::updateAllTrainShow()
{
	emit dataChanged(index(0, ColShow), index(coll.trainCount() - 1, ColShow));
}

void TrainListModel::updateAllTrainStartingTerminal()
{
	emit dataChanged(index(0, ColStarting), index(coll.trainCount() - 1, ColTerminal),
		{ Qt::DisplayRole });
}

void TrainListModel::updateAllTrainTypes()
{
	emit dataChanged(index(0, ColType), index(coll.trainCount() - 1, ColType), 
                     { Qt::DisplayRole });
}

void TrainListModel::refreshData()
{
    beginResetModel();
    endResetModel();
}

void TrainListModel::commitBatchChangeType(const QVector<int>& rows)
{
	foreach(int r, rows) {
		emit dataChanged(index(r, ColType), index(r, ColType));
	}
	emit onTypeBatchChanged();
}



qecmd::RemoveTrains::RemoveTrains(const QList<std::shared_ptr<Train>>& trains,
    const QList<int>& indexes, TrainCollection& coll_, TrainListModel* model_,
    QUndoCommand* parent) :
    QUndoCommand(QObject::tr("删除") + QString::number(trains.size()) + QObject::tr("个车次"), parent),
    _trains(trains), _indexes(indexes), coll(coll_), model(model_)
{
}

void qecmd::RemoveTrains::undo()
{
    model->undoRemoveTrains(_trains, _indexes);
}

void qecmd::RemoveTrains::redo()
{
    model->redoRemoveTrains(_trains, _indexes);
}

qecmd::SortTrains::SortTrains(const QList<std::shared_ptr<Train>>& ord_,
    TrainListModel* model_, QUndoCommand* parent):
    QUndoCommand(QObject::tr("列车排序"),parent),ord(ord_),model(model_)
{
}

void qecmd::SortTrains::undo()
{
    model->undoRedoSort(ord);
}

void qecmd::SortTrains::redo()
{
    if (first) {
        first = false;
        return;
    }
    model->undoRedoSort(ord);
}

bool qecmd::SortTrains::mergeWith(const QUndoCommand* another)
{
    if (id() != another->id())
        return false;
    auto cmd = static_cast<const qecmd::SortTrains*>(another);
    if (model == cmd->model) {
        //只针对同一个model的排序做合并
        //成功合并：抛弃中间状态
        return true;
    }
    return false;
}

qecmd::BatchChangeType::BatchChangeType(TrainCollection& coll_, const QVector<int>& indexes_, std::shared_ptr<TrainType> type,
    TrainListModel* model_, QUndoCommand* parent) :
    QUndoCommand(QObject::tr("批量更新%1个车次类型").arg(indexes_.size()), parent),
    coll(coll_), indexes(indexes_), types(indexes_.size(), type), model(model_)
{
}

void qecmd::BatchChangeType::commit()
{
    for (int i = 0; i < indexes.size(); i++) {
        int index = indexes.at(i);
        auto train = coll.trainAt(index);
        std::swap(train->typeRef(), types[i]);
        coll.updateTrainType(train, types[i]);
    }
    model->commitBatchChangeType(indexes);
}


