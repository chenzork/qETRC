﻿#pragma once

#include <QMainWindow>
#include <QTreeView>
#include <QList>
#include <QUndoStack>
#include <QStringLiteral>

#include "data/rail/railway.h"
#include "data/diagram/diagram.h"

#include "kernel/diagramwidget.h"
#include "model/diagram/diagramnavimodel.h"
#include "editors/trainlistwidget.h"
#include "editors/railstationwidget.h"
#include "navi/navitree.h"

//for SARibbon
#include "SARibbonMainWindow.h"
#include "SARibbonMenu.h"
#include "SARibbonContextCategory.h"

#include "DockManager.h"

#include "traincontext.h"
#include "railcontext.h"
#include "viewcategory.h"

/**
 * @brief The MainWindow class
 */
class MainWindow : public SARibbonMainWindow
{
    Q_OBJECT
    Diagram _diagram;
    ads::CDockManager* manager;
    QUndoStack* undoStack;

    //窗口，Model的指针
    DiagramNaviModel* naviModel;
    NaviTree* naviView;
    SARibbonMenu* pageMenu, * railMenu, *appMenu;
    QList<ads::CDockWidget*> diagramDocks;
    QList<DiagramWidget*> diagramWidgets;
    ads::CDockWidget* naviDock, * trainListDock;
    TrainListWidget* trainListWidget;
    QList<RailStationWidget*> railStationWidgets;
    QList<ads::CDockWidget*> railStationDocks;

    SARibbonContextCategory* contextPage;
    TrainContext* contextTrain;
    RailContext* contextRail;
    ViewCategory* catView;

    /**
     * 可能在多个地方使用到的action，包装一下
     */
    struct {
        QAction* open, * save, * saveas, * newfile;
    } sharedActions;
    QList<QAction*> actRecent;

    friend class ViewCategory;

    bool changed = false;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();



    /**
     * 注意既然是Undo，这个就肯定在栈顶，直接删除最后一个就好了
     */
    void undoAddPage(std::shared_ptr<DiagramPage> page);

    /**
     * 调用前，应当已经把Page加入到Diagram中
     */
    void addPageWidget(std::shared_ptr<DiagramPage> page);

private:
    /**
     * 程序启动，构建UI
     * 注意启动的第一阶段先不管打开运行图的事情
     */
    void initUI();

    void initDockWidgets();

    void initToolbar();

    void initAppMenu();

    /**
     * 程序启动，依次尝试读取上次打开的和默认文件
     */
    void loadInitDiagram();

    /**
     * 打开新的运行图、重置运行图等之前执行。
     * 询问是否保存、
     * 清理当前运行图的东西。
     */
    bool clearDiagram();

    /**
     * 清理运行图数据。
     * 用在打开运行图失败时
     */
    void clearDiagramUnchecked();

    /**
     * 重置操作之前调用，例如打开运行图 
     */
    void beforeResetGraph();

    /**
     * 重置操作之后调用
     */
    void endResetGraph();

    /**
     * 打开新运行图时的操作
     * 每个Page添加一个窗口
     */
    void resetDiagramPages();

    /**
     * 打开运行图 返回是否成功
     */
    bool openGraph(const QString& filename);

    /**
     * 向所有运行图窗口添加指定运行线。
     * （简单封装）
     */
    void addTrainLine(Train& train);
    void removeTrainLine(Train& train);

    void updateWindowTitle();

    /**
     * 询问是否保存。返回程序是否继续
     */
    bool saveQuestion();

protected:
    virtual void closeEvent(QCloseEvent* e)override;


private slots:
    /**
     * act前缀表示action，强调用户直接动作
     */
    void actNewGraph();
    void actOpenGraph();
    void actSaveGraph();
    void actSaveGraphAs();

    void actPopupAppButton();

    void addRecentFile(const QString& filename);

    void resetRecentActions();

    void openRecentFile();
    

    /**
     * 用户操作的新增运行图页面，需做Undo操作
     */
    void actAddPage(std::shared_ptr<DiagramPage> page);

    void markChanged();

    void markUnchanged();

    /**
     * 引起contextMenu展示或者隐藏的操作
     */
    void focusInPage(std::shared_ptr<DiagramPage> page);
    void focusOutPage();
    void focusInTrain(std::shared_ptr<Train> train);
    void focusOutTrain();
    void focusInRailway(std::shared_ptr<Railway> rail);
    void focusOutRailway();

    void undoRemoveTrains(const QList<std::shared_ptr<Train>>& trains);
    void redoRemoveTrains(const QList<std::shared_ptr<Train>>& trains);

    /**
     * 线路站表变化时调用这里。
     * 目前的主要任务是重新铺画有关运行图。
     */
    void onStationTableChanged(std::shared_ptr<Railway> rail, bool equiv);

    /**
     * 更新所有包含指定线路的运行图。
     */
    void updateRailwayDiagrams(std::shared_ptr<Railway> rail);

    /**
     * 重新铺画所有运行图。
     */
    void updateAllDiagrams();

    /**
     * 导入车次。刷新相关面板，重新铺画运行图。
     */
    void onTrainsImported();

    /**
     * 删除所有车次
     */
    void removeAllTrains();
    

public slots:

    /**
     * 从TrainListWidget发起的删除列车操作
     * 注意包含UndoStack压栈操作！！
     */
    void trainsRemoved(const QList<std::shared_ptr<Train>>& trains, 
        const QList<int>& indexes, TrainListModel* model);

    /**
     * 列车列表变化（添加或删除），提示相关更新
     * 主要是通告Navi那边变化！
     */
    void informTrainListChanged();

    void informPageListChanged();

    void trainsReordered();

    void trainSorted(const QList<std::shared_ptr<Train>>& oldList, TrainListModel* model);

    /**
     * 打开（或创建）指定线路的编辑面板
     */
    void actOpenRailStationWidget(std::shared_ptr<Railway> rail);


};

