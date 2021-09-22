﻿#pragma once
#include <QSplitter>
#include <vector>
#include "data/rail/trackdiagramdata.h"
#include "util/buttongroup.hpp"

class QEMoveableModel;
class QTableView;
class QEControlledTable;
class QSpinBox;
class QSlider;
class RailStation;
class Railway;
class Diagram;
struct AdapterStation;
class TrainLine;
class TrackDiagram;
class TrackDiagramData;

/**
 * @brief The RailTrackSetupWidget class
 * 左侧那个设置的面板。形式上，对于RailTrackWidget来说，是单例
 */
class RailTrackSetupWidget: public QWidget
{
    Q_OBJECT
    TrackDiagramData& _data;
    RadioButtonGroup<3,QVBoxLayout>* gpMode;
    RadioButtonGroup<2>* gpMainStay;
    QSpinBox* spSame,*spOpps;
    QEControlledTable* ctable;
    QTableView* table;
    QEMoveableModel* model;
public:
    RailTrackSetupWidget(TrackDiagramData& data, QWidget* parent=nullptr);

private:
    void initUI();
signals:
    void applied();
    void actSaveOrder(const QList<QString>& order);
public slots:
    void refreshData();
private slots:
    void actApply();
    void actSave();
    void onModeChanged();

};


/**
 * @brief The RailTrackWidget class
 * pyETRC.StationVisualizeDialog  股道分析的对话框
 */
class RailTrackWidget : public QSplitter
{
    Q_OBJECT
    using events_t=std::vector<std::pair<std::shared_ptr<TrainLine>,
        const AdapterStation*>>;
    Diagram& diagram;
    std::shared_ptr<Railway> railway;
    std::shared_ptr<RailStation> station;

    events_t events;
    TrackDiagramData data;   // 必须在events后初始化
    TrackDiagram* trackDiagram;
    RailTrackSetupWidget* setupWidget;
    QSlider* slider;
public:
    RailTrackWidget(Diagram& diagram, std::shared_ptr<Railway> railway,
                    std::shared_ptr<RailStation> station, QWidget* parent=nullptr);
signals:
    void actSaveTrackOrder(std::shared_ptr<Railway> rail, std::shared_ptr<RailStation> station,
                      const QList<QString>& order);
private:
    void initUI();
public slots:
    void refreshData();
private slots:
    void onSaveOrder(const QList<QString>& order);
};

