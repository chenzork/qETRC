﻿#pragma once

#include <SARibbonContextCategory.h>
#include <memory>
#include <QLineEdit>
#include <DockWidget.h>
#include <SARibbonLineEdit.h>
#include <SARibbonComboBox.h>
#include <SARibbonCheckBox.h>
#include <QDoubleSpinBox>

#include "data/diagram/diagram.h"
#include "editors/basictrainwidget.h"
#include "util/linestylecombo.h"

class MainWindow;
/**
 * @brief The TrainContext class
 * 列车的ContextMenu。尝试从MainWindow分出来，是为了减少MainWindow代码量。
 * 把相关操作代理了，MainWindow只负责转发
 * 暂时按照QObject处理，但其实好像不是很必要
 */
class TrainContext : public QObject
{
    Q_OBJECT
    Diagram& diagram;
    std::shared_ptr<Train> train{};
    SARibbonContextCategory* const cont;
    MainWindow* const mw;

    SARibbonLineEdit* edName, * edStart, * edEnd;

    //后缀m表示可修改的控件
    SARibbonLineEdit *edNamem,*edStartm,*edEndm;
    SARibbonLineEdit *edNameDown,*edNameUp;
    SARibbonComboBox* comboType;
    PenStyleCombo* comboLs;
    SARibbonCheckBox *checkPassen;
    SARibbonToolButton* btnColor, * btnAutoUI;
    QDoubleSpinBox* spWidth;
    QColor tmpColor;

    QList<BasicTrainWidget*> basicWidgets;
    QList<ads::CDockWidget*> basicDocks;
public:
    TrainContext(Diagram& diagram_, SARibbonContextCategory* const context_, MainWindow* mw);
    auto* context() { return cont; }
    auto getTrain() { return train; }
    void resetTrain();

signals:
    void highlightTrainLine(std::shared_ptr<Train> train);
    void timetableChanged(std::shared_ptr<Train> train);
    void actRemoveTrain(int index);
    
private:
    void initUI();

    void setupTypeCombo();

    /**
     * 返回当前列车对应的BasicWidget的下标
     * 暂定线性查找
     * 如果找不到，返回-1
     */
    int getBasicWidgetIndex();

    int getBasicWidgetIndex(std::shared_ptr<Train> t);

    /**
     * 由所给dock查找下标，用于关闭面板时删除
     */
    int getBasicWidgetIndex(ads::CDockWidget* dock);

    /**
     * 更新指定车次打开的时刻表窗口。未见得是当前车次。
     */
    void updateTrainWidget(std::shared_ptr<Train> t);

public slots:
    void setTrain(std::shared_ptr<Train> train_);

    void removeTrainWidget(std::shared_ptr<Train> train);

    /**
     * 由更新命令直接调用，将操作cmd压栈
     */
    void onTrainTimetableChanged(std::shared_ptr<Train> train, std::shared_ptr<Train> table);

    /**
     * 实际更新时刻表。undo/redo调用。
     * 更新数据，重新绑定数据，更新时刻表页面，更新列表页面（可能有里程等数据的变化），铺画运行图
     */
    void commitTimetableChange(std::shared_ptr<Train> train, std::shared_ptr<Train> table);

    /**
     * 更新信息，重新铺画运行图（但不重新绑定）
     * 更新当前页面（目前只需要更新当前页面），通知列表页面更新
     * 似乎现在直接利用timetableChanged()信号就够了
     */
    void commitTraininfoChange(std::shared_ptr<Train> train, std::shared_ptr<Train> info);

    /**
     * 显示或者创建当前车次的基本编辑面板。
     */
    void actShowBasicWidget();

private slots:
    void showTrainEvents();
    void actShowTrainLine();

    void onTrainDockClosed();

    void removeBasicDockAt(int idx);

    void onFullNameChanged();

    void onAutoUIChanged(bool on);

    void actSelectColor();

    void actApply();

    void refreshData();

    void actRemoveCurrentTrain();
};


namespace qecmd {
    class ChangeTimetable :
        public QUndoCommand
    {
        std::shared_ptr<Train> train, table;
        TrainContext* const cont;
    public:
        ChangeTimetable(std::shared_ptr<Train> train, std::shared_ptr<Train> newtable,
            TrainContext* context, QUndoCommand* parent = nullptr);
        virtual void undo()override;
        virtual void redo()override;
    };

    class UpdateTrainInfo :
        public QUndoCommand
    {
        std::shared_ptr<Train> train, info;
        TrainContext* const cont;
    public:
        UpdateTrainInfo(std::shared_ptr<Train> train_, std::shared_ptr<Train> newinfo,
            TrainContext* context, QUndoCommand* parent = nullptr) :
            QUndoCommand(QObject::tr("更新列车信息: ") + newinfo->trainName().full()),
            train(train_), info(newinfo), cont(context) {}
        virtual void undo()override {
            cont->commitTraininfoChange(train, info);
        }
        virtual void redo()override {
            cont->commitTraininfoChange(train, info);
        }
    };
}

