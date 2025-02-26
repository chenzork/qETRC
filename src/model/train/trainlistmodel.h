﻿#pragma once


#include <QAbstractTableModel>
#include <QUndoCommand>
#include <memory>

class Train;
class TrainCollection;

struct Config;




/**
 * @brief The TrainListModel class
 * 采用类似只读的模式实现  有关操作立即响应
 * 注意全程应当是类似单例的模式 （Diagram只有一个TrainCollection） 采用引用
 * 暂定由上层的Widget (原来的pyETRC.TrainWidget)负责删除
 */
class TrainListModel : public QAbstractTableModel
{
    Q_OBJECT

    TrainCollection& coll;
    QUndoStack*const _undo;

    enum Columns {
        ColTrainName=0,
        ColStarting,
        ColTerminal,
        ColType,
        ColShow,
        ColMile,
        ColSpeed,
        MAX_COLUMNS
    };
public:
    friend class TrainListWidget;

    TrainListModel(TrainCollection& coll,QUndoStack* undo, QObject* parent);

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    //调整的接口 下次再实现
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
    //virtual bool removeRows(int row, int count, const QModelIndex &parent) override;
    //virtual bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    virtual void sort(int column, Qt::SortOrder order) override;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

signals:
    /**
     * 操作压栈  发送给viewCategory处理
     */
    void trainShowChanged(std::shared_ptr<Train> train, bool show);

    /**
     * 这个信号只用来通告MainWindow，让NaviTree重新排序一下
     */
    //void informTrainSorted();

    void trainsRemovedUndone(const QList<std::shared_ptr<Train>>& trains);
    void trainsRemovedRedone(const QList<std::shared_ptr<Train>>& trains);

    /**
     * 2022.02.06 批量更新类型
     * 通知TrainContext更新一下数据即可
     */
    void onTypeBatchChanged();

public slots:
    /**
     * 撤销或重做排序，操作都一样。由UndoCommand调用
     * 重新排序，然后提醒MainWindow那边更新
     */
    void undoRedoSort(QList<std::shared_ptr<Train>>& lst);

    /**
     * 执行删除，然后通告Main那边调整顺序 （转发给Navi）
     */
    void redoRemoveTrains(const QList<std::shared_ptr<Train>>& trainsconst,const  QList<int>& indexes);

    void undoRemoveTrains(const QList<std::shared_ptr<Train>>& trains,
        const QList<int>& indexes);

    /**
     * 指定列车数据更改 （目前仅考虑时刻表更改）
     * 更新数据。暂定线性查找
     */
    void onTrainChanged(std::shared_ptr<Train> train);

    /**
     * 这几个slot用来接收navitree那边删除单个列车的信号
     */
    void onBeginRemoveRows(int start, int end) {
        beginRemoveRows({}, start, end);
    }
    void onEndRemoveRows() {
        endRemoveRows();
    }

    void onBeginInsertRows(int start, int end) {
        beginInsertRows({}, start, end);
    }
    void onEndInsertRows() {
        endInsertRows();
    }

    /**
     * 删除线路后调用，更新所有列车的里程、均速数据
     */
    void updateAllMileSpeed();

    void updateAllTrainShow();

    void updateAllTrainStartingTerminal();

    void updateAllTrainTypes();

    void refreshData();

    /**
     * 2022.02.06
     * 批量更新列车类型后调用，即更新表的内容。
     */
    void commitBatchChangeType(const QVector<int>& rows);
};

class TrainType;

namespace qecmd {

    /**
     * 删除一组列车。在TrainListWidget中调用。
     * 暂定持有TrainCollection的引用，undo/redo有权限执行添加删除操作。
     * 注意indexes应当排好序，并且和trains一一对应！
     */
    class RemoveTrains :
        public QUndoCommand
    {
        QList<std::shared_ptr<Train>> _trains;
        QList<int> _indexes;
        TrainCollection& coll;
        TrainListModel* const model;
    public:
        RemoveTrains(const QList<std::shared_ptr<Train>>& trains,
            const QList<int>& indexes, TrainCollection& coll_,
            TrainListModel* model_,
            QUndoCommand* parent = nullptr);

        virtual void undo()override;

        /**
         * 注意push操作会执行这个函数！
         * 因为TrainListWidget必须保证删除按钮是有效的（无论是否有Slot接受这个CMD）
         * 所以第一次的redo不能在这里做。置标志位。
         */
        virtual void redo()override;

        const auto& trains()const { return _trains; }
        auto& trains() { return _trains; }
    };

    /**
     * 排序。支持合并
     */
    class SortTrains :public QUndoCommand {
        QList<std::shared_ptr<Train>> ord;
        TrainListModel* const model;
        static constexpr int ID = 100;
        bool first = true;
    public:
        SortTrains(const QList<std::shared_ptr<Train>>& ord_, TrainListModel* model_,
            QUndoCommand* parent = nullptr);
        virtual void undo()override;
        virtual void redo()override;
        virtual int id()const override { return ID; }
        virtual bool mergeWith(const QUndoCommand* another)override;
    };

    class BatchChangeType :public QUndoCommand {
        TrainCollection& coll;
        QVector<int> indexes;
        QVector<std::shared_ptr<TrainType>> types;
        TrainListModel* const model;
    public:
        BatchChangeType(TrainCollection& coll_, const QVector<int>& indexes_, std::shared_ptr<TrainType> type,
            TrainListModel* model_, QUndoCommand* parent = nullptr);
        virtual void undo()override { commit(); }
        virtual void redo()override { commit(); }
    private:
        void commit();
    };

    struct StartingTerminalData;

}



